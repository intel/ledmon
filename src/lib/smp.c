/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2022-2024 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/bsg.h>
#include <scsi/sg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/sysmacros.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "led/libled.h"
#include "block.h"
#include "cntrl.h"
#include "config.h"
#include "enclosure.h"
#include "led/libled.h"
#include "list.h"
#include "scsi.h"
#include "smp.h"
#include "status.h"
#include "sysfs.h"
#include "utils.h"
#include "libled_private.h"

#define GPIO_TX_GP1	0x01

#define INIT_IBPI(act, loc, err)  \
	{	.error = err,     \
		.locate = loc,    \
		.activity = act   \
	}

#define LED_OFF	0
#define LED_ON	1
#define LED_4HZ	2
#define LED_I4HZ	3
#define LED_EOF	4
#define LED_SOF	5
#define LED_2HZ	6
#define LED_I2HZ	7

static const struct gpio_rx_table {
	struct gpio_tx_register_byte pattern;
	int support_mask;
} ibpi2sgpio[] = {
	[LED_IBPI_PATTERN_UNKNOWN]        = { INIT_IBPI(LED_SOF, LED_OFF, LED_OFF), 1 }, /* OK */
	[LED_IBPI_PATTERN_ONESHOT_NORMAL] = { INIT_IBPI(LED_SOF, LED_OFF, LED_OFF), 1 }, /* OK */
	[LED_IBPI_PATTERN_NORMAL]         = { INIT_IBPI(LED_SOF, LED_OFF, LED_OFF), 1 }, /* OK */
	[LED_IBPI_PATTERN_DEGRADED]       = { INIT_IBPI(LED_SOF, LED_OFF, LED_OFF), 0 }, /* NO */
	[LED_IBPI_PATTERN_REBUILD]        = { INIT_IBPI(LED_SOF, LED_ON, LED_ON), 1 }, /* OK */
	[LED_IBPI_PATTERN_FAILED_ARRAY]   = { INIT_IBPI(LED_SOF, LED_4HZ, LED_OFF), 0 }, /* NO */
	[LED_IBPI_PATTERN_HOTSPARE]       = { INIT_IBPI(LED_SOF, LED_OFF, LED_4HZ), 0 }, /* NO */
	[LED_IBPI_PATTERN_PFA]            = { INIT_IBPI(LED_SOF, LED_OFF, LED_2HZ), 0 }, /* NO */
	[LED_IBPI_PATTERN_FAILED_DRIVE]   = { INIT_IBPI(LED_SOF, LED_OFF, LED_ON), 1 }, /* OK */
	[LED_IBPI_PATTERN_LOCATE]         = { INIT_IBPI(LED_SOF, LED_ON, LED_OFF), 1 }, /* OK */
	[LED_IBPI_PATTERN_LOCATE_OFF]     = { INIT_IBPI(LED_SOF, LED_OFF, LED_OFF), 1 }  /* OK */
};

struct smp_read_response_frame_header {
	uint8_t frame_type;	/* =0x41 */
	uint8_t function;	/* =0x02 for read, 0x82 for write */
	uint8_t function_result;
	uint8_t reserved;
	uint32_t read_data[];	/* variable length of data */
	/* uint32_t crc; */
} __attribute__ ((__packed__));

struct smp_write_response_frame {
	uint8_t frame_type;	/* =0x41 */
	uint8_t function;	/* =0x02 for read, 0x82 for write */
	uint8_t function_result;
	uint8_t reserved;
	uint32_t crc;
} __attribute__ ((__packed__));

struct smp_read_request_frame {
	uint8_t frame_type;	/* =0x40 */
	uint8_t function;	/* =0x02 for read, 0x82 for write */
	uint8_t register_type;
	uint8_t register_index;
	uint8_t register_count;
	uint8_t reserved[3];
	uint32_t crc;
} __attribute__ ((__packed__));

struct smp_write_request_frame_header {
	uint8_t frame_type;	/* =0x40 */
	uint8_t function;	/* =0x02 for read, 0x82 for write */
	uint8_t register_type;
	uint8_t register_index;
	uint8_t register_count;
	uint8_t reserved[3];
	uint32_t data[];	/* variable length of data */
	/* uint32_t crc; */
} __attribute__ ((__packed__));

