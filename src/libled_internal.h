/*
 * LED library internal
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


/*
 * NOTE: This are functions that are used by ledctl and ledmon,
 * but are not part of the public API
 */

#ifndef _LIB_LED_INTERNAL_INCLUDED_
#define _LIB_LED_INTERNAL_INCLUDED_

#include "led/libled.h"

/**
 * @brief Turns off all LEDs for all support hardware for the system
 *
 * @param[in] ctx	Library context
 *
 * Note: Needs to be followed with led_flush()
 */
void off_all(struct led_ctx *ctx);

/**
 * @brief Sets library blink behavior.
 *
 * @param[in]	ctx	Library context
 * @param[in]	migration
 * @param[in]	init
 * @param[in]	all
 * @param[in]	raid_members
 * @return led_status_t LED_STATUS_SUCCESS on success, else error reason.
 */
led_status_t device_blink_behavior_set(struct led_ctx *ctx, int migration,
						int init, int all, int raid_members);

/**
 * @brief Add a device path to an allow list.  The library will then only consider this device
 * for API calls.  Function can be repeated, to add to an internal list.
 *
 * @param[in]	ctx	Library context
 * @param]in]	path	Device path in allow
 * @return led_status_t	LED_STATUS_SUCCESS on success, else error reason.
 *
 * Note: Mutually exclusive with exclude, allow takes priority.
 */
led_status_t device_allow_pattern_add(struct led_ctx *ctx, const char *path);

/**
 * @brief Add a device path to an exclude list.  The library will then exclude this device
 * from API calls.  Function can be repeated, to add to an internal list.
 *
 * @param[in]	ctx	Library context
 * @param[in]	path	Device path to exclude
 * @return led_status_t LED_STATUS_SUCCESS on success, else error reason.
 */
led_status_t device_exclude_pattern_add(struct led_ctx *ctx, const char *path);

/**
 * @brief Log to the library context fd with the specified log level and message
 *
 * @param[in]	ctx		Library context
 * @param[in]	loglevel	Enumerated log level
 * @param[in]	buf		printf style format string
 * @param[in]	...		Variable number of arguments for printf format string
 */
void lib_log(struct led_ctx *ctx, enum led_log_level_enum loglevel, const char *buf, ...)
				__attribute__ ((format (printf, 3, 4)));
#endif
