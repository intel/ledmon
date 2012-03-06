/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2011 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <config.h>

#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <scsi/sg.h>
#include <linux/bsg.h>
#include <sys/ioctl.h>


#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "ibpi.h"
#include "status.h"
#include "list.h"
#include "block.h"
#include "cntrl.h"
#include "scsi.h"
#include "enclosure.h"
#include "sysfs.h"
#include "utils.h"
#include "smp.h"

struct gpio_tx_register_byte {
  unsigned char activity:3;
  unsigned char locate:2;
  unsigned char error:3;
} __attribute__((__packed__));

struct gpio_tx_register_byte gpio_tx_table[4];

#define INIT_IBPI(err, loc, act)  \
  { 	.error = err,               \
      .locate = loc,              \
      .activity = act             \
  }

static const struct gpio_rx_table {
  struct gpio_tx_register_byte pattern;
  int support_mask;
} ibpi2sgpio[] = {
  [IBPI_PATTERN_UNKNOWN]        = { INIT_IBPI(0,0,0), 1 }, /* OK */
  [IBPI_PATTERN_ONESHOT_NORMAL] = { INIT_IBPI(0,0,0), 1 }, /* OK */
  [IBPI_PATTERN_NORMAL]         = { INIT_IBPI(0,0,0), 1 }, /* OK */
  [IBPI_PATTERN_DEGRADED]       = { INIT_IBPI(0,0,0), 0 }, /* NO */
  [IBPI_PATTERN_REBUILD]        = { INIT_IBPI(0,1,1), 1 }, /* OK */
  [IBPI_PATTERN_REBUILD_P]      = { INIT_IBPI(0,0,0), 0 }, /* NO */
  [IBPI_PATTERN_FAILED_ARRAY]   = { INIT_IBPI(0,0,0), 0 }, /* NO */
  [IBPI_PATTERN_HOTSPARE]       = { INIT_IBPI(0,0,0), 0 }, /* NO */
  [IBPI_PATTERN_PFA]            = { INIT_IBPI(0,0,0), 0 }, /* NO */
  [IBPI_PATTERN_FAILED_DRIVE]   = { INIT_IBPI(0,0,1), 1 }, /* OK */
  [IBPI_PATTERN_LOCATE]         = { INIT_IBPI(0,1,0), 1 }, /* OK */
  [IBPI_PATTERN_LOCATE_OFF]     = { INIT_IBPI(0,0,0), 1 }  /* OK */
};

struct smp_read_response_frame_header {
  uint8_t frame_type; /* =0x41 */
  uint8_t function; /* =0x02 for read, 0x82 for write */
  uint8_t function_result;
  uint8_t reserved;
  uint32_t read_data[0]; /* variable length of data */
  /* uint32_t crc; */
} __attribute__((__packed__));

struct smp_write_response_frame {
  uint8_t frame_type; /* =0x41 */
  uint8_t function; /* =0x02 for read, 0x82 for write */
  uint8_t function_result;
  uint8_t reserved;
  uint32_t crc;
} __attribute__((__packed__));

struct smp_read_request_frame {
  uint8_t frame_type; /* =0x40 */
  uint8_t function; /* =0x02 for read, 0x82 for write */
  uint8_t register_type;
  uint8_t register_index;
  uint8_t register_count;
  uint8_t reserved[3];
  uint32_t crc;
} __attribute__((__packed__));

struct smp_write_request_frame_header {
  uint8_t frame_type; /* =0x40 */
  uint8_t function; /* =0x02 for read, 0x82 for write */
  uint8_t register_type;
  uint8_t register_index;
  uint8_t register_count;
  uint8_t reserved[3];
  uint32_t data[0]; /* variable length of data */
  /* uint32_t crc; */
} __attribute__((__packed__));

struct sgpio_cfg_0_frame {
  uint8_t reserved;
  uint8_t reserved1:4;
  uint8_t version:4;
  uint8_t gp_register_count:4;
  uint8_t cfg_register_count:3;
  uint8_t enable:1;
  uint8_t supported_drive_cnt;
} __attribute__((__packed__));

