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

#ifndef _SYSFS_H_INCLUDED_
#define _SYSFS_H_INCLUDED_

#include <stdbool.h>
#include "list.h"
#include "status.h"

#define SYSFS_PCI_DEVICES	"/sys/bus/pci/devices"

#define SYSTEM_DEV_DIR		"/dev"

struct sysfs {
	/**
	 * This is internal variable global to sysfs module only. It is a list of
	 * block devices registered in the system. Use sysfs_init()
	 * function to initialize the variable. Use sysfs_scan() function to populate
	 * the list. Use sysfs_reset() function to delete the content of the list.
	 */
	struct list sysfs_block_list;

	/**
	 * This is internal variable global to sysfs module only. It is a list of
	 * RAID volumes registered in the system. Use sysfs_init()
	 * function to initialize the variable. Use sysfs_scan() function to populate
	 * the list. Use sysfs_reset() function to delete the content of the list.
	 */
	struct list volum_list;

	/**
	 * This is internal variable global to sysfs module only. It is a list of
	 * storage controller devices registered in the system and
	 * supported by Intel(R) Enclosure LEDs Control Utility. Use sysfs_init()
	 * function to initialize the variable. Use sysfs_scan() function to populate
	 * the list. Use sysfs_reset() function to delete the content of the list.
	 */
	struct list cntrl_list;

	/**
	 * This is internal variable global to sysfs module only. It is a list of
	 * tail devices registered in the system. Use sysfs_init()
	 * function to initialize the variable. Use sysfs_scan() function to populate
	 * the list. Use sysfs_reset() function to delete the content of the list.
	 */
	struct list tail_list;

	/**
	 * This is internal variable global to sysfs module only. It is a list of
	 * RAID containers registered in the system. Use sysfs_init()
	 * function to initialize the variable. Use sysfs_scan() function to populate
	 * the list. Use sysfs_reset() function to delete the content of the list.
	 */
	struct list cntnr_list;

	/**
	 * This is internal variable global to sysfs module only. It is a to list of
	 * enclosures registered in the system.
	 */
	struct list enclo_list;

	/**
	 * This is internal variable global to sysfs module only. It is a list of
	 * PCI slots registered in the system. Use sysfs_init()
	 * function to initialize the variable. Use sysfs_scan() function to populate
	 * the list. Use sysfs_reset() function to delete the content of the list.
	 */
	struct list pci_slots_list;

	/**
	 * This is internal variable global to sysfs module only. It is a list of
	 * PCI slots registered in the system. Use sysfs_init()
	 * function to initialize the variable. Use sysfs_scan() function to populate
	 * the list. Use sysfs_reset() function to delete the content of the list.
	 */
	struct list slots_list;

};


/**
 * @brief Initializes sysfs module.
 *
 * This function initializes sysfs module internal lists.
 * Application must call this function before any sysfs module function.
 */

void sysfs_init(struct led_ctx *ctx);

/**
 * @brief Resets the content of internal lists.
 *
 * This function releases memory allocated for elements of internal lists.
 */
void sysfs_reset(struct led_ctx *ctx);

/**
 * @brief Scans sysfs tree and populates internal lists.
 *
 * This function scans sysfs tree for storage controllers, block devices, RAID
 * devices, container devices, tail devices and enclosure devices registered
 * in the system. Only supported block and controller devices are put on a list.
 */
void sysfs_scan(struct led_ctx *ctx);

/**
 * The function returns list of enclosure devices attached to SAS/SCSI storage
 * controller(s).
 */
const struct list *sysfs_get_enclosure_devices(struct led_ctx *ctx);

/**
 * The function returns list of controller devices present in the system.
 */
const struct list *sysfs_get_cntrl_devices(struct led_ctx *ctx);

/**
 * The function returns list of RAID volumes present in the system.
 */
const struct list *sysfs_get_volumes(struct led_ctx *ctx);

/**
 * The function returns list of block devices present in the system.
 */
const struct list *sysfs_get_block_devices(struct led_ctx *ctx);

/**
 * The function returns list of pci slots present in the system.
 */
const struct list *sysfs_get_pci_slots(struct led_ctx *ctx);

/**
 * The function returns list of slots for supported controllers
 * present in the system.
 */
const struct list *sysfs_get_slots(struct led_ctx *ctx);

/**
 * The function checks if the given storage controller is attached to enclosure
 * device(s).
 */
int sysfs_enclosure_attached_to_cntrl(struct led_ctx *ctx, const char *path);

/*
 * This function checks driver type.
 */
int sysfs_check_driver(const char *path, const char *driver);

#endif				/* _SYSFS_H_INCLUDED_ */
