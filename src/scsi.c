/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2017 Intel Corporation.
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include <scsi/sg_lib.h>
#include <scsi/sg_cmds_extra.h>

#include "cntrl.h"
#include "config.h"
#include "enclosure.h"
#include "list.h"
#include "scsi.h"
#include "ses.h"
#include "status.h"
#include "sysfs.h"
#include "utils.h"

static int debug = 0;

static void print_page10(struct ses_pages *);

static void send_diag_slot(struct ses_pages *, int, const unsigned char *, int);

static int ses_set_message(enum ibpi_pattern, unsigned char *);

static inline int get_page2(int fd, unsigned char *p2, int *len)
{
	int ret;
	/* Get Enclosure Status */
	ret = sg_ll_receive_diag(fd, 1, ENCL_CTRL_DIAG_STATUS,
				 p2, SES_ALLOC_BUFF, 0, debug);
	*len = (p2[2] << 8) + p2[3] + 4;
	return ret;
}

static inline int get_page1(int fd, unsigned char *p1, int *len)
{
	int ret;
	/* Get Enclosure Status */
	ret = sg_ll_receive_diag(fd, 1, ENCL_CFG_DIAG_STATUS,
				 p1, SES_ALLOC_BUFF, 0, debug);
	*len = (p1[2] << 8) + p1[3] + 4;
	return ret;
}

static inline int get_page10(int fd, unsigned char *p10, int *len)
{
	int ret;
	/* Get Enclosure Status */
	ret = sg_ll_receive_diag(fd, 1, ENCL_ADDITIONAL_EL_STATUS,
				 p10, SES_ALLOC_BUFF, 0, debug);
	*len = (p10[2] << 8) + p10[3] + 4;
	return ret;
}

static int process_page1(struct ses_pages *sp)
{
	int num_encl;		/* number of subenclosures */
	unsigned char *ed;	/* Enclosure Descriptor */
	int len = 0;
	int sum_headers = 0;	/* Number of Type descriptor headers */
	int i = 0;

	/* How many enclosures is in the main enclosure? */
	num_encl = sp->page1[1] + 1;
	sp->page1_len = (sp->page1[2] << 8) + sp->page1[3] + 4;
	/* Go to Enclosure Descriptor */
	ed = sp->page1 + 8;
	for (i = 0; i < num_encl; i++, ed += len) {
		if (ed + 3 > sp->page1 + sp->page1_len) {
			log_debug
			    ("SES: Error, response pare 1 truncated at %d\n",
			     i);
			return 1;
		}
		sum_headers += ed[2];
		len = ed[3] + 4;
		if (len < 40) {
			log_debug("SES: Response too short for page 1\n");
			continue;
		}
	}

	sp->page1_types = ed;
	sp->page1_types_len = sum_headers;

	/* ed is on type descr header */
	for (i = 0; i < sum_headers; i++, ed += 4) {
		if (ed > sp->page1 + sp->page1_len) {
			log_debug("SES: Response page 1 truncated at %d\n", i);
			return 1;
		}
	}
	return 0;
}

static struct ses_pages *ses_init(void)
{
	struct ses_pages *sp;
	sp = calloc(1, sizeof(*sp));
	if (!sp)
		return NULL;
	sp->page1 = calloc(SES_ALLOC_BUFF, sizeof(*sp->page1));
	if (!sp->page1)
		goto sp1;
	sp->page2 = calloc(SES_ALLOC_BUFF, sizeof(*sp->page2));
	if (!sp->page2)
		goto sp2;
	sp->page10 = calloc(SES_ALLOC_BUFF, sizeof(*sp->page10));
	if (!sp->page10)
		goto sp10;
	return sp;

 sp10:
	free(sp->page2);
 sp2:
	free(sp->page1);
 sp1:
	free(sp);
	return NULL;
}