/**
 * to_sas_gpio_gp_bit - given the gpio frame data find the byte/bit position of 'od'
 * @od: od bit to find
 * @data: incoming bitstream (from frame)
 * @index: requested data register index (from frame)
 * @count: total number of registers in the bitstream (from frame)
 * @bit: bit position of 'od' in the returned byte
 *
 * returns NULL if 'od' is not in 'data'
 *
 * From SFF-8485 v0.7:
 * "In GPIO_TX[1], bit 0 of byte 3 contains the first bit (i.e., OD0.0)
 *  and bit 7 of byte 0 contains the 32nd bit (i.e., OD10.1).
 *
 *  In GPIO_TX[2], bit 0 of byte 3 contains the 33rd bit (i.e., OD10.2)
 *  and bit 7 of byte 0 contains the 64th bit (i.e., OD21.0)."
 *
 * The general-purpose (raw-bitstream) RX registers have the same layout
 * although 'od' is renamed 'id' for 'input data'.
 *
 * SFF-8489 defines the behavior of the LEDs in response to the 'od' values.
 */
static unsigned char *to_sas_gpio_gp_bit(unsigned int od, unsigned char *data,
					 unsigned char index,
					 unsigned char count,
					 unsigned char *bit)
{
	unsigned int reg;
	unsigned char byte;

	/* gp registers start at index 1 */
	if (index == 0)
		return NULL;

	index--;		/* make index 0-based */
	if (od < index * 32)
		return NULL;

	od -= index * 32;
	reg = od >> 5;

	if (reg >= count)
		return NULL;

	od &= (1 << 5) - 1;
	byte = 3 - (od >> 3);
	*bit = od & ((1 << 3) - 1);

	return &data[reg * 4 + byte];
}

int try_test_sas_gpio_gp_bit(unsigned int od, unsigned char *data,
			     unsigned char index, unsigned char count)
{
	unsigned char *byte;
	unsigned char bit;

	byte = to_sas_gpio_gp_bit(od, data, index, count, &bit);
	if (!byte)
		return -1;

	return (*byte >> bit) & 1;
}

int try_set_sas_gpio_gp_bit(unsigned int od, unsigned char *data,
			    unsigned char index, unsigned char count)
{
	unsigned char *byte;
	unsigned char bit;

	byte = to_sas_gpio_gp_bit(od, data, index, count, &bit);
	if (!byte)
		return 0;

	*byte |= 1 << bit;
	return 1;
}

int try_clear_sas_gpio_gp_bit(unsigned int od, unsigned char *data,
			      unsigned char index, unsigned char count)
{
	unsigned char *byte;
	unsigned char bit;

	byte = to_sas_gpio_gp_bit(od, data, index, count, &bit);
	if (!byte)
		return 0;

	*byte &= ~(1 << bit);
	return 1;
}

/**
 * set_raw_pattern - turn a tx register into a tx_gp bitstream
 *
 * takes @dev_idx (phy index) and a @pattern (error, locate, activity)
 * tuple and modifies the bitstream in @data accordingly */
int set_raw_pattern(unsigned int dev_idx, unsigned char *data,
		    const struct gpio_tx_register_byte *pattern)
{
	int od_offset = dev_idx * 3;
	int rc = 0;

	if (pattern->activity == LED_ON)
		rc +=
		    try_set_sas_gpio_gp_bit(od_offset + 0, data, GPIO_TX_GP1,
					    1);
	else
		rc +=
		    try_clear_sas_gpio_gp_bit(od_offset + 0, data, GPIO_TX_GP1,
					      1);

	if (pattern->locate == LED_ON)
		rc +=
		    try_set_sas_gpio_gp_bit(od_offset + 1, data, GPIO_TX_GP1,
					    1);
	else
		rc +=
		    try_clear_sas_gpio_gp_bit(od_offset + 1, data, GPIO_TX_GP1,
					      1);

	if (pattern->error == LED_ON)
		rc +=
		    try_set_sas_gpio_gp_bit(od_offset + 2, data, GPIO_TX_GP1,
					    1);
	else
		rc +=
		    try_clear_sas_gpio_gp_bit(od_offset + 2, data, GPIO_TX_GP1,
					      1);

	return rc == 3;
}

