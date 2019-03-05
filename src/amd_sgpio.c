/*
 * AMD SGPIO LED control
 * Copyright (C) 2019, Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <sys/file.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "config.h"
#include "ibpi.h"
#include "list.h"
#include "utils.h"
#include "amd_sgpio.h"

#define HOST_CAP_EMS	(1 << 6)

#define REG_TYPE_CFG	0x00
#define REG_TYPE_TX	0x03
#define REG_TYPE_AMD	0xC0

#define MSG_TYPE_SGPIO	0x03

struct sgpio_header_register {
	uint32_t	reserved1:4;
	uint32_t	msg_type:4;	/* 0x03 - SGPIO */
	uint32_t	data_size:8;
	uint32_t	msg_size:8;
	uint32_t	reserved2:8;
} __attribute__ ((__packed__));

struct sgpio_request_register {
	uint32_t	frame_type:8;
	uint32_t	function:8;
	uint32_t	reg_type:8;
	uint32_t	reg_index:8;

	uint32_t	reg_count:8;
	uint32_t	reserved:24;
} __attribute__ ((__packed__));

struct sgpio_amd_register {
	uint32_t	initiator:1;
	uint32_t	reserved1:3;
	uint32_t	polarity_flip:1;
	uint32_t	bypass_enable:1;
	uint32_t	return_to_normal:1;
	uint32_t	unused:1;
	uint32_t	reserved2:24;
} __attribute__ ((__packed__));

struct sgpio_config_register {
	uint32_t	reserved1:8;
	uint32_t	reserved2:4;
	uint32_t	version:4;	/* default = 0 */
	uint32_t	gp_reg_count:4; /* read only */
	uint32_t	cfg_reg_count:3;
	uint32_t	sgpio_enable:1;
	uint32_t	drive_count:8;

	uint32_t	reserved3:8;
	uint32_t	blink_gen_a:4;
	uint32_t	blink_gen_b:4;
	uint32_t	force_activity_off:4;
	uint32_t	max_activity_on:4;
	uint32_t	stretch_activity_off:4;
	uint32_t	stretch_activity_on:4;
} __attribute__ ((__packed__));

struct drive_leds {
	uint8_t		error:3;
	uint8_t		locate:2;
	uint8_t		activity:3;
} __attribute__ ((__packed__));

struct sgpio_transmit_register {
	struct drive_leds	drive[4];
} __attribute__ ((__packed__));

struct amd_register {
	struct sgpio_header_register hdr;
	struct sgpio_request_register req;
	struct sgpio_amd_register amd;
} __attribute__ ((__packed__));

struct config_register {
	struct sgpio_header_register hdr;
	struct sgpio_request_register req;
	struct sgpio_config_register cfg;
} __attribute__ ((__packed__));

struct transmit_register {
	struct sgpio_header_register hdr;
	struct sgpio_request_register req;
	struct sgpio_transmit_register tx;
} __attribute__ ((__packed__));

static uint8_t ibpi_pattern[] = {
	[IBPI_PATTERN_NONE]		= 0x00,
	[IBPI_PATTERN_REBUILD]		= 0x07,
	[IBPI_PATTERN_HOTSPARE]		= 0x02,
	[IBPI_PATTERN_PFA]		= 0x03,
	[IBPI_PATTERN_FAILED_DRIVE]	= 0x00,
	[IBPI_PATTERN_LOCATE]		= 0x07,
	[IBPI_PATTERN_LOCATE_OFF]	= 0x00
};

