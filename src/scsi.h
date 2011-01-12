/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
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
char *scsi_get_slot_path(const char *path);

/**
 * @brief Sends message to SES processor of an enclosure.
 *
 * This function send a message to an enclosure in order to control LEDs of
 * the given slot/component. It uses interface of ENCLOSURE kernel module to
 * control LEDs.
 *
 * @param[in]      path           Path to an enclosure device in sysfs.
 * @param[in]      ibpi           IBPI pattern to visualize.
 *
 * @return Number of characters written if successful or -1 in case of error
 *         and errno is set to appropriate error code.
 */
int scsi_libsas_write(const char *path, enum ibpi_pattern ibpi);

#endif /* _SCSI_H_INCLUDED_ */
