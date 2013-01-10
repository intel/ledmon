/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2013 Intel Corporation.
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
#include <stdlib.h>
#include <stdint.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "status.h"
#include "ibpi.h"
#include "utils.h"
#include "list.h"
#include "sysfs.h"
#include "block.h"
#include "slave.h"

/**
 */
static unsigned char _get_state(const char *path)
{
  char *p, *t, *s;
  unsigned char result = SLAVE_STATE_UNKNOWN;

  s = p = get_text(path, "state");
  if (p) {
    while (s) {
      t = strchr(s, ',');
      if (t) {
        *(t++) = '\0';
      }
      if (strcmp(s, "spare") == 0) {
        result |= SLAVE_STATE_SPARE;
      } else if (strcmp(s, "in_sync") == 0) {
        result |= SLAVE_STATE_IN_SYNC;
      } else if (strcmp(s, "faulty") == 0) {
        result |= SLAVE_STATE_FAULTY;
      } else if (strcmp(s, "write_mostly") == 0) {
        result |= SLAVE_STATE_WRITE_MOSTLY;
      } else if (strcmp(s, "blocked") == 0) {
        result |= SLAVE_STATE_BLOCKED;
      }
      s = t;
    }
    free(p);
  }
  return result;
}

/**
 */
static unsigned int _get_errors(const char *path)
{
  return get_int(path, 0, "errors");
}

/**
 */
static unsigned int _get_slot(const char *path)
{
  unsigned int result = -1;

  char *p = get_text(path, "slot");
  if (p) {
    if (strcmp(p, "none") != 0) {
      result = atoi(p);
    }
    free(p);
  }
  return result;
}

/**
 */
static int _compare(struct block_device *device, const char *path)
{
  return (strcmp(device->sysfs_path, path) == 0);
}

/**
 */
static struct block_device *_get_block(const char *path, void *block_list)
{
  char temp[PATH_MAX];
  char link[PATH_MAX];
  struct block_device *device = NULL;

  str_cpy(temp, path, PATH_MAX);
  str_cat(temp, "/block", PATH_MAX);

  if (realpath(temp, link)) {
    device = list_first_that(block_list, _compare, link);
  }
  return device;
}

/**
 */
struct slave_device * slave_device_init(const char *path, void *block_list)
{
  struct slave_device *device = NULL;
  struct block_device *block;

  block = _get_block(path, block_list);
  if (block) {
    device = malloc(sizeof(struct slave_device));
    if (device) {
      device->raid = NULL;
      device->state = _get_state(path);
      device->slot = _get_slot(path);
      device->errors = _get_errors(path);
      device->block = block;
    }
  }
  return device;
}

/**
 */
void slave_device_fini(struct slave_device *device)
{
  (void)device;
  /* Function reserved for future improvements. */
}