/**
 * @brief open device for smp protocol
 */
static int _open_smp_device(const char *filename)
{
	char buf[PATH_MAX];
	FILE *df;
	int hba_fd;
	int dmaj, dmin;
	snprintf(buf, sizeof(buf), "%s/dev", filename);
	df = fopen(buf, "r");
	if (!df)
		return -1;
	if (fgets(buf, sizeof(buf), df) == NULL) {
		fclose(df);
		return -1;
	}
	if (sscanf(buf, "%d:%d", &dmaj, &dmin) != 2) {
		fclose(df);
		return -1;
	}
	fclose(df);
	snprintf(buf, sizeof(buf), "/var/tmp/led.%d.%d.%d", dmaj, dmin,
		 getpid());
	if (mknod(buf, S_IFCHR | S_IRUSR | S_IWUSR, makedev(dmaj, dmin)) < 0)
		return -1;
	hba_fd = open(buf, O_RDWR);
	unlink(buf);
	if (hba_fd < 0)
		return -1;
	return hba_fd;
}

/**
 * @brief close smp device
 */
static int _close_smp_device(int fd)
{
	return close(fd);
}

/**
   @brief use sg protocol in order to send data directly to hba driver
 */
static int _send_smp_frame(int hba, void *data, size_t data_size,
			   void *response, size_t *response_size)
{
	struct sg_io_v4 sg_frame;
	uint8_t request_buf[SCSI_MAX_CDB_LENGTH];
	int response_status = 0;

	/* wrap the frame into sg structure */
	memset(&sg_frame, 0, sizeof(sg_frame));
	sg_frame.guard = 'Q';
	sg_frame.protocol = BSG_PROTOCOL_SCSI;
	sg_frame.subprotocol = BSG_SUB_PROTOCOL_SCSI_TRANSPORT;

	sg_frame.request_len = sizeof(request_buf);
	sg_frame.request = (uintptr_t) request_buf;

	sg_frame.dout_xfer_len = data_size;
	sg_frame.dout_xferp = (uintptr_t) data;

	sg_frame.din_xfer_len = *response_size;
	sg_frame.din_xferp = (uintptr_t) response;

	sg_frame.timeout = SG_RESPONSE_TIMEOUT;
	/* send ioctl */
	if (ioctl(hba, SG_IO, &sg_frame) < 0)
		return -1;

	/* return status */
	if (sg_frame.driver_status)
		response_status = sg_frame.driver_status;
	else if (sg_frame.transport_status)
		response_status = sg_frame.transport_status;
	else if (sg_frame.device_status)
		response_status = sg_frame.device_status;

	*response_size = sg_frame.din_xfer_len - sg_frame.din_resid;

	return response_status;
}

/* 1024 bytes for data, 4 for crc */
#define MAX_SMP_FRAME_DATA 1024
#define MAX_SMP_FRAME_LEN (sizeof(struct smp_write_request_frame_header) + \
				MAX_SMP_FRAME_DATA + SMP_FRAME_CRC_LEN)

/**
   @brief prepare full smp frame ready to send to hba

   @note len is a number of 32bit words
 */
static int _start_smp_write_gpio(int hba,
				 struct smp_write_request_frame_header *header,
				 void *data, size_t len)
{
	uint8_t buf[MAX_SMP_FRAME_LEN];
	struct smp_write_response_frame response;
	size_t response_size = sizeof(response);
	int status;

	memset(&response, 0, sizeof(response));
	/* create full frame */
	if (len * SMP_DATA_CHUNK_SIZE > MAX_SMP_FRAME_DATA)
		__set_errno_and_return(EINVAL);
	memset(buf, 0, sizeof(buf));
	memcpy(buf, header, sizeof(*header));
	memcpy(buf + sizeof(*header), data, len * SMP_DATA_CHUNK_SIZE);

	status =
	    _send_smp_frame(hba, buf,
			    sizeof(*header) + len * SMP_DATA_CHUNK_SIZE +
			    SMP_FRAME_CRC_LEN, &response, &response_size);

	/* if frame is somehow malformed return failure */
	if (status != GPIO_STATUS_OK ||
	    response.frame_type != SMP_FRAME_TYPE_RESP ||
	    response.function != header->function) {
		return GPIO_STATUS_FAILURE;
	}

	return response.function_result;
}

