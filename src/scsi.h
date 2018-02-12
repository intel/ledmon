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

#ifndef _SCSI_H_INCLUDED
#define _SCSI_H_INCLUDED

#include "block.h"
#include "ibpi.h"

/**
 * @brief Gets a path to slot of an enclosure.
 *
 * This function returns a sysfs path to component of enclosure the device
 * belongs to.
 *
 * @param[in]      path           Canonical sysfs path to block device.
 *
 * @return A sysfs path to enclosure's component associated with the given
 *         block device if successful, otherwise NULL pointer.
 */
char *scsi_get_slot_path(const char *path, const char *cntrl_path);

/**
 * @brief Prepares SES message based on ibpi pattern.
 *
 * @param[in]      device         Sysfs path of a drive in enclosure.
 * @param[in]      ibpi           IBPI pattern to visualize.
 *
 * @return 0 on success, otherwise error.
 */
int scsi_ses_write(struct block_device *device, enum ibpi_pattern ibpi);

/**
 * @brief Sends message to SES processor of an enclosure.
 *
 * This function send a message to an enclosure in order to control LEDs of
 * the given slot/component. It uses interface of ENCLOSURE kernel module to
 * control LEDs.
 *
 * @param[in]      device         Sysfs path of a drive in enclosure.
 *
 * @return 0 on success, otherwise error.
 */
int scsi_ses_flush(struct block_device *device);

/**
 * @brief Assigns enclosure device to block device.
 *
 * @param[in]      device        Path to block device.
 *
 * @return 1 on success, 0 otherwise.
 * */
int scsi_get_enclosure(struct block_device *device);

#endif				/* _SCSI_H_INCLUDED_ */