struct sgpio_cfg_1_frame {
   uint8_t reserved;
   uint8_t blink_gen_a:4;
   uint8_t blink_gen_b:4;
   uint8_t forced_act_off:4;
   uint8_t max_act_on:4;
   uint8_t stretch_act_off:4;
   uint8_t stretch_act_on:4;
} __attribute__((__packed__));

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
  if (!df) {
    return -1;
  }
  if (fgets(buf, sizeof(buf), df) < 0) {
    fclose(df);
    return -1;
  }
  if (sscanf(buf, "%d:%d", &dmaj, &dmin) != 2) {
    fclose(df);
    return -1;
  }
  fclose(df);
  snprintf(buf, sizeof(buf), "/var/tmp/led.%d.%d.%d", dmaj, dmin, getpid());
  if (mknod(buf, S_IFCHR | S_IRUSR | S_IWUSR, makedev(dmaj, dmin)) < 0) {
    return -1;
  }
  hba_fd = open(buf, O_RDWR);
  unlink(buf);
  if (hba_fd < 0)
  {
    return -1;
  }
  return hba_fd;
}

/**
 * @brief close smp device
 */
static int _close_smp_device(int fd)
{
  return close(fd);
}

/* smp constants */
#define SMP_FRAME_TYPE_REQ	0x40
#define SMP_FRAME_TYPE_RESP	0x41

#define SMP_FUNC_GPIO_READ	0x02
#define SMP_FUNC_GPIO_WRITE	0x82

#define SMP_FRAME_CRC_LEN		sizeof(uint32_t)
#define SMP_DATA_CHUNK_SIZE	sizeof(uint32_t)

/* gpio constants */
/* gpio register types */
#define GPIO_REG_TYPE_CFG		0x00
#define GPIO_REG_TYPE_RX		0x01
#define GPIO_REG_TYPE_RX_GP	0x02
#define GPIO_REG_TYPE_TX		0x03
#define GPIO_REG_TYPE_TX_GP	0x04

/* gpio register indexes */
#define GPIO_REG_IND_CFG_0	0x00
#define GPIO_REG_IND_CFG_1	0x01

#define GPIO_REG_IND_RX_0		0x00
#define GPIO_REG_IND_RX_1		0x01

#define GPIO_REG_IND_TX_0		0x00
#define GPIO_REG_IND_TX_1		0x01

#define SG_RESPONSE_TIMEOUT (5 * 1000) /* 1000 as miliseconds multiplier */
#define SCSI_MAX_CDB_LENGTH	0x10

#define GPIO_STATUS_OK			0x00
#define GPIO_STATUS_FAILURE 0x80

/**
   @brief use sg protocol in order to send data directly to hba driver
 */
static int _send_smp_frame(int hba, void *data, size_t data_size, void *response, size_t *response_size)
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
  if (ioctl(hba, SG_IO, &sg_frame) < 0) {
    return -1;
  }

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
static int _start_smp_write_gpio(int hba, struct smp_write_request_frame_header *header, void *data, size_t len)
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
  memcpy(buf, header, sizeof (*header));
  memcpy(buf + sizeof(*header), data, len * SMP_DATA_CHUNK_SIZE);

  status = _send_smp_frame(hba, buf, sizeof(*header) + len * SMP_DATA_CHUNK_SIZE + SMP_FRAME_CRC_LEN,
                           &response, &response_size);

  /* if frame is somehow malformed return failure */
  if (status != GPIO_STATUS_OK ||
      response.frame_type != SMP_FRAME_TYPE_RESP ||
      response.function != header->function) {
    return GPIO_STATUS_FAILURE;
  }

  return response.function_result;
}

/**
   @brief prepare full smp frame ready to send to hba

   @note count is a number of 32bit words
 */
