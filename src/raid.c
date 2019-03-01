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
#include <stdlib.h>
#include <string.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "block.h"
#include "config.h"
#include "ibpi.h"
#include "list.h"
#include "raid.h"
#include "slave.h"
#include "status.h"
#include "sysfs.h"
#include "utils.h"

/**
 */
static enum raid_state _get_array_state(const char *path)
{
	enum raid_state state = RAID_STATE_UNKNOWN;

	char *p = get_text(path, "md/array_state");
	if (p) {
		if (strcmp(p, "clear") == 0)
			state = RAID_STATE_CLEAR;
		else if (strcmp(p, "inactive") == 0)
			state = RAID_STATE_INACTIVE;
		else if (strcmp(p, "suspended") == 0)
			state = RAID_STATE_SUSPENDED;
		else if (strcmp(p, "readonly") == 0)
			state = RAID_STATE_READONLY;
		else if (strcmp(p, "read-auto") == 0)
			state = RAID_STATE_READ_AUTO;
		else if (strcmp(p, "clean") == 0)
			state = RAID_STATE_CLEAN;
		else if (strcmp(p, "active") == 0)
			state = RAID_STATE_ACTIVE;
		else if (strcmp(p, "write-pending") == 0)
			state = RAID_STATE_WRITE_PENDING;
		else if (strcmp(p, "active-idle") == 0)
			state = RAID_STATE_ACTIVE_IDLE;
		free(p);
	}
	return state;
}

/**
 */
static enum raid_action _get_sync_action(const char *path)
{
	enum raid_action action = RAID_ACTION_UNKNOWN;

	char *p = get_text(path, "md/sync_action");
	if (p) {
		if (strcmp(p, "idle") == 0)
			action = RAID_ACTION_IDLE;
		else if (strcmp(p, "reshape") == 0)
			action = RAID_ACTION_RESHAPE;
		else if (strcmp(p, "frozen") == 0)
			action = RAID_ACTION_FROZEN;
		else if (strcmp(p, "resync") == 0)
			action = RAID_ACTION_RESYNC;
		else if (strcmp(p, "check") == 0)
			action = RAID_ACTION_CHECK;
		else if (strcmp(p, "recover") == 0)
			action = RAID_ACTION_RECOVER;
		else if (strcmp(p, "repair") == 0)
			action = RAID_ACTION_REPAIR;
		free(p);
	}
	return action;
}

/**
 */
static enum raid_level _get_level(const char *path)
{
	enum raid_level result = RAID_LEVEL_UNKNOWN;

	char *p = get_text(path, "md/level");
	if (p) {
		if (strcmp(p, "raid0") == 0)
			result = RAID_LEVEL_0;
		else if (strcmp(p, "raid1") == 0)
			result = RAID_LEVEL_1;
		else if (strcmp(p, "raid10") == 0)
			result = RAID_LEVEL_10;
		else if (strcmp(p, "raid4") == 0)
			result = RAID_LEVEL_4;
		else if (strcmp(p, "raid5") == 0)
			result = RAID_LEVEL_5;
		else if (strcmp(p, "raid6") == 0)
			result = RAID_LEVEL_6;
		else if (strcmp(p, "linear") == 0)
			result = RAID_LEVEL_LINEAR;
		else if (strcmp(p, "faulty") == 0)
			result = RAID_LEVEL_FAULTY;
		free(p);
	}
	return result;
}

/**
 */
struct raid_device *raid_device_init(const char *path, unsigned int device_num,
				     enum device_type type)
{
	struct raid_device *device = NULL;
	enum raid_state state;
	const char *debug_dev;

	state = _get_array_state(path);
	if (state > RAID_STATE_INACTIVE ||
	    (type == DEVICE_TYPE_CONTAINER && state > RAID_STATE_CLEAR)) {
		device = malloc(sizeof(struct raid_device));
		if (device) {
			device->sysfs_path = str_dup(path);
			device->device_num = device_num;
			device->sync_action = _get_sync_action(path);
			device->array_state = state;
			device->level = _get_level(path);
			device->degraded = get_int(path, -1, "md/degraded");
			device->raid_disks = get_int(path, 0, "md/raid_disks");
			device->type = type;
			debug_dev = strrchr(path, '/');
			debug_dev = debug_dev ? debug_dev + 1 : path;
			log_debug("(%s) path: %s, level=%d, state=%d, " \
					"degraded=%d, disks=%d, type=%d",
					__func__, debug_dev, device->level,
						state, device->degraded,
						device->raid_disks, type);
		}
	}
	return device;
}

/**
 */
void raid_device_fini(struct raid_device *device)
{
	if (device) {
		if (device->sysfs_path)
			free(device->sysfs_path);
		free(device);
	}
}

/**
 */
struct raid_device *raid_device_duplicate(struct raid_device *device)
{
	struct raid_device *new_device = NULL;

	if (device) {
		new_device = malloc(sizeof(struct raid_device));
		if (new_device) {
			*new_device = *device;
			new_device->sysfs_path = str_dup(device->sysfs_path);
		}
	}
	return new_device;
}

/**
 */
struct raid_device *find_raid_device(const struct list *raid_list,
				     char *raid_sysfs_path)
{
	struct raid_device *raid = NULL;

	list_for_each(raid_list, raid) {
		if (strcmp(raid->sysfs_path, raid_sysfs_path) == 0)
			return raid;
	}
	return NULL;
}
