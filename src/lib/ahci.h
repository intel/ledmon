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

#ifndef _AHCI_H_INCLUDED_
#define _AHCI_H_INCLUDED_

#include "block.h"
#include "led/libled.h"

/**
 * @brief Gets sysfs path to SATA port.
 *
 * The function returns a path to SATA port in sysfs tree the given block device
 * is connected to.
 *
 * @param[in]      path           Path to block device in sysfs tree.
 *
 * @return Canonical path if successful, otherwise NULL pointer if an error occurred.
 */
char *ahci_get_port_path(const char *path);

/**
 * @brief Sends LED control message using SGPIO.
 *
 * This function visualizes IBPI pattern on LEDs associated with a slot in
 * drive bay. This function is designed to send messaged to AHCI controller
 * only.
 *
 * @param[in]      path           Path in sysfs tree to slot in drive bay.
 * @param[in]      ibpi           IBPI pattern to visualize on LEDs associated
 *                                with the given slot.
 *
 * @return Number of bytes send to controller, -1 means error occurred and
 *         errno has additional error information.
 */
int ahci_sgpio_write(struct block_device *path, enum led_ibpi_pattern ibpi);

#endif				/* _AHCI_H_INCLUDED_ */