/**
   @brief prepare smp frame header
 */
int smp_write_gpio(const char *path, int smp_reg_type,
			   int smp_reg_index, int smp_reg_count, void *data,
			   size_t len)
{
	struct smp_write_request_frame_header header;
	int status = -1;

	memset(&header, 0, sizeof(struct smp_write_request_frame_header));
	header.frame_type = SMP_FRAME_TYPE_REQ;
	header.function = SMP_FUNC_GPIO_WRITE;
	header.register_type = smp_reg_type;
	header.register_index = smp_reg_index;
	header.register_count = smp_reg_count;
	int fd = _open_smp_device(path);
	if (fd != -1) {
		status = _start_smp_write_gpio(fd, &header, data, len);
		_close_smp_device(fd);
	}
	return status;
}

#define BLINK_GEN_1HZ				8
#define BLINK_GEN_2HZ				4
#define BLINK_GEN_4HZ				2
#define DEFAULT_FORCED_ACTIVITY_OFF		1
#define DEFAULT_MAXIMUM_ACTIVITY_ON		2
#define DEFAULT_STRETCH_ACTIVITY_OFF		0
#define DEFAULT_STRETCH_ACTIVITY_ON		0

/* one data chunk is 32bit long */
#define SMP_DATA_CHUNKS				1

struct gpio_tx_register_byte *get_bdev_ibpi_buffer(struct block_device *bdevice)
{
	if (bdevice && bdevice->host)
		return bdevice->host->ibpi_state_buffer;
	return NULL;
}

/**
 */
int scsi_smp_fill_buffer(struct block_device *device, enum led_ibpi_pattern ibpi)
{
	const char *sysfs_path = device->cntrl_path;
	struct gpio_tx_register_byte *gpio_tx;

	if (sysfs_path == NULL)
		__set_errno_and_return(EINVAL);
	if ((ibpi < LED_IBPI_PATTERN_NORMAL) || (ibpi > LED_IBPI_PATTERN_LOCATE_OFF))
		__set_errno_and_return(ERANGE);
	if (!device->cntrl) {
		/* Unable to log here as we need the device->cntrl to not be null to access ctx */
		__set_errno_and_return(ENODEV);
	}
	if (device->cntrl->cntrl_type != LED_CNTRL_TYPE_SCSI) {
		lib_log(device->cntrl->ctx, LED_LOG_LEVEL_DEBUG,
			"No SCSI ctrl dev '%s'", strstr(sysfs_path, "host"));
		__set_errno_and_return(EINVAL);
	}
	if (!device->host) {
		lib_log(device->cntrl->ctx, LED_LOG_LEVEL_DEBUG,
			"No host for '%s'", strstr(sysfs_path, "host"));
		__set_errno_and_return(ENODEV);
	}

	if (device->cntrl->isci_present && !ibpi2sgpio[ibpi].support_mask) {
		char *c = strrchr(device->sysfs_path, '/');

		if (c++) {
			lib_log(device->cntrl->ctx, LED_LOG_LEVEL_DEBUG,
				"pattern %s not supported for device (/dev/%s)", ibpi2str(ibpi), c);
		} else {
			lib_log(device->cntrl->ctx, LED_LOG_LEVEL_DEBUG,
				"pattern %s not supported for device %s", ibpi2str(ibpi),
				device->sysfs_path);
		}
		__set_errno_and_return(ENOTSUP);
	}

	gpio_tx = get_bdev_ibpi_buffer(device);
	if (!gpio_tx) {
		lib_log(device->cntrl->ctx, LED_LOG_LEVEL_DEBUG,
			"%s(): no IBPI buffer. Skipping.", __func__);
		__set_errno_and_return(ENODEV);
	}

	if (device->cntrl->isci_present) {
		/* update bit stream for this device */
		set_raw_pattern(device->phy_index,
			&device->host->bitstream[0], &ibpi2sgpio[ibpi].pattern);
	} else {
		/*
		 * GPIO_TX[n] register has the highest numbered drive of the
		 * four in the first byte and the lowest numbered drive in the
		 * fourth byte. See SFF-8485 Rev. 0.7 Table 24.
		 */
		gpio_tx[device->phy_index + 3 - (device->phy_index % 4) * 2] =
			ibpi2sgpio[ibpi].pattern;
	}

	/* write only if state has changed */
	if (ibpi != device->ibpi_prev)
		device->host->flush = 1;

	return 1;
}

