/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2018 Intel Corporation.
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

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "config.h"
#include "ibpi.h"
#include "list.h"
#include "slave.h"
#include "status.h"
#include "sysfs.h"
#include "utils.h"

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
			if (t)
				*(t++) = '\0';
			if (strcmp(s, "spare") == 0)
				result |= SLAVE_STATE_SPARE;
			else if (strcmp(s, "in_sync") == 0)
				result |= SLAVE_STATE_IN_SYNC;
			else if (strcmp(s, "faulty") == 0)
				result |= SLAVE_STATE_FAULTY;
			else if (strcmp(s, "write_mostly") == 0)
				result |= SLAVE_STATE_WRITE_MOSTLY;
			else if (strcmp(s, "blocked") == 0)
				result |= SLAVE_STATE_BLOCKED;
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
		if (strcmp(p, "none") != 0)
			result = atoi(p);
		free(p);
	}
	return result;
}

/**
 */
static struct block_device *_get_block(const char *path, struct list *block_list)
{
	char temp[PATH_MAX];
	char link[PATH_MAX];
	struct block_device *device;

	snprintf(temp, sizeof(temp), "%s/%s", path, "block");

	if (!realpath(temp, link))
		return NULL;

	/* translate partition to master block dev */
	if (snprintf(temp, PATH_MAX, "%s/partition", link) > 0) {
		struct stat sb;
		char *ptr;

		if (stat(temp, &sb) == 0 && S_ISREG(sb.st_mode)) {
			ptr = strrchr(link, '/');
			if (ptr)
				*ptr = '\0';
		}
	}

	list_for_each(block_list, device) {
		if (strcmp(device->sysfs_path, link) == 0)
			return device;
	}
	return NULL;
}

/**
 */
struct slave_device *slave_device_init(const char *path, struct list *block_list)
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
	free(device);
}
