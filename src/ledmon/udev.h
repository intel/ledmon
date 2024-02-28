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

#ifndef _UDEV_H_INCLUDED_
#define _UDEV_H_INCLUDED_

#include <lib/list.h>
#include "sysfs.h"

/**
 */
enum udev_action {
	UDEV_ACTION_UNKNOWN = 0,
	UDEV_ACTION_ADD,
	UDEV_ACTION_REMOVE
};

/**
 * @brief Deletes udev context and udev monitor.
 *
 * @return The function does not return a value..
 */
void stop_udev_monitor(void);

/**
 * @brief Returns udev monitor file descriptor or creates udev context and
 *        udev monitor if monitor does not exist.
 *
 * @return Udev monitor file descriptor if successful, otherwise the
 *         function returns -1 on libudev error and libudev sets errno.
 */
int get_udev_monitor(void);

/**
 * @brief Handles udev event.
 *
 *        This function checks event type and if it is 'add' or remove
 *        function sets custom IBPI pattern to block device which is affected
 *        by this event.
 *
 * @param[in]    ledmon_block_list    list containing block devices, it is
 *                                    used to match device from udev event.
 * @param[in]    ctx                  Library context.
 *
 * @return 0 if 'add' or 'remove' event handled successfully;
 *         1 if registered event is not 'add' or 'remove';
 *         -1 on libudev error.
 */
int handle_udev_event(struct list *ledmon_block_list, struct led_ctx *ctx);

#endif                         /* _UDEV_H_INCLUDED_ */
