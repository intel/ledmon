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
#include "list.h"
#include "tail.h"
#include "status.h"
#include "sysfs.h"
#include "utils.h"

/**
 */
static unsigned char _get_state(const char *path)
{
	char *p, *t, *s;
	unsigned char result = TAIL_STATE_UNKNOWN;

	s = p = get_text(path, "state");
	if (p) {
		while (s) {
			t = strchr(s, ',');
			if (t)
				*(t++) = '\0';
			if (strcmp(s, "spare") == 0)
				result |= TAIL_STATE_SPARE;
			else if (strcmp(s, "in_sync") == 0)
				result |= TAIL_STATE_IN_SYNC;
			else if (strcmp(s, "faulty") == 0)
				result |= TAIL_STATE_FAULTY;
			else if (strcmp(s, "write_mostly") == 0)
				result |= TAIL_STATE_WRITE_MOSTLY;
			else if (strcmp(s, "blocked") == 0)
				result |= TAIL_STATE_BLOCKED;
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
static int _get_slot(const char *path, unsigned int *dest)
{
	int ret = 1;
	unsigned int n;
	char *p = get_text(path, "slot");

	if (p) {
		if (strcmp(p, "none") != 0)
			if (str_toui(&n, p, NULL, 10) == 0) {
				*dest = n;
				ret = 0;
			}
		free(p);
	}
	return ret;
}

/**
 */
static struct block_device *_get_block(const char *path, struct list *block_list)
{
	char temp[PATH_MAX];
	char link[PATH_MAX];
	struct block_device *device;

	snprintf(temp, sizeof(temp), "%s/block", path);

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
struct tail_device *tail_device_init(const char *path, struct list *block_list)
{
	struct tail_device *device = NULL;
	struct block_device *block;

	block = _get_block(path, block_list);
	if (block) {
		device = malloc(sizeof(struct tail_device));
		if (device && _get_slot(path, &device->slot) == 0) {
			device->raid = NULL;
			device->state = _get_state(path);
			device->errors = _get_errors(path);
			device->block = block;
		} else {
			free(device);
			device = NULL;
		}
	}
	return device;
}

/**
 */
void tail_device_fini(struct tail_device *device)
{
	free(device);
}
