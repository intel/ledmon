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

#ifndef _ENCLOSURE_H_INCLUDED_
#define _ENCLOSURE_H_INCLUDED_

/**
 * @brief Enclosure device structure.
 *
 * This structure describes an enclosure device connected to one of
 * SAS controllers existing in the system.
 */
struct enclosure_device {
  /**
   * Path to an enclosure device in sysfs tree. This is controller base
   * canonical path.
   */
	char *sysfs_path;

  /**
   * SAS address as identifier of an enclosure.
   */
	uint64_t sas_address;
};

/**
 * @brief Allocates memory for an enclosure device structure.
 *
 * This function allocates memory for a new structure describing an enclosure
 * device. It reads the sysfs entries and populates structure fields.
 * The function uses libsas abstraction layer to extract required information.
 *
 * @param[in]      path           Path to an enclosure device in sysfs tree.
 *                                The path begins with "/sys/class/enclosure/".
 *
 * @return Pointer to enclosure device structure if successful, otherwise the
 *         function returns NULL pointer. The NULL pointer means either the
 *         specified path is invalid or there's not enough memory in the system
 *         to allocated new structure.
 */
struct enclosure_device *enclosure_device_init(const char *path);

/**
 * @brief Releases an enclosure device structure.
 *
 * This function releases memory allocated for enclosure device structure. To be
 * more specific it only frees memory allocated for the fields of the structure.
 * It is due to the way list is implemented for the purpose of this utility.
 *
 * @param[in]      device         Pointer to enclosure device structure.
 *
 * @return The function does not return a value.
 */
void enclosure_device_fini(struct enclosure_device *enclosure);

#endif				/* _ENCLOSURE_H_INCLUDED_ */
