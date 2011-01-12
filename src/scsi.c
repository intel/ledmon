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
#include "scsi.h"
#include "enclosure.h"
#include "sysfs.h"
#include "utils.h"

/**
 */
#define STATUS_OK                   "OK"
#define STATUS_NON_CRITICAL         "non-critical"
#define STATUS_CRITICAL             "critical"
#define STATUS_UNRECOVERABLE        "unrecoverable"
#define STATUS_UNAVAILABLE          "unavailable"

/**
 */
#define VALUE_ON                    "1"
#define VALUE_OFF                   "0"

/**
 */
#define _ENCLOSURE_LOCATE(__path, __value)  \
  _enclosure_write(__path, "locate", __value)

/**
 */
#define _ENCLOSURE_FAULT(__path, __value)   \
  _enclosure_write(__path, "fault", __value)

/**
 */
#define _ENCLOSURE_STATUS(__path, __status) \
  _enclosure_write(__path, "status", __status)

/**
 * @brief Writes enclosure attribute.
 *
 * This is internal function of scsi module. This function writes a value of
 * an attribute to sysfs.
 */
static int _enclosure_write(const char *path, const char *file, const char *value)
{
  char temp[PATH_MAX];

  str_cpy(temp, path, PATH_MAX);
  str_cat(temp, PATH_DELIM_STR, PATH_MAX);
  str_cat(temp, file, PATH_MAX);

  return buf_write(temp, value);
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
static char * _slot_find(const char *enclo_path, const char *device_path)
{
  void *dir;
  char *temp, *result = NULL;

  dir = scan_dir(enclo_path);
  if (dir) {
    temp = list_first_that(dir, _slot_match, device_path);
    if (temp) {
      result = strdup(temp);
    }
    list_fini(dir);
  }
  return result;
}

/**
 */
int scsi_libsas_write(const char *sysfs_path, enum ibpi_pattern ibpi)
{
  if (sysfs_path == NULL) {
    __set_errno_and_return(EINVAL);
  }
  if ((ibpi < IBPI_PATTERN_NORMAL) || (ibpi > IBPI_PATTERN_LOCATE)) {
    __set_errno_and_return(ERANGE);
  }
  switch (ibpi) {
  case IBPI_PATTERN_ONESHOT_NORMAL:
  case IBPI_PATTERN_NORMAL:
    _ENCLOSURE_STATUS(sysfs_path, STATUS_OK);
    _ENCLOSURE_FAULT(sysfs_path, VALUE_OFF);
    _ENCLOSURE_LOCATE(sysfs_path, VALUE_OFF);
    break;
  case IBPI_PATTERN_FAILED_ARRAY:
    return _ENCLOSURE_STATUS(sysfs_path, STATUS_UNRECOVERABLE);
  case IBPI_PATTERN_DEGRADED:
    return _ENCLOSURE_STATUS(sysfs_path, STATUS_CRITICAL);
  case IBPI_PATTERN_REBUILD:
  case IBPI_PATTERN_REBUILD_P:
    return _ENCLOSURE_STATUS(sysfs_path, STATUS_NON_CRITICAL);
  case IBPI_PATTERN_FAILED_DRIVE:
    return _ENCLOSURE_FAULT(sysfs_path, VALUE_ON);
  case IBPI_PATTERN_LOCATE_OFF:
    return _ENCLOSURE_LOCATE(sysfs_path, VALUE_OFF);
  case IBPI_PATTERN_LOCATE:
    return _ENCLOSURE_LOCATE(sysfs_path, VALUE_ON);
  case IBPI_PATTERN_HOTSPARE:
    return _ENCLOSURE_STATUS(sysfs_path, STATUS_UNAVAILABLE);
  default:
    break;
  }
  return 0;
}

/**
 */
char *scsi_get_slot_path(const char *path)
{
  struct enclosure_device *device;
  char *result = NULL;

  device = sysfs_get_enclosure_devices();
  while (device) {
    if ((result = _slot_find(device->sysfs_path, path)) != NULL) {
      break;
    }
    device = list_next(device);
  }
  return result;
}
