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

#ifndef _SYSFS_H_INCLUDED_
#define _SYSFS_H_INCLUDED_

#include "status.h"

/**
 * @brief Initializes sysfs module.
 *
 * This function initializes sysfs module internal lists.
 * Application must call this function before any sysfs module function.
 */
void sysfs_init(void);

/**
 * @brief Resets the content of internal lists.
 *
 * This function releases memory allocated for elements of internal lists.
 */
void sysfs_reset(void);

/**
 * @brief Scans sysfs tree and populates internal lists.
 *
 * This function scans sysfs tree for storage controllers, block devices, RAID
 * devices, container devices, slave devices and enclosure devices registered
 * in the system. Only supported block and controller devices are put on a list.
 */
void sysfs_scan(void);

/**
 * The function returns list of enclosure devices attached to SAS/SCSI storage
 * controller(s).
 */
const struct list *sysfs_get_enclosure_devices(void);

/**
 * The function returns list of controller devices present in the system.
 */
const struct list *sysfs_get_cntrl_devices(void);

/**
 * The function returns list of RAID volumes present in the system.
 */
const struct list *sysfs_get_volumes(void);

/**
 * The function returns list of block devices present in the system.
 */
const struct list *sysfs_get_block_devices(void);

/**
 * The function returns list of pci slots present in the system.
 */
const struct list *sysfs_get_pci_slots(void);

/**
 * The function checks if the given storage controller is attached to enclosure
 * device(s).
 */
int sysfs_enclosure_attached_to_cntrl(const char *path);

/*
 * This function checks driver type.
 */
int sysfs_check_driver(const char *path, const char *driver);

#endif				/* _SYSFS_H_INCLUDED_ */
