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

/* #define USE_SMP_UTIL_GPIO 1 */

struct gpio_tx_register_byte {
  unsigned char error:3;
  unsigned char locate:2;
  unsigned char activity:3;
} __attribute__((__packed__));

struct gpio_tx_register_byte gpio_tx_table[4];

#define INIT_IBPI(err, loc, act)  \
  { 	.error = err,               \
      .locate = loc,              \
      .activity = act }

static const struct gpio_tx_register_byte ibpi2sgpio[] = {
  [IBPI_PATTERN_UNKNOWN]        = INIT_IBPI(0,0,0),
  [IBPI_PATTERN_ONESHOT_NORMAL] = INIT_IBPI(0,0,0),
  [IBPI_PATTERN_NORMAL]         = INIT_IBPI(0,0,0),
  [IBPI_PATTERN_DEGRADED]       = INIT_IBPI(0,2,0),
  [IBPI_PATTERN_REBUILD]        = INIT_IBPI(6,0,0),
  [IBPI_PATTERN_REBUILD_P]      = INIT_IBPI(1,1,0),
  [IBPI_PATTERN_FAILED_ARRAY]   = INIT_IBPI(0,2,0),
  [IBPI_PATTERN_HOTSPARE]       = INIT_IBPI(0,0,0),
  [IBPI_PATTERN_PFA]            = INIT_IBPI(2,0,0),
  [IBPI_PATTERN_FAILED_DRIVE]   = INIT_IBPI(1,0,0),
  [IBPI_PATTERN_LOCATE]         = INIT_IBPI(0,1,0),
  [IBPI_PATTERN_LOCATE_OFF]     = INIT_IBPI(0,0,0)
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

#ifndef USE_SMP_UTIL_GPIO
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

#define SG_RESPONSE_TIMEOUT 5 * 1000 /* 1000 as miliseconds multiplier */

/**
   @brief use sg protocol in order to send data directly to hba driver
 */
static int _send_smp_frame(int hba, void *data, size_t data_size, void *response, size_t response_size)
{
  struct sg_io_v4 sg_frame;
  uint8_t request_buf[16];
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

  sg_frame.din_xfer_len = response_size;
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

  return response_status;
}

static int _send_smp_write_frame(int hba, void *data, size_t data_size, struct smp_write_response_frame *response)
{
  int status;
  status = _send_smp_frame(hba, data, data_size, response, sizeof(*response));
  if (!status)
    status = response->function_result;
  return status;
}

/* 1024 for data, 4 for crc */
#define MAX_SMP_FRAME_DATA 1024
#define MAX_SMP_FRAME_LEN (sizeof(struct smp_write_request_frame_header) + \
                           MAX_SMP_FRAME_DATA + 4)

/**
   @brief prepare full smp frame ready to send to hba

   @note len is a number of 32bit words
 */
static int _start_smp_gpio(int hba, struct smp_write_request_frame_header *header, void *data, size_t len)
{
  uint8_t buf[MAX_SMP_FRAME_LEN];
  struct smp_write_response_frame response;
  int status;

  /* create full frame */
  if (len * sizeof(uint32_t) > MAX_SMP_FRAME_DATA)
    __set_errno_and_return(EINVAL);
  memcpy(buf, header, sizeof (*header));
  memcpy(buf + sizeof(*header), data, len * sizeof(uint32_t));
  /* crc is calculated and checked via driver. Generally, space for crc should
   * be allocated by caller, so we do this here */
  memset(buf + sizeof(*header) + len * sizeof(uint32_t), 0, sizeof(uint32_t));

  status = _send_smp_write_frame(hba, buf, sizeof(*header) + len * sizeof(uint32_t), &response);
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
  status = _start_smp_gpio(fd, &header, data, len);
  _close_smp_device(fd);
  return status;
}

#else /* USE_SMP_UTIL_GPIO */

/**
 */
static char *_byte2hex_stream(unsigned char *data, size_t len)
{
  char *buffer;
  int i;
  if (!data)
    return NULL;
  buffer = malloc(len * 3);
  if (!buffer)
    return NULL;
  for (i = 0; i < len; i++) {
    snprintf(buffer + i * 3, 4, "%02X,", data[i]);
  }
  buffer[i * 3 - 1] = '\0';
  return buffer;
}

/**
 */
static int _start_smputil_write_gpio(const char *cmd)
{
  FILE *f;
  struct smp_write_response_frame response_frame;
  size_t resp_len;
  int status;

  f = popen(cmd, "r");
  if (!f)
    return 0;
  resp_len = fread(&response_frame, 1, sizeof(response_frame), f);
  if (ferror(f))
    return 1;
  status = pclose(f);
  if (status != 0)
    return status;
  if (response_frame.function_result != 0)
    return response_frame.function_result;
  return 0;
}