static int _start_smp_read_gpio(int hba, struct smp_read_request_frame *frame, void *data, size_t count)
{
  uint8_t buf[MAX_SMP_FRAME_LEN];
  struct smp_read_response_frame_header *response = (void *)buf;
  size_t response_size = sizeof(struct smp_read_response_frame_header) + count * SMP_DATA_CHUNK_SIZE + SMP_FRAME_CRC_LEN;
  int status;

  memset(buf, 0, response_size);

  /* create full frame */
  if (count * SMP_FRAME_CRC_LEN > MAX_SMP_FRAME_DATA)
    __set_errno_and_return(EINVAL);

  status = _send_smp_frame(hba, frame, sizeof(*frame), response, &response_size);

  if (status != GPIO_STATUS_OK ||
      response->frame_type != SMP_FRAME_TYPE_RESP ||
      response->function != frame->function) {
    return GPIO_STATUS_FAILURE;
  }

  if (response->function_result != GPIO_STATUS_OK)
    return response->function_result;

  memcpy(data, buf + sizeof (struct smp_read_response_frame_header), count * SMP_DATA_CHUNK_SIZE);
  return status;
}

/**
   @brief prepare smp frame header
 */
static int _smp_write_gpio(const char *path, int smp_reg_type, int smp_reg_index,
                   int smp_reg_count, void *data, size_t len)
{
  struct smp_write_request_frame_header header;
  int status;
  header.frame_type = SMP_FRAME_TYPE_REQ;
  header.function = SMP_FUNC_GPIO_WRITE;
  header.register_type = smp_reg_type;
  header.register_index = smp_reg_index;
  header.register_count = smp_reg_count;
  memset(header.reserved, 0, sizeof(header.reserved));
  int fd = _open_smp_device(path);
  status = _start_smp_write_gpio(fd, &header, data, len);
  _close_smp_device(fd);
  return status;
}

/**
   @brief prepare smp frame header
 */
static int _smp_read_gpio(const char *path, int smp_reg_type, int smp_reg_index,
                   int smp_reg_count, void *data, size_t len)
{
  struct smp_read_request_frame frame;
  int status;
  frame.frame_type = SMP_FRAME_TYPE_REQ;
  frame.function = SMP_FUNC_GPIO_READ;
  frame.register_type = smp_reg_type;
  frame.register_index = smp_reg_index;
  frame.register_count = smp_reg_count;
  memset(frame.reserved, 0, sizeof(frame.reserved));
  int fd = _open_smp_device(path);
  status = _start_smp_read_gpio(fd, &frame, data, len);
  _close_smp_device(fd);
  return status;
}

#define BLINK_GEN_1HZ									8
#define BLINK_GEN_2HZ									4
#define BLINK_GEN_4HZ									2
#define DEFAULT_FORCED_ACTIVITY_OFF		1
#define DEFAULT_MAXIMUM_ACTIVITY_ON		2
#define DEFAULT_STRETCH_ACTIVITY_OFF	0
#define DEFAULT_STRETCH_ACTIVITY_ON		0

#define DEFAULT_ISCI_SUPPORTED_DEVS		4

/* one data chunk is 32bit long */
#define SMP_DATA_CHUNKS								1

struct gpio_tx_register_byte *get_bdev_ibpi_buffer(struct block_device *bdevice)
{
  struct _host_type *hosts;
  char buf[PATH_MAX];
  char *p = strrchr(bdevice->cntrl_path, '/');
  if (!p++)
    return NULL;

  for (hosts = bdevice->cntrl->hosts; hosts; hosts = hosts->next) {
    snprintf(buf, sizeof(buf), "sas_host%d", hosts->host_id);

    if (strcmp(buf, p) == 0)
      return bdevice->cntrl->hosts->ibpi_state_buffer;
  }
  return NULL;
}

/**
 */