#define INIT_LED(e, l, a)	{.error = e, .locate = l, .activity = a}
static struct drive_leds tx_leds_blink_gen_a[] = {
	[IBPI_PATTERN_NORMAL] =		INIT_LED(0, 0, 0b101),
	[IBPI_PATTERN_ONESHOT_NORMAL] =	INIT_LED(0, 0, 0b101),
	[IBPI_PATTERN_REBUILD] =	INIT_LED(0b010, 0, 0),
	[IBPI_PATTERN_HOTSPARE]	=	INIT_LED(0b010, 0, 0),
	[IBPI_PATTERN_PFA] =		INIT_LED(0b010, 0, 0),
	[IBPI_PATTERN_FAILED_DRIVE] =	INIT_LED(0b001, 0, 0),
	[IBPI_PATTERN_LOCATE] =		INIT_LED(0b010, 0, 0b010),
	[IBPI_PATTERN_LOCATE_OFF] =	INIT_LED(0, 0, 0b101),
	[99] = INIT_LED(0, 0, 0b101),
};

static struct drive_leds tx_leds_blink_gen_b[] = {
	[IBPI_PATTERN_NORMAL] =		INIT_LED(0, 0, 0b101),
	[IBPI_PATTERN_ONESHOT_NORMAL] =	INIT_LED(0, 0, 0b101),
	[IBPI_PATTERN_REBUILD] =	INIT_LED(0b110, 0, 0),
	[IBPI_PATTERN_HOTSPARE]	=	INIT_LED(0b110, 0, 0),
	[IBPI_PATTERN_PFA] =		INIT_LED(0b110, 0, 0),
	[IBPI_PATTERN_FAILED_DRIVE] =	INIT_LED(0b001, 0, 0),
	[IBPI_PATTERN_LOCATE] =		INIT_LED(0b110, 0, 0b110),
	[IBPI_PATTERN_LOCATE_OFF] =	INIT_LED(0, 0, 0b101),
	[99] = INIT_LED(0, 0, 0b101),
};

struct amd_drive {
	int ata_port;
	int port;
	int drive_bay;
	int initiator;
};

#define CACHE_SZ	1024
int cache_fd = 0;

struct cache_entry {
	struct drive_leds leds[4];
	uint8_t blink_gen_a;
	uint8_t blink_gen_b;
	uint16_t reserved;
} __attribute__ ((__packed__));

static struct cache_entry *sgpio_cache;

static void _put_cache(void)
{
	munmap(sgpio_cache, CACHE_SZ);

	if (cache_fd) {
		flock(cache_fd, LOCK_UN);
		fsync(cache_fd);
		close(cache_fd);
		cache_fd = 0;
	}
}

static int _open_and_map_cache(void)
{
	struct stat sbuf;

	if (cache_fd)
		return 0;

	cache_fd = shm_open("/ledmon_amd_sgpio_cache", O_RDWR | O_CREAT,
			    S_IRUSR | S_IWUSR);
	if (cache_fd < 1) {
		log_error("Couldn't open SGPIO cache: %s", strerror(errno));
		return -1;
	}

	flock(cache_fd, LOCK_EX);

	fstat(cache_fd, &sbuf);
	if (sbuf.st_size == 0)
		ftruncate(cache_fd, CACHE_SZ);

	sgpio_cache = mmap(NULL, CACHE_SZ, PROT_READ | PROT_WRITE,
			   MAP_SHARED, cache_fd, 0);
	if (sgpio_cache == MAP_FAILED) {
		log_error("Couldn't map SGPIO cache: %s", strerror(errno));
		_put_cache();
		return -1;
	}

	return 0;
}

static struct cache_entry *_get_cache(struct amd_drive *drive)
{
	int rc, index;
	int base_port;

	rc = _open_and_map_cache();
	if (rc)
		return NULL;

	/* The sgpio_cache is just an array of cache_entry structs, and
	 * each cache_entry describes the drive LED settings for 4 drives.
	 * To find the corresponding cache entry for an ata port we need
	 * to first round down to the nearest multiple of 4, then divide
	 * by four. This gives us the following mapping;
	 *	cache_entry[0] => drive 0 to drive 3
	 *	cache_entry[1] => drive 4 to drive 7
	 *	cache_entry[n] => drive (4*n) to drive (4*n + 3)
	 */
	base_port = (drive->ata_port / 4) * 4;
	index = base_port / 4;

	return &sgpio_cache[index];
}

