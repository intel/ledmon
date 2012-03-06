/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009,2011, Intel Corporation.
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

#include <config.h>

#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "status.h"
#include "list.h"
#include "cntrl.h"
#include "utils.h"
#include "sysfs.h"

enum int_cntrl_type {
  INT_CNTRL_TYPE_UNKNOWN = CNTRL_TYPE_UNKNOWN,
  INT_CNTRL_TYPE_SCSI = CNTRL_TYPE_SCSI,
  INT_CNTRL_TYPE_AHCI = CNTRL_TYPE_AHCI,
  INT_CNTRL_TYPE_ISCI = CNTRL_TYPE_AHCI + 1,
  INT_CNTRL_TYPE_DELLSSD = CNTRL_TYPE_DELLSSD,
};

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
static int _is_isci_cntrl(const char *path)
{
  uint64_t class;

  if (sysfs_isci_driver(path) == 0)
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

static int _is_dellssd_cntrl(const char *path)
{
  return get_uint64(path, 0, "vendor") == 0x1344L &&
    get_uint64(path, 0, "device") == 0x5150L;
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
static enum int_cntrl_type _get_type(const char *path)
{
  enum int_cntrl_type type = CNTRL_TYPE_UNKNOWN;

  if (_is_dellssd_cntrl(path)) {
    type = CNTRL_TYPE_DELLSSD;
  }
  if (_is_scsi_cntrl(path)) {
    type = CNTRL_TYPE_SCSI;
  }
  if (_is_isci_cntrl(path)) {
    type = INT_CNTRL_TYPE_ISCI;
  }
  if (_is_ahci_cntrl(path)) {
    type = CNTRL_TYPE_AHCI;
  }
  return type;
}

struct _host_type *alloc_host(int id, struct _host_type *next)
{
  struct _host_type *host = NULL;
  host = malloc(sizeof(struct _host_type));
  if (host) {
    host->host_id = id;
    host->ibpi_state_buffer = NULL;
    host->next = next;
  }
  return host;
}

void free_hosts(struct _host_type *h)
{
  struct _host_type *t;
  while(h) {
    t = h->next;
    free(h->ibpi_state_buffer);
    free(h);
    h = t;
  }
}

void _find_host(const char *path, struct _host_type **hosts)
{
  const int host_len = sizeof("host") - 1;
  char *p;
  int index = -1;
  struct _host_type *th;

  p = strrchr(path, '/');
  if (!p++)
    return;
  if (strncmp(p, "host", host_len - 1) == 0) {
    index = atoi(p + host_len);
    th = alloc_host(index, (*hosts)?(*hosts):NULL);
    if (!th)
      return;
    *hosts = th;
  }
}

/**
 * @brief Get all instances of separate hosts on isci controller.
 *
 */
static struct _host_type *_cntrl_get_hosts(const char *path)
{
  struct _host_type *hosts = NULL;
  void *dir = scan_dir(path);
  if (dir) {
    list_for_each_parm(dir, _find_host, &hosts);
    list_fini(dir);
  }
  return hosts;
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
  /* first, open ...driver/module/parameters/ahci_em_messages
   * then open /sys/module/libahci/parameters/ahci_em_messages
   * and check if it is set to enabled.
   * if so, check if 'holders' of libahci points the same driver name
   * as device given by path
   */
  char buf[PATH_MAX];
  char *link, *name;
  DIR *dh;
  struct dirent *de = NULL;

  /* old kernel (prior to 2.6.36) */
  if (get_int(path, 0, "driver/module/parameters/ahci_em_messages") != 0);
      return 1;

  if (!get_int("", 0, "sys/module/libahci/parameters/ahci_em_messages"))
    return 0;

  if (snprintf(buf, sizeof(buf), "%s/%s", path, "driver") < 0) {
    return 0;
  }

  link = realpath(buf, NULL);
  if (!link) {
    return 0;
  }

  name = strrchr(link, '/');
  if (!name++) {
    free(link);
    return 0;
  }

  /* name contain controller name (ie. ahci), so check if libahci holds this driver */
  dh = opendir("/sys/module/libahci/holders");
  if (dh) {
    while ( (de = readdir(dh)) ) {
      if (!strcmp(de->d_name, name)) {
        break;
      }
    }
    closedir(dh);
  }

  free(link);
  return de ? 1 : 0;
}

/*
 * Allocates memory for a new controller device structure. See cntrl.h for
 * details.
 */
struct cntrl_device * cntrl_device_init(const char *path)
{
  unsigned int em_enabled;
  enum int_cntrl_type type;
  struct cntrl_device *device = NULL;

  type = _get_type(path);
  if (type != INT_CNTRL_TYPE_UNKNOWN) {
    switch (type) {
    case INT_CNTRL_TYPE_DELLSSD:
    case INT_CNTRL_TYPE_ISCI:
    case INT_CNTRL_TYPE_SCSI:
      em_enabled = 1;
      break;
    case INT_CNTRL_TYPE_AHCI:
      em_enabled = _ahci_em_messages(path);
      break;
    default:
      em_enabled = 0;
    }
    if (em_enabled) {
      device = malloc(sizeof(struct cntrl_device));
      if (device) {
        if (type == INT_CNTRL_TYPE_ISCI) {
          device->isci_present = 1;
          device->hosts = _cntrl_get_hosts(path);
          type = CNTRL_TYPE_SCSI;
        } else {
          device->isci_present = 0;
          device->hosts = NULL;
        }
        device->cntrl_type = (enum cntrl_type)type;
        device->sysfs_path = strdup(path);
      }
    } else {
      log_error("controller discovery: %s - enclosure management not " \
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
    free(device->sysfs_path);
    free_hosts(device->hosts);
  }
}
