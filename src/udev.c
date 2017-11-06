/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2017 Intel Corporation.
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

#include <libudev.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "block.h"
#include "ibpi.h"
#include "list.h"
#include "status.h"
#include "sysfs.h"
#include "udev.h"
#include "utils.h"

static struct udev_monitor *udev_monitor;

static int _compare(struct block_device *bd, const char *syspath)
{
	if (!bd || !syspath)
		return 0;

	if (strcmp(bd->sysfs_path, syspath) == 0) {
		return 1;
	} else {
		void *cntrl_list;
		struct block_device *bd_new;
		int ret;

		cntrl_list = sysfs_get_cntrl_devices();
		if (!cntrl_list)
			return 0;

		bd_new = block_device_init(cntrl_list, syspath);
		if (!bd_new)
			return 0;

		ret = block_compare(bd, bd_new);
		block_device_fini(bd_new);
		free(bd_new);

		return ret;
	}
}

static int create_udev_monitor(void)
{
	int res;
	struct udev *udev = udev_new();

	if (!udev) {
		log_error("Failed to create udev context instance.");
		return -1;
	}

	udev_monitor = udev_monitor_new_from_netlink(udev, "udev");
	if (!udev_monitor) {
		log_error("Failed to create udev monitor object.");
		udev_unref(udev);
		return -1;
	}

	res = udev_monitor_filter_add_match_subsystem_devtype(udev_monitor,
							      "block", "disk");
	if (res < 0) {
		log_error("Failed to modify udev monitor filters.");
		stop_udev_monitor();
		return -1;
	}

	res = udev_monitor_enable_receiving(udev_monitor);
	if (res < 0) {
		log_error("Failed to switch udev monitor to listening mode.");
		stop_udev_monitor();
		return -1;
	}

	return udev_monitor_get_fd(udev_monitor);
}

void stop_udev_monitor(void)
{
	if (udev_monitor) {
		struct udev *udev = udev_monitor_get_udev(udev_monitor);

		udev_monitor_unref(udev_monitor);

		if (udev)
			udev_unref(udev);
	}
}

int get_udev_monitor()
{
	if (udev_monitor)
		return udev_monitor_get_fd(udev_monitor);

	return create_udev_monitor();
}

int handle_udev_event(void *ledmon_block_list)
{
	struct udev_device *dev;

	dev = udev_monitor_receive_device(udev_monitor);
	if (dev) {
		const char *action = udev_device_get_action(dev);
		const char *syspath = udev_device_get_syspath(dev);
		struct block_device *block =
			list_first_that(ledmon_block_list, _compare, syspath);

		if (!block) {
			/* ignore - device is new or ledmon is not interested about it */
			return 0;
		}
		if (strncmp(action, "add", 3) == 0) {
			log_debug("ADDED %s", block->sysfs_path);
			block->ibpi = IBPI_PATTERN_ADDED;
		} else if (strncmp(action, "remove", 6) == 0) {
			log_debug("REMOVED %s", block->sysfs_path);
			block->ibpi = IBPI_PATTERN_REMOVED;
		} else {
			/* not interesting event */
			return 1;
		}
		return 0;
	} else {
		return -1;
	}
}