int scsi_smp_write(struct block_device *device, enum ibpi_pattern ibpi)
{
  const char *sysfs_path = device->cntrl_path;
  struct gpio_tx_register_byte *gpio_tx;
  struct gpio_tx_register_byte gpio_tx_data[4];

  if (sysfs_path == NULL) {
    __set_errno_and_return(EINVAL);
  }
  if ((ibpi < IBPI_PATTERN_NORMAL) || (ibpi > IBPI_PATTERN_LOCATE)) {
    __set_errno_and_return(ERANGE);
  }

  if (!ibpi2sgpio[ibpi].support_mask) {
    char *c = strrchr(device->sysfs_path, '/');
    if (c++) {
      log_warning("pattern not supported for device (/dev/%s)", c);
      fprintf(stderr, "%s(): pattern not supported for device (/dev/%s)\n", __func__, c);
    } else {
      log_warning("pattern not supported for device %s", device->sysfs_path);
      fprintf(stderr, "%s(): pattern not supported for device\n\t(%s)\n", __func__,
              device->sysfs_path);
    }
    __set_errno_and_return(ENOTSUP);
  }

  gpio_tx = get_bdev_ibpi_buffer(device);
  if (!gpio_tx) {
    /* controller's host smp module must be re-initialized */
    isci_cntrl_init_smp(device->sysfs_path, device->cntrl);
    gpio_tx = get_bdev_ibpi_buffer(device);
    if (!gpio_tx) {
      __set_errno_and_return(EINVAL);
    }
  }

  /* if we can read state of gpio_tx registry, update internal buffer */
  if (_smp_read_gpio(sysfs_path, GPIO_REG_TYPE_TX, GPIO_REG_IND_TX_0, /*count*/ 1,
                     &gpio_tx_data, SMP_DATA_CHUNKS) == GPIO_STATUS_OK) {
    memcpy(gpio_tx, gpio_tx_data, SMP_DATA_CHUNKS * sizeof(uint32_t));
  }

	/* update just one value */
  gpio_tx[(DEFAULT_ISCI_SUPPORTED_DEVS - 1) - device->phy_index] = ibpi2sgpio[ibpi].pattern;

	/* but send whole bunch */
  return _smp_write_gpio(sysfs_path, GPIO_REG_TYPE_TX, GPIO_REG_IND_TX_0, /*count*/ 1,
                         gpio_tx, SMP_DATA_CHUNKS);
}

/**
 */
void init_smp(const char *path, struct cntrl_device *device)
{
  char *host_path;
  struct _host_type *hosts = device->hosts;
  struct sgpio_cfg_0_frame cfg0;
  struct gpio_tx_register_byte *gpio_tx;
  int i;
  if (!device->isci_present)
    return;
  for (hosts = device->hosts; hosts; hosts = hosts->next) {
    /* already initialized */
    if (hosts->ibpi_state_buffer) {
      continue;
    }

    hosts->ibpi_state_buffer = calloc(DEFAULT_ISCI_SUPPORTED_DEVS,
                                      sizeof (struct gpio_tx_register_byte));
    if (!hosts->ibpi_state_buffer) {
      continue;
    }

    gpio_tx = hosts->ibpi_state_buffer;
    host_path = sas_get_slot_path(path, device->sysfs_path);
    memset(&cfg0, 0, sizeof(cfg0));
    cfg0.enable = 1;
    _smp_write_gpio(host_path, GPIO_REG_TYPE_CFG, GPIO_REG_IND_CFG_0, /*count*/ 1,
                    &cfg0, SMP_DATA_CHUNKS);

    /* try to read previous state of register. If it is not possible,
     * initialize to NORMAL */
    if (_smp_read_gpio(host_path, GPIO_REG_TYPE_TX, GPIO_REG_IND_TX_0, /*count*/ 1,
                       gpio_tx, SMP_DATA_CHUNKS)
        != GPIO_STATUS_OK) {
      /* initialize leds for all attached devices */
      for (i = 0; i < DEFAULT_ISCI_SUPPORTED_DEVS; i++)
        gpio_tx[(DEFAULT_ISCI_SUPPORTED_DEVS - 1) - i] = ibpi2sgpio[IBPI_PATTERN_ONESHOT_NORMAL].pattern;
    }
    _smp_write_gpio(host_path, GPIO_REG_TYPE_TX, GPIO_REG_IND_TX_0, /*count*/1,
                    gpio_tx, SMP_DATA_CHUNKS);
    free(host_path);
  }
}

/**
 */
int isci_cntrl_init_smp(const char *path, struct cntrl_device *cntrl)
{
  char *port_str;
  int host, port = 0;
  if (!cntrl->isci_present)
    return 0;
  port_str = get_path_component_rev(path, /* for hostN */ 5);
  if (!port_str)
    return port;
  sscanf(port_str, "port-%d:%d", &host, &port);
  free(port_str);

  init_smp(path, cntrl);
  return port;
}

