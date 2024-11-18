// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 Intel Corporation.

#ifndef _RAID_H_INCLUDED_
#define _RAID_H_INCLUDED_

/**
 */
enum raid_state {
	RAID_STATE_UNKNOWN = 0,
	RAID_STATE_CLEAR,
	RAID_STATE_INACTIVE,
	RAID_STATE_SUSPENDED,
	RAID_STATE_READONLY,
	RAID_STATE_READ_AUTO,
	RAID_STATE_CLEAN,
	RAID_STATE_ACTIVE,
	RAID_STATE_WRITE_PENDING,
	RAID_STATE_ACTIVE_IDLE
};

/**
 */
enum raid_level {
	RAID_LEVEL_UNKNOWN = 0,
	RAID_LEVEL_0,
	RAID_LEVEL_1,
	RAID_LEVEL_10,
	RAID_LEVEL_4,
	RAID_LEVEL_5,
	RAID_LEVEL_6,
	RAID_LEVEL_FAULTY,
	RAID_LEVEL_LINEAR
};

/**
 */
enum device_type {
	DEVICE_TYPE_UNKNOWN = 0,
	DEVICE_TYPE_VOLUME,
	DEVICE_TYPE_CONTAINER
};

/**
 */
enum raid_action {
	RAID_ACTION_UNKNOWN = 0,
	RAID_ACTION_IDLE,
	RAID_ACTION_RESHAPE,
	RAID_ACTION_FROZEN,
	RAID_ACTION_RESYNC,
	RAID_ACTION_CHECK,
	RAID_ACTION_RECOVER,
	RAID_ACTION_REPAIR
};

/**
 */
struct raid_device {
	enum device_type type;
	int device_num;
	char sysfs_path[PATH_MAX];
	int raid_disks;
	int degraded;
	enum raid_state array_state;
	enum raid_action sync_action;
	enum raid_level level;
};

/**
 */
struct raid_device *raid_device_init(const char *path, unsigned int device_num,
				     enum device_type type, struct led_ctx *ctx);

/**
 */
void raid_device_fini(struct raid_device *device);

/**
 */
struct raid_device *raid_device_duplicate(struct raid_device *device);

/**
 */
struct raid_device *find_raid_device(const struct list *raid_list,
				     char *raid_sysfs_path);
#endif				/* _RAID_H_INCLUDED_ */