static int _send_sgpio_register(const char *em_buffer_path, void *reg,
				int reg_len)
{
	int fd, count;
	int saved_errno;
	int retries = 3;

	do {
		fd = open(em_buffer_path, O_WRONLY);
		if (fd < 0) {
			log_error("Couldn't open EM buffer %s: %s",
				  em_buffer_path, strerror(errno));
			return -1;
		}

		count = write(fd, reg, reg_len);
		saved_errno = errno;
		close(fd);

		/* Insert small sleep to ensure hardware has enough time to
		 * see the register change and read it. Without the sleep
		 * multiple writes can result in an EBUSY return because
		 * hardware has not cleared the EM_CTL_TM (Transmit Message)
		 * bit.
		 */
		usleep(1000);

		if (count == reg_len || errno != EBUSY)
			break;
	} while (--retries != 0);

	close(fd);

	if (count != reg_len) {
		log_error("Couldn't write SGPIO register: %s",
			  strerror(saved_errno));
		return -1;
	}

	return 0;
}

static void _dump_cfg_register(struct config_register *cfg_reg)
{
	uint32_t *reg = (uint32_t *)cfg_reg;

	log_debug("\tCFG Register: %08x %08x %08x %08x %08x", reg[0],
		  reg[1], reg[2], reg[3], reg[4]);
}

static int _write_cfg_register(const char *em_buffer_path,
			       struct cache_entry *cache, int ibpi)
{
	struct config_register cfg_reg;

	memset(&cfg_reg, 0, sizeof(cfg_reg));

	cfg_reg.hdr.msg_type = MSG_TYPE_SGPIO;
	cfg_reg.hdr.msg_size = sizeof(cfg_reg);

	cfg_reg.req.frame_type = 0x40;
	cfg_reg.req.function = 0x82;
	cfg_reg.req.reg_type = REG_TYPE_CFG;
	cfg_reg.req.reg_count = 2;

	cfg_reg.cfg.sgpio_enable = 1;
	cfg_reg.cfg.force_activity_off = 1;
	cfg_reg.cfg.max_activity_on = 2;

	if (cache->blink_gen_a)
		cache->blink_gen_b = ibpi_pattern[ibpi];
	else
		cache->blink_gen_a = ibpi_pattern[ibpi];

	cfg_reg.cfg.blink_gen_a = cache->blink_gen_a;
	cfg_reg.cfg.blink_gen_b = cache->blink_gen_b;

	_dump_cfg_register(&cfg_reg);
	return _send_sgpio_register(em_buffer_path, &cfg_reg, sizeof(cfg_reg));
}

static void _dump_tx_register(struct transmit_register *tx_reg)
{
	uint32_t *reg = (uint32_t *)tx_reg;

	log_debug("\tTX Register:  %08x %08x %08x %08x", reg[0],
		  reg[1], reg[2], reg[3]);
}

static int _write_tx_register(const char *em_buffer_path,
			      struct transmit_register *tx_reg)
{
	tx_reg->hdr.msg_type = MSG_TYPE_SGPIO;
	tx_reg->hdr.msg_size = sizeof(tx_reg);

	tx_reg->req.frame_type = 0x40;
	tx_reg->req.function = 0x82;
	tx_reg->req.reg_type = REG_TYPE_TX;
	tx_reg->req.reg_count = 1;

	_dump_tx_register(tx_reg);
	return _send_sgpio_register(em_buffer_path, tx_reg, sizeof(*tx_reg));
}

static void _set_tx_drive_leds(struct transmit_register *tx_reg,
			       struct cache_entry *cache,
			       int drive_bay, int ibpi)
{
	int i;
	struct drive_leds *leds;

	if (cache->blink_gen_a)
		leds = &tx_leds_blink_gen_b[ibpi];
	else
		leds = &tx_leds_blink_gen_a[ibpi];

	cache->leds[drive_bay].error = leds->error;
	cache->leds[drive_bay].locate = leds->locate;
	cache->leds[drive_bay].activity = leds->activity;

	for (i = 0; i < 4; i++) {
		tx_reg->tx.drive[i].error = cache->leds[i].error;
		tx_reg->tx.drive[i].locate = cache->leds[i].locate;
		tx_reg->tx.drive[i].activity = cache->leds[i].activity;
	}
}

