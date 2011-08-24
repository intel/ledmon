/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009,2011 Intel Corporation. All rights reserved.
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

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "ibpi.h"
#include "block.h"
#include "ahci.h"
#include "utils.h"

/**
 * Time interval in u-seconds to wait before enclosure management message is being
 * sent to AHCI controller.
 */
#define EM_MSG_WAIT       1500

/**
 * This array maps IBPI pattern to value recognized by AHCI driver. The driver
 * uses this control number to issue SGPIO signals appropriately.
 */
static const unsigned int ibpi2sgpio[] = {
  [IBPI_PATTERN_UNKNOWN]        = 0x00000000,
  [IBPI_PATTERN_ONESHOT_NORMAL] = 0x00000000,
  [IBPI_PATTERN_NORMAL]         = 0x00000000,
  [IBPI_PATTERN_DEGRADED]       = 0x00200000,
  [IBPI_PATTERN_REBUILD]        = 0x00480000,
  [IBPI_PATTERN_REBUILD_P]      = 0x00480000,
  [IBPI_PATTERN_FAILED_ARRAY]   = 0x00280000,
  [IBPI_PATTERN_HOTSPARE]       = 0x01800000,
  [IBPI_PATTERN_PFA]            = 0x01400000,
  [IBPI_PATTERN_FAILED_DRIVE]   = 0x00400000,
  [IBPI_PATTERN_LOCATE]         = 0x00080000,
  [IBPI_PATTERN_LOCATE_OFF]     = 0x00080000
};

/*
 * The function sends a LED control message to AHCI controller. It uses
 * SGPIO to control the LEDs. See ahci.h for details.
 */
int ahci_sgpio_write(struct block_device *device, enum ibpi_pattern ibpi)
{
  char temp[WRITE_BUFFER_SIZE];
  char path[PATH_MAX];
  char *sysfs_path = device->cntrl_path;

  if (sysfs_path == NULL) {
    __set_errno_and_return(EINVAL);
  }
  if ((ibpi < IBPI_PATTERN_NORMAL) || (ibpi > IBPI_PATTERN_LOCATE_OFF)) {
    __set_errno_and_return(ERANGE);
  }

  sprintf(temp, "%u", ibpi2sgpio[ibpi]);

  str_cpy(path, sysfs_path, PATH_MAX);
  str_cat(path, "/em_message", PATH_MAX);

  usleep(EM_MSG_WAIT);
  return buf_write(path, temp) > 0;
}

/*
 * The function return path to SATA port in sysfs tree. See ahci.h for details.
 */
char *ahci_get_port_path(const char *path)
{
  char tmp[PATH_MAX], buf[BUFFER_MAX];
  char *p, *s;

  str_cpy(tmp, path, PATH_MAX);
  if ((p = strstr(tmp, "/target")) == NULL) {
    return NULL;
  }
  *p = '\0';
  if ((s = rindex(tmp, PATH_DELIM)) == NULL) {
    return NULL;
  }

  str_cpy(buf, s, BUFFER_MAX);
  str_cat(tmp, "/scsi_host", PATH_MAX);
  str_cat(tmp, buf, PATH_MAX);

  return str_dup(tmp);
}
