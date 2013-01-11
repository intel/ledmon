/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2013 Intel Corporation.
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
 * @brief Gets a path to slot of sas controller.
 *
 * This function returns a sysfs path to component of enclosure the device
 * belongs to.
 *
 * @param[in]      path           Canonical sysfs path to block device.
 *
 * @return A sysfs path to controller device associated with the given
 *         block device if successful, otherwise NULL pointer.
 */
char *sas_get_slot_path(const char *path, const char *ctrl_path);

/**
 * @brief Sends message to SES processor of an enclosure.
 *
 * This function send a message to an enclosure in order to control LEDs of
 * the given slot/component. It uses interface of ENCLOSURE kernel module to
 * control LEDs.
 *
 * @param[in]      device         Path to an enclosure device in sysfs.
 * @param[in]      ibpi           IBPI pattern to visualize.
 *
 * @return Number of characters written if successful or -1 in case of error
 *         and errno is set to appropriate error code.
 */
int scsi_ses_write(struct block_device *device, enum ibpi_pattern ibpi);

/**
 * @brief Fills encl_index and encl_dev.
 *
 * @param[in]      device        Path to block device.
 *
 * @return 1 on success, 0 otherwise.
 * */
int scsi_get_enclosure(struct block_device *device);

#endif				/* _SCSI_H_INCLUDED_ */
