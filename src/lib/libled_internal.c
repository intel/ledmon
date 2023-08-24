/*
 * LED library
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

#include <string.h>
#include <stdarg.h>

#include "list.h"
#include "libled_private.h"
#include "utils.h"

static led_status_t list_add(struct led_ctx *ctx, struct list *l, const char *path)
{
	char *t = NULL;

	if (!ctx || !path)
		return LED_STATUS_NULL_POINTER;

	t = strdup(path);
	if (!t || !list_append(l, t)) {
		free(t);
		return LED_STATUS_OUT_OF_MEMORY;
	}
	return LED_STATUS_SUCCESS;
}

led_status_t device_allow_pattern_add(struct led_ctx *ctx, const char *path)
{
	return list_add(ctx, &ctx->config.allowlist, path);
}

led_status_t device_exclude_pattern_add(struct led_ctx *ctx, const char *path)
{
	return list_add(ctx, &ctx->config.excludelist, path);
}

void lib_log(struct led_ctx *ctx, enum led_log_level_enum loglevel, const char *buf, ...)
{
	va_list vl;

	if (!ctx || ctx->log_fd < 0)
		return;

	va_start(vl, buf);
	_common_log(ctx->log_fd, ctx->log_lvl, loglevel, buf, vl);
	va_end(vl);
}

void off_all(struct led_ctx *ctx)
{
	struct block_device *device;

	list_for_each(sysfs_get_block_devices(ctx), device) {
		device->send_fn(device, LED_IBPI_PATTERN_LOCATE_OFF);
		device->flush_fn(device);
	}
}

led_status_t device_blink_behavior_set(struct led_ctx *ctx, int migration,
						      int init, int rebuild_all, int raid_members)
{
	if (!ctx)
		return LED_STATUS_NULL_POINTER;

	ctx->config.blink_on_init = init;
	ctx->config.blink_on_migration = migration;
	ctx->config.rebuild_blink_on_all = rebuild_all;
	ctx->config.raid_members_only = raid_members;
	return LED_STATUS_SUCCESS;
}
