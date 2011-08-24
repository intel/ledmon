/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009,2011 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _BLOCK_H_INCLUDED_
#define _BLOCK_H_INCLUDED_

struct block_device;

/**
 * @brief Pointer to a send message function.
 *
 * The pointer to a function which knows how to send LED message to a driver of
 * storage controller using the given protocol.
 *
 * @param[in]    path             path in sysfs to a host device
 *                                @see block_device::cntrl_path.
 * @param[in]    ibpi             an IBPI pattern (state) to visualize.
 *
 * @return 1 if successful, otherwise the function returns 0.
 */
typedef int (* send_message_t)(struct block_device *device, enum ibpi_pattern ibpi);

/**
 * @brief Describes a block device.
 *
 * This structure describes a block device. It does not describe virtual devices 
 * or partitions on physical block devices.
 */
struct block_device {
  /**
   * Real path in sysfs tree. This means i.e. if /sys/block/sda is symbolic link
   * then the link will be read and path stored in sysfs_path field. This path
   * may not exist in sysfs if connection to physical drive is lost. This filed
   * cannot have NULL pointer assigned.
   */
  char *sysfs_path;

  /**
   * The pointer to a function which sends a message to driver in order to
   * control LEDs in an enclosure or DAS system - @see send_message_t for details.
   * This field cannot have NULL pointer assigned.
   */
  send_message_t send_fn;

  /**
   * Canonical path to block device where enclosure management fields are located.
   * This path is always accessible even if the connection to physical device is lost.
   * In case of AHCI controller it points to SATA phy. In case of SAS this path
   * points to SES entry associated with the slot in an enclosure. This field cannot
   * have NULL pointer assign.
   */
  char *cntrl_path;

  /**
   * The current state of block device. This is an IBPI pattern and it is used
   * to visualize the state of block device.
   */
  enum ibpi_pattern ibpi;

  /**
   * The time stamp used to determine if the given block device still exist or
   * it failed and the device is no longer available. Every time IBPI pattern 
   * is updated, the time-stamp is updated, too.
   */
  time_t timestamp;

  /**
   * The pointer to storage controller structure the device is connected to.
   */
  struct cntrl_device *cntrl;

	/**
   * The index of phy utilized by directly attached to controller block device.
   * It is meaningful if device is controlled by isci driver.
   */
  int phy_index;
};

/**
 * @brief Creates a block device structure.
 *
 * This function allocates memory for a new structure of block device. It reads 
 * the sysfs entries and populates the structure fields. It performs all this
 * actions only if the block device is connected to the one of supported storage
 * controllers and the controller has enclosure management services enabled.
 *
 * @param[in]      cntrl_list     pointer to a list of supported controller 
 *                                devices.
 * @param[in]      sysfs_path     a path to block device in sysfs.
 *
 * @return Pointer to block device structure if successful, otherwise the function
 *         returns the NULL pointer.
 */
struct block_device * block_device_init(void *cntrl_list, const char *path);

/**
 * @brief Releases a block device structure.
 *
 * This function releases memory allocated for block device structure. To be more
 * specific it only frees memory allocated for the structure fields. It is due to
 * the way list is implemented for the purpose of this utility. If one would like
 * release block device structure not stored as a list node it must call free()
 * explicitly just after function block_device_fini() is called.
 *
 * @param[in]      device         pointer to block device structure.
 *
 * @return The function does not return a value.
 */
void block_device_fini(struct block_device *device);

/**
 * @brief Duplicates a block device structure.
 *
 * The function allocates memory for a block device structure and copies values
 * stored in fields of source block structure. The function allocates new memory
 * for all string fields in a copy structure. It is safe to release source block
 * structure just after it has been duplicated.
 *
 * @param[in]      device         pointer to source block device structure.
 *
 * @return Pointer to block device structure if successful, otherwise the function
 *         returns the NULL pointer.
 */
struct block_device * block_device_duplicate(struct block_device *device);

/**
 * The global timestamp variable. It is updated every time the sysfs is scanning
 * by an application. The value stored in this variable should be used to update
 * all timestamp stored in block device structures.
 */
extern time_t timestamp;

#endif /* _BLOCK_H_INCLUDED_ */