static int _init_tx_drive_leds(struct transmit_register *tx_reg,
			       struct cache_entry *cache)
{
	int i;
	int init_done = 0;

	memset(tx_reg, 0, sizeof(*tx_reg));

	for (i = 0; i < 4; i++) {
		if (cache->leds[i].error || cache->leds[i].locate ||
		    cache->leds[i].activity)
			continue;

		_set_tx_drive_leds(tx_reg, cache, i, 99);
		init_done = 1;
	}

	return init_done;
}

static void _dump_amd_register(struct amd_register *amd_reg)
{
	uint32_t *reg = (uint32_t *)amd_reg;

	log_debug("\tAMD Register: %08x %08x %08x %08x", reg[0],
		  reg[1], reg[2], reg[3]);
}

static int _write_amd_register(const char *em_buffer_path,
			       struct amd_drive *drive)
{
	struct amd_register amd_reg;

	memset(&amd_reg, 0, sizeof(amd_reg));

	amd_reg.hdr.msg_type = MSG_TYPE_SGPIO;
	amd_reg.hdr.msg_size = sizeof(amd_reg);

	amd_reg.req.frame_type = 0x40;
	amd_reg.req.function = 0x82;
	amd_reg.req.reg_type = REG_TYPE_AMD;
	amd_reg.req.reg_count = 1;

	amd_reg.amd.return_to_normal = 1;
	amd_reg.amd.bypass_enable = 1;
	amd_reg.amd.initiator = drive->initiator;

	_dump_amd_register(&amd_reg);
	return _send_sgpio_register(em_buffer_path, &amd_reg, sizeof(amd_reg));
}

static int _find_file_path(const char *start_path, const char *filename,
			   char *path, size_t path_len)
{
	int rc, found;
	struct stat sbuf;
	struct list dir;
	char *dir_name;
	const char *dir_path;

	rc = scan_dir(start_path, &dir);
	if (rc) {
		log_info("Failed to scan %s", start_path);
		return 0;
	}

	found = 0;
	list_for_each(&dir, dir_path) {
		dir_name = strrchr(dir_path, '/');
		if (!dir_name++)
			continue;

		if (strncmp(dir_name, filename, strlen(filename)) == 0) {
			char tmp[PATH_MAX];

			strncpy(tmp, dir_path, path_len);
			snprintf(path, path_len, "%s", dirname(tmp));

			found = 1;
			break;
		}

		if (lstat(dir_path, &sbuf) == -1)
			continue;

		if (S_ISDIR(sbuf.st_mode)) {
			found = _find_file_path(dir_path, filename,
						path, path_len);
			if (found)
				break;
		}
	}

	list_erase(&dir);
	return found;
}

static int _get_amd_drive(const char *start_path, struct amd_drive *drive)
{
	char *a, *p;
	int found;
	char path[PATH_MAX];
	char ata_dir[PATH_MAX];

	/* Start the search at the ataXX directory */
	strncpy(ata_dir, start_path, PATH_MAX);
	a = p = strstr(ata_dir, "ata");
	if (!p) {
		log_info("Couldn't find ata path for %s", start_path);
		return -1;
	}

	/* terminate the path after the ataXX/ part */
	p = strchr(p, '/');
	*p = '\0';

	/* skip past 'ata' to get the ata port number */
	a += 3;
	drive->ata_port = strtoul(a, NULL, 10);

	found = _find_file_path(ata_dir, "port_no", path, PATH_MAX);
	if (!found) {
		log_info("Couldn't find 'port_no' for %s\n", ata_dir);
		return -1;
	}

	drive->port = get_int(path, -1, "port_no");

	drive->drive_bay = 8 - drive->port;
	if (drive->drive_bay < 4) {
		drive->initiator = 1;
	} else {
		drive->drive_bay -= 4;
		drive->initiator = 0;
	}

	log_debug("AMD Drive: port %d, ata port %d, drive bay %d, initiator %d",
		  drive->port, drive->ata_port, drive->drive_bay,
		  drive->initiator);
	return 0;
}

