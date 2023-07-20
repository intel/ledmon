/*
 * LED library private
 *
 * Copyright (C) 2022-2023 Red Hat, Inc.
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

/* Note: This file "libled_private.h" should never be included in a header file! */

#ifndef _LIB_LED_PRIVATE_INCLUDED_
#define _LIB_LED_PRIVATE_INCLUDED_

#include "led/libled.h"
#include "sysfs.h"
#include "list.h"
#include "amd_sgpio.h"
#include "slot.h"
#include "libled_internal.h"

#include <linux/limits.h>

/**
 * @brief Embedded structure within the library context used for amd state and caching.
 */
struct amd_sgpio_state {
	int cache_fd;
	struct cache_entry *cache;
};

/**
 * @brief Configuration options for the library.  Mainly used by ledmon.
 */
struct configuration {
	int blink_on_migration;
	int blink_on_init;
	int rebuild_blink_on_all;
	int raid_members_only;

	struct list allowlist;
	struct list excludelist;
};

/**
 * @brief Implementation of the opaque led_slot_list_entry.
 */
struct led_slot_list_entry {
	struct slot_property *slot;
	char device_name[PATH_MAX];
};

/**
 * @brief Implementation of the opaque led_slot_list.
 */
struct led_slot_list {
	struct list slot_list;
	struct node *iter;
};

/**
 * @brief Implementation of the opaque led_cntrl_list_entry.
 */
struct led_cntrl_list_entry {
	char path[PATH_MAX];
	enum led_cntrl_type cntrl_type;
};

/**
 * @brief Implementation of the opaque led_cntrl_list.
 */
struct led_cntrl_list {
	struct list cntrl_list;
	struct node *iter;
};

/**
 * @brief Library context
 *
 * This structure is the common data for the library context.  This is an opaque data type and thus
 * the internals of this structure cannot be visible to library users.
 */
struct led_ctx {
	struct sysfs sys;
	int log_fd;
	enum led_log_level_enum log_lvl;
	led_status_t deferred_error;
	int dellssd_hw_gen;	/* cached value for dellssd hardware generation */
	long ipmi_msgid;	/* incrementing message id */
	struct amd_sgpio_state amd_sgpio;
	struct configuration config;

	struct led_cntrl_list cl;
	struct led_slot_list sl;
};

#endif
