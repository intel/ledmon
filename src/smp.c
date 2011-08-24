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


struct gpio_tx_register_byte {
  unsigned char error:3;
  unsigned char locate:2;
  unsigned char activity:3;
};

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

/**
 */
char *_byte2hex_stream(unsigned char *data, size_t len)
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

struct smp_response_frame {
  uint8_t frame_type;
  uint8_t function;
  uint8_t function_result;
  uint8_t reserved;
  uint32_t read_data[1];
};

struct sgpio_cfg_0_frame {
  uint8_t reserved;
  uint8_t reserverd1:4;
  uint8_t version:4;
  uint8_t gp_register_count:4;
  uint8_t cfg_register_count:3;
  uint8_t enable:1;
  uint8_t supported_drive_cnt;
 };

struct sgpio_cfg_1_frame {
   uint8_t reserved;
   uint8_t blink_gen_a:4;
   uint8_t blink_gen_b:4;
   uint8_t forced_act_off:4;
   uint8_t max_act_on:4;
   uint8_t stretch_act_off:4;
   uint8_t stretch_act_on:4;
 };

/**
 */
int start_smp_gpio(const char *cmd, void *response, size_t len)
{
  FILE *f;
  /* 8 for header */
  unsigned char smp_resp[1024 + 8];
  struct smp_response_frame *response_frame;
  size_t resp_len;
  int status;

  f = popen(cmd, "r");
  if (!f)
    return 0;
  resp_len = fread(smp_resp, 1, sizeof(smp_resp) - 1, f);
  response_frame = (void *)smp_resp;
  if (ferror(f))
    return 1;
  status = pclose(f);
  if (status != 0)
    return status;
  if (response_frame->function_result != 0)
    return response_frame->function_result;
  if (response && len > 0) {
    memcpy(response, response_frame->read_data, len);
  }
  return 0;
}

#define SMP_WRITE_GPIO "smp_write_gpio %s -r -t %d -i %d -c %d -d %s "

int smp_write_gpio(const char *path, int smp_reg, int smp_reg_index,
                   int smp_reg_count, void *data, size_t len)
{
   char *hex_stream;
   char cmd[PATH_MAX];
   hex_stream = _byte2hex_stream(data, len);
   snprintf(cmd, sizeof(cmd), SMP_WRITE_GPIO, path, smp_reg,
            smp_reg_index, smp_reg_count, hex_stream);
   free(hex_stream);

   return start_smp_gpio(cmd, NULL, 0);
}

#define BLINK_GEN_1HZ									8
#define BLINK_GEN_2HZ									4
#define BLINK_GEN_4HZ									2
#define DEFAULT_FORCED_ACTIVITY_OFF		1
#define DEFAULT_MAXIMUM_ACTIVITY_ON		2
#define DEFAULT_STRETCH_ACTIVITY_OFF	0
#define DEFAULT_STRETCH_ACTIVITY_ON		0

#define DEFAULT_ISCI_SUPPORTED_DEVS		4

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

  return smp_write_gpio(sysfs_path, /*registry*/ 3, /*index*/ 1,
                        /*count*/ 1, gpio_tx, DEFAULT_ISCI_SUPPORTED_DEVS);
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
  smp_write_gpio(host_path, /*registry*/ 0, /*index*/ 0, /*count*/ 1,
                 &cfg1, sizeof(cfg1));

  /* configure defaults for blinking */
  cfg1.blink_gen_a = BLINK_GEN_2HZ;
  cfg1.blink_gen_b = BLINK_GEN_1HZ;
  cfg1.forced_act_off = DEFAULT_FORCED_ACTIVITY_OFF;
  cfg1.max_act_on = DEFAULT_MAXIMUM_ACTIVITY_ON;
  cfg1.stretch_act_off = DEFAULT_STRETCH_ACTIVITY_OFF;
  cfg1.stretch_act_on = DEFAULT_STRETCH_ACTIVITY_ON;
  smp_write_gpio(host_path, /*registry*/ 0, /*index*/ 1, /*count*/1,
                 &cfg1, sizeof(cfg1));

  /* initialize leds for all attached devices */
  for (i = 0; i < DEFAULT_ISCI_SUPPORTED_DEVS; i++)
    gpio_tx[i] = ibpi2sgpio[IBPI_PATTERN_ONESHOT_NORMAL];
  smp_write_gpio(host_path, /*registry*/ 3, /*index*/ 0, /*count*/1,
                 device->ibpi_state_buffer, DEFAULT_ISCI_SUPPORTED_DEVS);
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