static int _set_ibpi(struct block_device *device, enum ibpi_pattern ibpi)
{
	int rc;
	struct amd_drive drive;
	struct transmit_register tx_reg;
	struct cache_entry *cache;
	struct cache_entry cache_dup;

	log_debug("Setting %s:", ibpi_str[ibpi]);
	log_debug("\tdevice: ...%s", strstr(device->sysfs_path, "/ata"));
	log_debug("\tbuffer: ...%s", strstr(device->cntrl_path, "/ata"));

	/* Retrieve the port number and correlate that to the drive slot.
	 * Port numbers 8..1 correspond to slot numbers 0..7. This is
	 * further broken down (since we can only control 4 drive slots)
	 * such that initiator = 1 for slots 0..3 and initiator = 0 for
	 * slots 4..7, the slot value is reduced by 4 for slots 4..7 so
	 * we can calculate the correct bits to set in the register for
	 * that drive.
	 */
	rc = _get_amd_drive(device->sysfs_path, &drive);
	if (rc)
		return rc;

	cache = _get_cache(&drive);
	if (!cache)
		return -EINVAL;

	/* Save copy of cache entry */
	memcpy(&cache_dup, cache, sizeof(cache_dup));

	rc = _write_amd_register(device->cntrl_path, &drive);
	if (rc)
		goto _set_ibpi_error;

	rc = _write_cfg_register(device->cntrl_path, cache, ibpi);
	if (rc)
		goto _set_ibpi_error;

	memset(&tx_reg, 0, sizeof(tx_reg));
	_set_tx_drive_leds(&tx_reg, cache, drive.drive_bay, ibpi);
	rc = _write_tx_register(device->cntrl_path, &tx_reg);

_set_ibpi_error:
	if (rc) {
		/* Restore saved cache entry */
		memcpy(cache, &cache_dup, sizeof(*cache));
	}

	_put_cache();
	return rc;
}

static int _amd_sgpio_init_one(const char *path, struct amd_drive *drive,
			       struct cache_entry *cache)
{
	int rc, do_init;
	struct transmit_register tx_reg;

	do_init = _init_tx_drive_leds(&tx_reg, cache);
	if (!do_init)
		return 0;

	log_debug("Initializing host %d..%d:", drive->ata_port,
		  drive->ata_port + 3);
	log_debug("\tbuffer: %s", strstr(path, "/ata"));

	rc = _write_amd_register(path, drive);
	if (rc)
		return rc;

	rc = _write_cfg_register(path, cache, IBPI_PATTERN_NONE);
	if (rc)
		return rc;

	return _write_tx_register(path, &tx_reg);
}

