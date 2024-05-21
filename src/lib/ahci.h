// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 Intel Corporation.

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
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t ahci_sgpio_write(struct block_device *path, enum led_ibpi_pattern ibpi);

#endif				/* _AHCI_H_INCLUDED_ */
