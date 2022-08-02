/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2022-2022 Intel Corporation.
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

#ifndef _ENCLOSURE_H_INCLUDED_
#define _ENCLOSURE_H_INCLUDED_

#include <stdint.h>
#include <limits.h>

#include "ses.h"

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
	char sysfs_path[PATH_MAX];

  /**
   * SAS address as identifier of an enclosure.
   */
	uint64_t sas_address;

  /**
   * Path to enclosure's sg device.
   */
	char *dev_path;

	struct ses_pages ses_pages;

	struct ses_slot *slots;
	int slots_count;
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
 * This function releases memory allocated for enclosure device structure.
 *
 * @param[in]      device         Pointer to enclosure device structure.
 *
 * @return The function does not return a value.
 */
void enclosure_device_fini(struct enclosure_device *enclosure);

int enclosure_open(const struct enclosure_device *enclosure);

#endif				/* _ENCLOSURE_H_INCLUDED_ */
