/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "status.h"
#include "list.h"
#include "cntrl.h"
#include "utils.h"
#include "sysfs.h"

/**
 */
static int _is_scsi_cntrl(const char *path)
{
  uint64_t class;

  if (sysfs_enclosure_attached_to_cntrl(path) == 0)
    return 0;

  class = get_uint64(path, 0, "class");
  return (class & 0xff0000L) == 0x010000L;
}

/**
 */
static int _is_ahci_cntrl(const char *path)
{
  char temp[PATH_MAX], link[PATH_MAX], *t;

  str_cpy(temp, path, PATH_MAX);
  str_cat(temp, "/driver", PATH_MAX);

  if (realpath(temp, link) == NULL)
    return 0;

  t = strrchr(link, '/');
  if ((t != NULL) && (strcmp(t + 1, "ahci") != 0))
    return 0;

  return get_uint64(path, 0, "vendor") == 0x8086L;
}

/**
 * @brief Determines the type of controller.
 *
 * This is internal function of 'controller device' module. The function
 * determines the type of controller device. It might be AHCI, SCSI or
 * UNKNOWN device type.
 *
 * @param[in]      path           path to controller device in sysfs tree.
 *
 * @return The type of controller device. If the type returned is
 *         CNTRL_TYPE_UNKNOWN this means a controller device is not
 *         supported.
 */
static enum cntrl_type _get_type(const char *path)
{
  if (_is_scsi_cntrl(path)) {
    return CNTRL_TYPE_SCSI;
  }
  if (_is_ahci_cntrl(path)) {
    return CNTRL_TYPE_AHCI;
  }
  return CNTRL_TYPE_UNKNOWN;
}

/**
 * @brief Check if enclosure management is enabled.
 *
 * This is internal function of 'controller device' module. The function checks
 * whether enclosure management is enabled for AHCI controller.
 *
 * @param[in]      path           path to controller device in sysfs tree.
 *
 * @return 1 if enclosure management is enabled, otherwise the function returns 0.
 */
static unsigned int _ahci_em_messages(const char *path)
{
  return (get_int(path, 0, "driver/module/parameters/ahci_em_messages") != 0);
}

/*
 * Allocates memory for a new controller device structure. See cntrl.h for
 * details.
 */
struct cntrl_device * cntrl_device_init(const char *path)
{
  unsigned int em_enabled;
  enum cntrl_type type;
  struct cntrl_device *device = NULL;

  type = _get_type(path);
  if (type != CNTRL_TYPE_UNKNOWN) {
    switch (type) {
    case CNTRL_TYPE_SCSI:
      em_enabled = 1;
      break;
    case CNTRL_TYPE_AHCI:
      em_enabled = _ahci_em_messages(path);
      break;
    default:
      em_enabled = 0;
    }
    if (em_enabled) {
      device = malloc(sizeof(struct cntrl_device));
      if (device) {
        device->cntrl_type = type;
        device->sysfs_path = strdup(path);
      }
    } else {
      log_error("%s: enclosure management not " \
          "supported.", path);
    }
  }
  return device;
}

/*
 * Frees memory allocated for controller device structure. See cntrl.h for
 * details.
 */
void cntrl_device_fini(struct cntrl_device *device)
{
  if (device) {
    if (device->sysfs_path) {
      free(device->sysfs_path);
    }
  }
}