#define SMP_WRITE_GPIO "smp_write_gpio %s -r -t %d -i %d -c %d -d %s"

static int _smp_write_gpio(const char *path, int smp_reg, int smp_reg_index,
                   int smp_reg_count, void *data, size_t len)
{
   char *hex_stream;
   char cmd[PATH_MAX];
   hex_stream = _byte2hex_stream(data, len * sizeof(uint32_t));
   snprintf(cmd, sizeof(cmd), SMP_WRITE_GPIO, path, smp_reg,
            smp_reg_index, smp_reg_count, hex_stream);
   free(hex_stream);

   return _start_smputil_write_gpio(cmd);
}

#endif /* USE_SMP_UTIL_GPIO */

#define BLINK_GEN_1HZ									8
#define BLINK_GEN_2HZ									4
#define BLINK_GEN_4HZ									2
#define DEFAULT_FORCED_ACTIVITY_OFF		1
#define DEFAULT_MAXIMUM_ACTIVITY_ON		2
#define DEFAULT_STRETCH_ACTIVITY_OFF	0
#define DEFAULT_STRETCH_ACTIVITY_ON		0

#define DEFAULT_ISCI_SUPPORTED_DEVS		4

/* one data chunk is 32bit long */
#define SMP_DATA_CHUNK_SIZE						1

/**
 */
int scsi_smputil_write(struct block_device *device, enum ibpi_pattern ibpi)
{
  const char *sysfs_path = device->cntrl_path;
  struct gpio_tx_register_byte *gpio_tx;

  if (sysfs_path == NULL) {
    __set_errno_and_return(EINVAL);
  }
  if ((ibpi < IBPI_PATTERN_NORMAL) || (ibpi > IBPI_PATTERN_LOCATE)) {
    __set_errno_and_return(ERANGE);
  }

  gpio_tx = device->cntrl->ibpi_state_buffer;
  gpio_tx[device->phy_index] = ibpi2sgpio[ibpi];

  return _smp_write_gpio(sysfs_path, /*registry*/ 3, /*index*/ 1,
                        /*count*/ 1, gpio_tx, SMP_DATA_CHUNK_SIZE);
}

/**
 */
void init_smp(const char *path, struct cntrl_device *device)
{
  char *host_path;
  struct sgpio_cfg_0_frame cfg0;
  struct sgpio_cfg_1_frame cfg1;
  struct gpio_tx_register_byte *gpio_tx;
  int i;
  if (!device->isci_present || device->ibpi_state_buffer)
    return;
  device->ibpi_state_buffer = calloc(DEFAULT_ISCI_SUPPORTED_DEVS,
                                     sizeof (struct gpio_tx_register_byte));
  if (!device->ibpi_state_buffer)
    return;
  gpio_tx = device->ibpi_state_buffer;
  host_path = sas_get_slot_path(path, device->sysfs_path);
  memset(&cfg0, 0, sizeof(cfg0));
  cfg0.enable = 1;
  _smp_write_gpio(host_path, /*registry*/ 0, /*index*/ 0, /*count*/ 1,
                  &cfg0, SMP_DATA_CHUNK_SIZE);

  /* configure defaults for blinking */
  cfg1.blink_gen_a = BLINK_GEN_2HZ;
  cfg1.blink_gen_b = BLINK_GEN_1HZ;
  cfg1.forced_act_off = DEFAULT_FORCED_ACTIVITY_OFF;
  cfg1.max_act_on = DEFAULT_MAXIMUM_ACTIVITY_ON;
  cfg1.stretch_act_off = DEFAULT_STRETCH_ACTIVITY_OFF;
  cfg1.stretch_act_on = DEFAULT_STRETCH_ACTIVITY_ON;
  _smp_write_gpio(host_path, /*registry*/ 0, /*index*/ 1, /*count*/1,
                  &cfg1, SMP_DATA_CHUNK_SIZE);

  /* initialize leds for all attached devices */
  for (i = 0; i < DEFAULT_ISCI_SUPPORTED_DEVS; i++)
    gpio_tx[(DEFAULT_ISCI_SUPPORTED_DEVS - 1) - i] = ibpi2sgpio[IBPI_PATTERN_ONESHOT_NORMAL];
  _smp_write_gpio(host_path, /*registry*/ 3, /*index*/ 0, /*count*/1,
                  device->ibpi_state_buffer, SMP_DATA_CHUNK_SIZE);
  free(host_path);
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