static void ses_free(struct ses_pages *sp)
{
	if (!sp)
		return;
	if (sp->page1)
		free(sp->page1);
	if (sp->page2)
		free(sp->page2);
	if (sp->page10)
		free(sp->page10);
	memset(sp, 0, sizeof(*sp));
	free(sp);
}

static void dump_p10(unsigned char *p)
{
	int i;
	printf("----------------------------------------------\n");
	for (i = 0; i < 8; i++, p += 16) {
		printf("%p: %02x %02x %02x %02x %02x %02x %02x " \
		       "%02x %02x %02x %02x %02x %02x %02x %02x %02x\n", p,
		       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
		       p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
	}
}

/* Enclosure is already opened. */
static int is_addr_in_encl(int fd, const char *addr, int *idx)
{
	/* Get page10 and read address. If it fits, return 1. */
	struct ses_pages *sp = ses_init();
	unsigned char *add_desc = NULL, *types = NULL;
	unsigned char *ap = NULL, *addr_p = NULL;
	int i, j, len = 0;
	char addr_cmp[32];

	if (!sp)
		return 0;
	/* Start processing */
	/* Read configuration. */
	if (get_page1(fd, sp->page1, &sp->page1_len))
		goto err;

	if (process_page1(sp))
		goto err;

	/* Get Enclosure Status - needed? */
	if (get_page2(fd, sp->page2, &sp->page2_len))
		goto err;

	/* Additional Element Status */
	if (get_page10(fd, sp->page10, &sp->page10_len))
		goto err;

	if (debug)
		print_page10(sp);

	/* Check Page10 for address. Extract index. */
	ap = add_desc = sp->page10 + 8;
	types = sp->page1_types;
	for (i = 0; i < sp->page1_types_len; i++, types += 4) {
		if (types[0] == SES_DEVICE_SLOT ||
		    types[0] == SES_ARRAY_DEVICE_SLOT) {
			for (j = 0; j < types[1]; j++, ap += len) {
				if (debug)
					dump_p10(ap);
				/* Get Additional Element Status Descriptor */
				/* length (x-1) */
				len = ap[1] + 2;
				if ((ap[0] & 0xf) != SCSI_PROTOCOL_SAS)
					continue;	/* need SAS PROTO */
				/* It is a SAS protocol, go on */
				if ((ap[0] & 0x10))	/* Check EIP */
					addr_p = ap + 8;
				else
					addr_p = ap + 4;
				/* Process only PHY 0 descriptor. */
				sprintf(addr_cmp,
					"%02x%02x%02x%02x%02x%02x%02x%02x",
					addr_p[12], addr_p[13], addr_p[14],
					addr_p[15], addr_p[16], addr_p[17],
					addr_p[18], addr_p[19]);

				if (!strcmp(addr + 2, addr_cmp)) {
					if (idx)
						*idx = ap[3];
					ses_free(sp);
					return 1;
				}
			}
		}
	}
 err:
	ses_free(sp);
	return 0;
}

static char *_get_dev_sg(const char *path)
{
	char *ret = NULL;
	DIR *d;
	struct dirent *de;

	/* /sys/class/enclosure/X/device/scsi_generic path is expected. */

	d = opendir(path);
	if (!d)
		return NULL;
	while ((de = readdir(d))) {
		if ((strcmp(de->d_name, ".") == 0) ||
		    (strcmp(de->d_name, "..")) == 0) {
			continue;
		}
		break;
		/* */
	}
	if (de) {
		ret = calloc(strlen("/dev/") +
			     strlen(de->d_name) + 1, sizeof(*ret));
		if (ret) {
			strcpy(ret, "/dev/");
			strcat(ret, de->d_name);
		}
	}
	closedir(d);
	return ret;
}

/* SYSFS_ENCL/<enclosure>/SCSI_GEN - path to sgX directory for enclosure. */
#define SYSFS_ENCL "/sys/class/enclosure"
#define SCSI_GEN "device/scsi_generic"
static int get_enclosure_fd(struct block_device *device, char *addr)
{
	DIR *d = NULL;
	struct dirent *de = NULL;
	char *p = NULL;
	int len;
	char *dev = NULL;
	int fd = -1;

	/* There may be device path already filled */
	if (device->encl_index != -1 && device->encl_dev[0] != 0) {
		fd = open(device->encl_dev, O_RDWR);
		if (fd == -1) {
			/* Enclosure device may change during extensive disks
			 * hotplugging */
			device->encl_index = -1;
			memset(device->encl_dev, 0, sizeof(device->encl_dev));

			if (!addr) {
				/* If there is no SAS address then there is
				 * no way to find enclosure device that this
				 * drive 'was' in. */
				return fd;
			}
		} else {
			return fd;
		}
	}

	if (!addr) {
		/* when there is no enclosure device and device index
		 * then address should not be NULL */
		return -1;
	}

	d = opendir(SYSFS_ENCL);
	if (!d)
		return -1;
	while ((de = readdir(d))) {
		if ((strcmp(de->d_name, ".") == 0) ||
		    (strcmp(de->d_name, "..")) == 0) {
			continue;
		}
		/* */
		len =
		    strlen(SCSI_GEN) + strlen(SYSFS_ENCL) + strlen(de->d_name) +
		    4;
		p = calloc(len, sizeof(*p));
		if (!p)
			break;
		strcpy(p, SYSFS_ENCL);
		strcat(p, "/");
		strcat(p, de->d_name);
		strcat(p, "/");
		strcat(p, SCSI_GEN);
		dev = _get_dev_sg(p);
		free(p);
		if (!dev)
			break;
		fd = open(dev, O_RDWR);
		if (fd == -1) {
			free(dev);
			break;
		}
		if (is_addr_in_encl(fd, addr, &device->encl_index)) {
			strncpy(device->encl_dev, dev, PATH_MAX);
			free(dev);
			break;	/* HIT */
		} else {
			close(fd);
			fd = -1;
		}
		free(dev);
	}
	closedir(d);
	return fd;
}

static void put_enclosure_fd(int fd)
{
	close(fd);
}

static void ses_send_to_idx(int fd, int idx, enum ibpi_pattern ibpi)
{
	unsigned char msg[4] = { 0 };
	struct ses_pages *sp;

	sp = ses_init();
	if (!sp)
		return;

	/* Start processing */
	/* Read configuration. */
	if (get_page1(fd, sp->page1, &sp->page1_len))
		goto err;

	if (process_page1(sp))
		goto err;

	/* Get Enclosure Status */
	if (get_page2(fd, sp->page2, &sp->page2_len))
		goto err;

	if (ses_set_message(ibpi, msg))
		goto err;	/* unknown message */

	send_diag_slot(sp, fd, msg, idx);

 err:
	ses_free(sp);
	return;
}

static void print_page10(struct ses_pages *sp)
{
	unsigned char *ai = sp->page10 + 8;
	int i = 0, len = 0, eip = 0;
	unsigned char *sas = NULL;

	while (ai < sp->page10 + sp->page10_len) {
		printf("%s()[%d]: Inv: %d, EIP: %d, Proto: 0x%04x\n", __func__,
		       i++, ((ai[0] & 0x80) >> 7), ((ai[0] & 0x10) >> 4),
		       ai[0] & 0xf);
		printf("\tDescriptor len (x-1): %d\n", ai[1] + 1);
		eip = ai[0] && 0x10;
		if (eip)
			printf("\tElement Index: %d\n", ai[3]);
		len = ai[1] + 2;
		if ((ai[0] & 0xf) == SCSI_PROTOCOL_SAS) {
			if (eip)
				sas = ai + 4;
			else
				sas = ai + 2;
			printf("\tProtocol SAS:\n");
			printf("\tNumber of phy descriptors: %d\n", sas[0]);
			printf("\tNot all phys: %d, descriptor type: 0x%1x\n",
			       (sas[1] & 1), ((sas[1] & 0xc0) >> 6));
			if (eip) {
				printf("\tDevice slot number: %d\n", sas[3]);
				sas += 2;
			}
			sas += 2;
			printf("\tDevice type: 0x%01x\n",
			       ((sas[0] & 0x70) >> 4));
			printf("\tSMP Initiator Port: 0x%01x\n",
			       ((sas[2] & 2) >> 1));
			printf("\tSTP Initiator Port: 0x%01x\n",
			       ((sas[2] & 4) >> 2));
			printf("\tSSP Initiator Port: 0x%01x\n",
			       ((sas[2] & 8) >> 3));
			printf("\tSATA DEVICE: 0x%01x\n", (sas[3] & 1));
			printf("\tSMP Target Port: 0x%01x\n",
			       ((sas[3] & 2) >> 1));
			printf("\tSTP Target Port: 0x%01x\n",
			       ((sas[3] & 4) >> 2));
			printf("\tSSP Target Port: 0x%01x\n",
			       ((sas[3] & 8) >> 3));
			printf("\tSATA Port Selector: 0x%01x\n",
			       ((sas[3] & 0X80) >> 7));
			printf
			    ("\tAttached SAS Address: 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
			     sas[4], sas[5], sas[6], sas[7], sas[8], sas[9],
			     sas[10], sas[11]);
			printf
			    ("\tSAS Address: 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
			     sas[12], sas[13], sas[14], sas[15], sas[16],
			     sas[17], sas[18], sas[19]);
			printf("\tPHY Identified: 0x%01x\n", sas[20]);
		} else
			printf("\tProtocol not SAS: 0x%02x, skipping\n",
			       (ai[0] & 0xf));
		/* */
		ai += len;
	}
	return;
}

static void send_diag_slot(struct ses_pages *sp, int fd,
			   const unsigned char *info, int idx)
{
	unsigned char *types = sp->page1_types;
	unsigned char *desc = sp->page2 + 8;	/* Move do descriptors */
	int i, j;
	int slot = 0;

	memset(desc, 0, sp->page2_len - 8);

	for (i = 0; i < sp->page1_types_len; i++, types += 4) {
		if (types[0] == 0x01 || types[0] == 0x17) {
			desc += 4;	/* At first, skip overall header. */
			for (j = 0; j < types[1]; j++, desc += 4) {
				if (slot++ == idx) {
					memcpy(desc, info, 4);
					/* set select */
					desc[0] |= 0x80;
					/* keep PRDFAIL */
					desc[0] |= 0x40;
					/* clear reserved flags */
					desc[0] &= 0xf0;
					break;
				}
			}
		}
	}
	if (sg_ll_send_diag(fd, 0, 1, 0, 0, 0, 0,
			    sp->page2, sp->page2_len, 0, debug)) {
		return;
	}
	return;
}

static int ses_set_message(enum ibpi_pattern ibpi, unsigned char *u)
{
	switch (ibpi) {
	case IBPI_PATTERN_UNKNOWN:
	case IBPI_PATTERN_ONESHOT_NORMAL:
	case IBPI_PATTERN_NORMAL:
		_clr_msg(u);
		_set_ok(u);
		break;
	case IBPI_PATTERN_FAILED_ARRAY:
		_set_ifa(u);
		break;
	case IBPI_PATTERN_DEGRADED:
		_set_ica(u);
		break;
	case IBPI_PATTERN_REBUILD:
	case IBPI_PATTERN_REBUILD_P:
		_set_rebuild(u);
		break;
	case IBPI_PATTERN_FAILED_DRIVE:
		_set_fault(u);
		break;
	case IBPI_PATTERN_LOCATE_OFF:
		_clr_ident(u);
		break;
	case IBPI_PATTERN_LOCATE:
		_set_ident(u);
		break;
	case IBPI_PATTERN_HOTSPARE:
		_set_hspare(u);
		break;
	case IBPI_PATTERN_PFA:
		_set_rsvd_dev(u);
		break;
		/* SES MESSAGES */
	case SES_REQ_ABORT:
		_set_abrt(u);
		break;
	case SES_REQ_REBUILD:
		_set_rebuild(u);
		break;
	case SES_REQ_IFA:
		_set_ifa(u);
		break;
	case SES_REQ_ICA:
		_set_ica(u);
		break;
	case SES_REQ_CONS_CHECK:
		_set_cons_check(u);
		break;
	case SES_REQ_HOSTSPARE:
		_set_hspare(u);
		break;
	case SES_REQ_RSVD_DEV:
		_set_rsvd_dev(u);
		break;
	case SES_REQ_OK:
		_set_ok(u);
		break;
	case SES_REQ_IDENT:
		_set_ident(u);
		break;
	case SES_REQ_RM:
		_set_rm(u);
		break;
	case SES_REQ_INS:
		_set_ins(u);
		break;
	case SES_REQ_MISSING:
		_set_miss(u);
		break;
	case SES_REQ_DNR:
		_set_dnr(u);
		break;
	case SES_REQ_ACTIVE:
		_set_actv(u);
		break;
	case SES_REQ_EN_BB:
		_set_enbb(u);
		break;
	case SES_REQ_EN_BA:
		_set_enba(u);
		break;
	case SES_REQ_DEV_OFF:
		_set_off(u);
		break;
	case SES_REQ_FAULT:
		_set_fault(u);
		break;
	default:
		return 1;
	}
	return 0;
}

/* SAS_ADDR_PATH - path in sysfs where sas_address for disk can be found */
#define SAS_ADDR_PATH "/sys/class/sas_end_device/%s/" \
						"device/sas_device/%s/sas_address"

static char *get_drive_end_dev(const char *path)
{
	char *s, *c, *p;

	c = strstr(path, "end_device");
	if (!c)
		return NULL;
	s = strchr(c, '/');
	if (!s)
		return NULL;
	p = calloc(s - c + 1, sizeof(*p));
	if (!p)
		return NULL;

	strncpy(p, c, s - c);
	p[s - c] = 0;
	return p;
}

static char *get_drive_sas_addr(const char *path)
{
#define ADDR_LEN 64
	int len = strlen(path);
	char *buff, *end_dev, addr[ADDR_LEN] = { 0 };
	int fd;

	/* Make big buffer. */
	buff = calloc(len * 2, sizeof(*buff));

	if (!buff)
		return NULL;

	end_dev = get_drive_end_dev(path);
	if (!end_dev) {
		free(buff);
		return NULL;
	}

	(void)sprintf(buff, SAS_ADDR_PATH, end_dev, end_dev);

	fd = open(buff, O_RDONLY);
	if (fd == -1) {
		free(end_dev);
		free(buff);
		return NULL;
	}
	if (read(fd, addr, ADDR_LEN) == ADDR_LEN) {
		/* The value should be 19. */
		free(end_dev);
		free(buff);
		close(fd);
		return NULL;
	}
	close(fd);
	len = strnlen(addr, ADDR_LEN);
	if (len && addr[len - 1] == '\n')
		addr[len - 1] = 0;
	else /* make sure that addr is null-terminated */
		addr[len < ADDR_LEN ? len : ADDR_LEN - 1] = 0;
	free(end_dev);
	free(buff);
	return strdup(addr);
}

/**
 */
static int _slot_match(const char *slot_path, const char *device_path)
{
	char temp[PATH_MAX], link[PATH_MAX];

	str_cpy(temp, slot_path, PATH_MAX);
	str_cat(temp, "/device", PATH_MAX);

	if (realpath(temp, link) == NULL)
		return 0;

	return strncmp(link, device_path, strlen(link)) == 0;
}

/**
 */
static char *_slot_find(const char *enclo_path, const char *device_path)
{
	void *dir;
	char *temp, *result = NULL;

	dir = scan_dir(enclo_path);
	if (dir) {
		temp = list_first_that(dir, _slot_match, device_path);
		if (temp)
			result = strdup(temp);
		list_fini(dir);
	}
	return result;
}

int scsi_get_enclosure(struct block_device *device)
{
	char *addr;
	int fd = -1;

	if (!device || !device->sysfs_path)
		return 0;

	memset(device->encl_dev, 0, sizeof(device->encl_dev));
	addr = get_drive_sas_addr(device->sysfs_path);
	if (addr == NULL)
		return 0;

	fd = get_enclosure_fd(device, addr);
	if (fd != -1)
		put_enclosure_fd(fd);
	if (addr)
		free(addr);

	return (fd == -1) ? 0 : 1;
}

/**
 */
int scsi_ses_write(struct block_device *device, enum ibpi_pattern ibpi)
{
	int fd = -1;
	char *addr = NULL;

	if (!device || !device->sysfs_path)
		__set_errno_and_return(EINVAL);

	/* write only if state has changed */
	if (ibpi == device->ibpi_prev)
		return 1;

	if ((ibpi < IBPI_PATTERN_NORMAL) || (ibpi > SES_REQ_FAULT))
		__set_errno_and_return(ERANGE);

	/* Failed drive is special. Path in sysfs may be not available.
	 * In other case re-read address.
	 * */
	if (ibpi != IBPI_PATTERN_FAILED_DRIVE) {
		addr = get_drive_sas_addr(device->sysfs_path);
		if (addr == NULL) {
			/* Device maybe gone during scan. */
			log_warning
			    ("Detected inconsistency. " \
			     "Marking device '%s' as failed.",
			     strstr(device->sysfs_path, "host"));
			ibpi = IBPI_PATTERN_FAILED_DRIVE;
			device->ibpi = IBPI_PATTERN_FAILED_DRIVE;

			/* FIXME: at worst case we may lose all reference
			 * to this drive and should remove it from list.
			 * No API for do that now. */
		}
	}
	fd = get_enclosure_fd(device, addr);
	if (fd != -1) {
		ses_send_to_idx(fd, device->encl_index, ibpi);
		put_enclosure_fd(fd);
	} else {
		log_warning
		    ("Unable to send %s message to %s. Device is missing?",
		     ibpi_str[ibpi], strstr(device->sysfs_path, "host"));
	}
	if (addr)
		free(addr);
	return 0;
}

/**
 * @brief Gets a path to slot of sas controller.
 *
 * This function returns a sysfs path to component of enclosure the device
 * belongs to.
 *
 * @param[in]      path           Canonical sysfs path to block device.
 *
 * @return A sysfs path to controller device associated with the given
 *         block device if successful, otherwise NULL pointer.
 */
static char *sas_get_slot_path(const char *path, const char *ctrl_path)
{
	char *host;
	char host_path[PATH_MAX] = { 0 };
	size_t ctrl_path_len = strlen(ctrl_path);

	if (strncmp(path, ctrl_path, ctrl_path_len) != 0)
		return NULL;
	host = get_path_hostN(path);
	if (host) {
		snprintf(host_path, sizeof(host_path), "%s/%s/bsg/sas_%s",
			 ctrl_path, host, host);
		free(host);
	}
	return str_dup(host_path);
}

/**
 */
static char *_get_enc_slot_path(const char *path)
{
	struct enclosure_device *device;
	char *result = NULL;

	device = sysfs_get_enclosure_devices();
	while (device) {
		result = _slot_find(device->sysfs_path, path);
		if (result != NULL)
			break;
		device = list_next(device);
	}
	return result;
}

/**
 */
char *scsi_get_slot_path(const char *path, const char *ctrl_path)
{
	char *result = NULL;

	result = _get_enc_slot_path(path);
	if (!result)
		result = sas_get_slot_path(path, ctrl_path);
	return result;
}