static int _amd_sgpio_init(const char *path)
{
	int rc;
	char em_path[PATH_MAX];
	struct amd_drive drive;
	struct cache_entry *cache;
	struct cache_entry cache_dup;

	snprintf(em_path, PATH_MAX, "%s/em_buffer", path);

	rc = _get_amd_drive(em_path, &drive);
	if (rc) {
		log_error("Couldn't find drive info for %s\n", em_path);
		return rc;
	}

	cache = _get_cache(&drive);
	if (!cache) {
		log_error("Couldn't retrieve cache");
		return -1;
	}

	/* Save copy of cache entry */
	memcpy(&cache_dup, cache, sizeof(cache_dup));

	rc = _amd_sgpio_init_one(em_path, &drive, cache);
	if (rc) {
		log_error("SGPIO register init failed for bank %d, %s",
			  drive.initiator, em_path);

		/* Restore saved cache entry */
		memcpy(cache, &cache_dup, sizeof(*cache));

		goto _init_amd_sgpio_err;
	}

	_put_cache();

	/* AMD uses SGPIO registers to control drive LEDs in sets of 8
	 * drives. The initiator bit in the amd register controls which
	 * set of four drives (0-3 or 4-7) the transmit register is
	 * updating.
	 *
	 * When initializing the registers we want to do all 8 drives
	 * so we need to reset the drive ata_port and initiator values.
	 */
	if (drive.initiator) {
		drive.ata_port -= 4;
		drive.initiator = 0;
	} else {
		drive.ata_port += 4;
		drive.initiator = 1;
	}

	cache = _get_cache(&drive);
	if (!cache) {
		log_error("Couldn't retrieve cache");
		return -1;
	}

	/* Save copy of cache entry */
	memcpy(&cache_dup, cache, sizeof(cache_dup));

	rc = _amd_sgpio_init_one(em_path, &drive, cache);
	if (rc) {
		log_error("SGPIO register init failed for bank %d, %s",
			  drive.initiator, em_path);

		/* Restore saved cache entry */
		memcpy(cache, &cache_dup, sizeof(*cache));
	}

_init_amd_sgpio_err:
	_put_cache();
	return rc;
}

int amd_sgpio_em_enabled(const char *path)
{
	char *p;
	int rc, found;
	uint32_t caps;
	char em_path[PATH_MAX];

	/* Find base path for enclosure management */
	found = _find_file_path(path, "em_buffer", em_path, PATH_MAX);
	if (!found) {
		log_info("Couldn't find base EM path for %s\n", path);
		return 0;
	}

	/* Validate that enlosure management is supported */
	p = get_text(em_path, "em_message_supported");
	if (!p) {
		log_info("Couldn't get 'em_messages_supported' for %s",
			 path);
		return 0;
	}

	if (strstr(p, "sgpio") == NULL) {
		log_info("SGPIO EM not supported for %s\n", path);
		free(p);
		return 0;
	}

	free(p);

	/* Verify host enclosure management capabilities */
	p = get_text(em_path, "ahci_host_caps");
	if (!p) {
		log_info("Couldn't read host capabilities for %s\n", path);
		return 0;
	}

	rc = sscanf(p, "%" SCNx32, &caps);
	free(p);
	if (rc <= 0) {
		log_info("Couldn't parse host capabilities for %s", path);
		return 0;
	}

	if (!(caps & HOST_CAP_EMS)) {
		log_info("EM not supported for %s", path);
		return 0;
	}

	rc = _amd_sgpio_init(em_path);
	return rc ? 0 : 1;
}

int amd_sgpio_write(struct block_device *device, enum ibpi_pattern ibpi)
{
	/* write only if state has changed */
	if (ibpi == device->ibpi_prev)
		return 1;

	if ((ibpi < IBPI_PATTERN_NORMAL) || (ibpi > IBPI_PATTERN_LOCATE_OFF))
		__set_errno_and_return(ERANGE);

	if ((ibpi == IBPI_PATTERN_DEGRADED) ||
	    (ibpi == IBPI_PATTERN_FAILED_ARRAY))
		__set_errno_and_return(EOPNOTSUPP);

	return _set_ibpi(device, ibpi);
}

char *amd_sgpio_get_path(const char *cntrl_path)
{
	int len, found;
	char *em_buffer_path;
	char tmp[PATH_MAX];

	em_buffer_path = malloc(PATH_MAX);
	if (!em_buffer_path) {
		log_error("Couldn't allocate memory to get path for %s\n%s",
			  cntrl_path, strerror(errno));
		return NULL;
	}

	found = _find_file_path(cntrl_path, "em_buffer", tmp, PATH_MAX);
	if (!found) {
		log_error("Couldn't find EM buffer for %s\n", cntrl_path);
		free(em_buffer_path);
		return NULL;
	}

	len = snprintf(em_buffer_path, PATH_MAX, "%s/em_buffer", tmp);
	if (len < 0 || len >= PATH_MAX) {
		free(em_buffer_path);
		return NULL;
	}

	return em_buffer_path;
}