int scsi_smp_write_buffer(struct block_device *device)
{
	const char *sysfs_path = device->cntrl_path;

	if (sysfs_path == NULL)
		__set_errno_and_return(EINVAL);
	if (!device->host)
		__set_errno_and_return(ENODEV);

	if (device->host->flush) {
		device->host->flush = 0;
		/* re-transmit the bitstream */
		if (device->cntrl->isci_present) {
			return smp_write_gpio(sysfs_path,
					       GPIO_REG_TYPE_TX_GP,
					       GPIO_TX_GP1, 1,
					       &device->host->bitstream[0],
					       SMP_DATA_CHUNKS);
		} else {
			return smp_write_gpio(sysfs_path,
					       GPIO_REG_TYPE_TX,
					       0, (device->host->ports+3)/4,
					       device->host->ibpi_state_buffer,
					       (device->host->ports+3)/4);
		}
	} else
		return 1;
}

/**
 */
static void init_smp(struct cntrl_device *device)
{
	struct _host_type *hosts;
	int i;
	if (!device)
		return;

	for (hosts = device->hosts; hosts; hosts = hosts->next) {
		/* already initialized */
		if (hosts->ibpi_state_buffer)
			continue;
		hosts->ibpi_state_buffer =
				calloc(hosts->ports,
					sizeof(struct
					gpio_tx_register_byte));

		if (!hosts->ibpi_state_buffer)
			continue;

		for (i = 0; i < hosts->ports; i++)
			set_raw_pattern(i, &hosts->bitstream[0],
					&ibpi2sgpio
					[LED_IBPI_PATTERN_ONESHOT_NORMAL].pattern);
		hosts->flush = 0;
	}
}

/**
 */
int cntrl_init_smp(const char *path, struct cntrl_device *cntrl)
{
	char *path2 = NULL;
	char *c;
	int port = 0;
	struct dirent *de;
	DIR *d;

	if (!cntrl)
		return port;

	/* Other case - just init controller. */
	if (path && strstr(path, "port-")) {
		path2 = strdup(path);
		if (!path2)
			return port;

		c = strstr(path2, "port-");
		if (!c) {
			/* Should not happen. */
			lib_log(cntrl->ctx, LED_LOG_LEVEL_DEBUG,
				"%s() missing 'port' in path '%s'", __func__, path2);
			free(path2);
			return port;
		}
		c = strchr(c, '/');
		if (!c) {
			free(path2);
			return port;
		}
		*c = 0;
		/* And now path2 has only up to 'port-...' string. */

		/* this should open port-XX:X directory
		 * FIXME: for enclosure it may be port-XX:Y:Z but it's a second
		 * occurrence
		 *
		 * We may try this on the missing device.
		 * */
		d = opendir(path2);
		if (!d) {
			lib_log(cntrl->ctx, LED_LOG_LEVEL_DEBUG,
				"%s() Error dir open '%s', path ='%s'",
				__func__, path2, path);
			free(path2);
			return port;
		}
		while ((de = readdir(d))) {
			if ((strcmp(de->d_name, ".") == 0) ||
			    (strcmp(de->d_name, "..")) == 0) {
				continue;
			}
			if (strncmp(de->d_name, "phy-", strlen("phy-")) == 0) {
				/* Need link called "phy-XX:Y
				 * Y is real phy we need.
				 * This can also be found
				 * in phy_identifier file
				 */
				char *s = strstr(de->d_name, ":") + 1;

				if (str_toi(&port, s, NULL, 10) != 0)
					continue;

				break;
			}
		}
		closedir(d);
		free(path2);
	}
	init_smp(cntrl);
	return port;
}
