/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2017-2019 Intel Corporation.
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
#include "status.h"
#include "sysfs.h"
#include "udev.h"
#include "utils.h"

static struct udev_monitor *udev_monitor;

static int _compare(const struct block_device *bd, const char *syspath)
{
	if (!bd || !syspath)
		return 0;

	if (strcmp(bd->sysfs_path, syspath) == 0) {
		return 1;
	} else {
		struct block_device *bd_new;
		int ret;

		bd_new = block_device_init(sysfs_get_cntrl_devices(), syspath);
		if (!bd_new)
			return 0;

		ret = block_compare(bd, bd_new);
		block_device_fini(bd_new);

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

int get_udev_monitor(void)
{
	if (udev_monitor)
		return udev_monitor_get_fd(udev_monitor);

	return create_udev_monitor();
}

static int _check_raid(const char *path)
{
	char *t = strrchr(path, '/');

	if (t == NULL)
		return 0;
	return strncmp(t + 1, "md", 2) == 0;
}

static enum udev_action _get_udev_action(const char *action)
{
	enum udev_action ret = UDEV_ACTION_UNKNOWN;

	if (strncmp(action, "add", 3) == 0)
		ret = UDEV_ACTION_ADD;
	else if (strncmp(action, "remove", 6) == 0)
		ret = UDEV_ACTION_REMOVE;
	return ret;
}

static void _clear_raid_dev_info(struct block_device *block, char *raid_dev)
{
	if (block->raid_dev && block->raid_dev->sysfs_path) {
		char *tmp = strrchr(block->raid_dev->sysfs_path, '/');

		if (tmp == NULL) {
			log_debug("Device: %s have wrong raid_dev path: %s",
				block->sysfs_path,
				block->raid_dev->sysfs_path);
			return;
		}
		if (strcmp(raid_dev, tmp + 1) == 0) {
			log_debug("CLEAR raid_dev %s in %s ",
				  raid_dev, block->sysfs_path);
			raid_device_fini(block->raid_dev);
			block->raid_dev = NULL;
		}
	}

}

int handle_udev_event(struct list *ledmon_block_list)
{
	struct udev_device *dev;
	int status = -1;

	dev = udev_monitor_receive_device(udev_monitor);
	if (dev) {
		const char *action = udev_device_get_action(dev);
		enum udev_action act = _get_udev_action(action);
		const char *syspath = udev_device_get_syspath(dev);
		struct block_device *block = NULL;

		if (act == UDEV_ACTION_UNKNOWN) {
			status = 1;
			goto exit;
		}

		list_for_each(ledmon_block_list, block) {
			if (_compare(block, syspath))
				break;
			block = NULL;
		}

		if (!block) {
			if (act == UDEV_ACTION_REMOVE && _check_raid(syspath)) {
				/*ledmon is interested about removed arrays*/
				char *dev_name;

				dev_name = strrchr(syspath, '/') + 1;
				log_debug("REMOVED %s", dev_name);
				list_for_each(ledmon_block_list, block)
					_clear_raid_dev_info(block, dev_name);
				status = 0;
				goto exit;
			}
			status = 1;
			goto exit;
		}

		if (act == UDEV_ACTION_ADD) {
			log_debug("ADDED %s", block->sysfs_path);
			if (block->ibpi == IBPI_PATTERN_FAILED_DRIVE ||
				block->ibpi == IBPI_PATTERN_REMOVED ||
				block->ibpi == IBPI_PATTERN_UNKNOWN)
				block->ibpi = IBPI_PATTERN_ADDED;
		} else if (act == UDEV_ACTION_REMOVE) {
			log_debug("REMOVED %s", block->sysfs_path);
			block->ibpi = IBPI_PATTERN_REMOVED;
		} else {
			/* not interesting event */
			status = 1;
			goto exit;
		}
		status = 0;
	} else {
		return -1;
	}

exit:
	udev_device_unref(dev);
	return status;
}
