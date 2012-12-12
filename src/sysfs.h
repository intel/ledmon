/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2012 Intel Corporation.
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

/**
 * @brief Initializes sysfs module.
 *
 * This function initializes sysfs module. The function allocates memory for
 * internal lists and initializes the lists. Application must call this function
 * before any sysfs module function.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t sysfs_init(void);

/**
 * @brief Finalizes sysfs module.
 *
 * This function releases memory allocated for sysfs module and its components.
 * Application must call this function at the very end of execution.
 *
 * @return The function does not return a value.
 */
void sysfs_fini(void);

/**
 * @brief Resets the content of internal lists.
 *
 * This function releases memory allocated for elements of internal lists. The
 * function does not release memory allocated for the lists itself. Use
 * sysfs_fini() function instead to release memory allocated for internal lists.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t sysfs_reset(void);

/**
 * @brief Scans sysfs tree and populates internal lists.
 *
 * This function scans sysfs tree for storage controllers, block devices, RAID
 * devices, container devices, slave devices and enclosure devices registered
 * in the system. Only supported block and controller devices are put on a list.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t sysfs_scan(void);

/**
 * The function returns list of enclosure devices attached to SAS/SCSI storage
 * controller(s).
 */
void *sysfs_get_enclosure_devices(void);

/**
 * The function returns list of controller devices present in the system.
 */
void *sysfs_get_cntrl_devices(void);

/**
 * The function checks if the given storage controller is attached to enclosure
 * device(s).
 */
int sysfs_enclosure_attached_to_cntrl(const char *path);

/**
 */
status_t sysfs_block_device_scan(void **block_list);

/**
 */
#define sysfs_block_device_for_each(__action) \
    __sysfs_block_device_for_each((action_t)(__action), (void *)0)

/**
 */
status_t __sysfs_block_device_for_each(action_t action, void *parm);

/**
 */
#define sysfs_block_device_first_that(__test, __parm) \
    __sysfs_block_device_first_that((test_t)(__test), (void *)(__parm))

/**
 */
void *__sysfs_block_device_first_that(test_t action, void *parm);

/*
 * This function checks if driver type is isci.
 */
int sysfs_isci_driver(const char *path);

#endif /* _SYSFS_H_INCLUDED_ */
