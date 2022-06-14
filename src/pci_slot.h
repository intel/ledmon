/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2016-2017 Intel Corporation.
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

#ifndef PCI_SLOT_H_INCLUDED_
#define PCI_SLOT_H_INCLUDED_

#include "ibpi.h"
#include "slot.h"
#include "status.h"

/**
 * @brief PCI hotplug slot structure.
 *
 * This structure describes a PCI hotplug slot exposed by the hotplug driver.
 */
struct pci_slot {
 /**
 * Path to PCI hotplug slot in sysfs tree.
 */
	char *sysfs_path;

 /**
 * PCI hotplug slot address.
 */
	char *address;
};

/**
 * @brief Allocates memory for a PCI hotplug slot structure.
 *
 * This function allocates memory for a new structure describing hotplug slot.
 * It reads the sysfs entries and populates structure fields.
 *
 * @param[in]      path           Path to a PCI hotplug slot in sysfs tree.
 *                                The path begins with "/sys/bus/pci/slots/".
 *
 * @return Pointer to PCI hotplug slot structure if successful, otherwise the
 *         function returns NULL pointer. The NULL pointer means either the
 *         specified path is invalid or there's not enough memory in the system
 *         to allocated new structure.
 */
struct pci_slot *pci_slot_init(const char *path);

/**
 * @brief Releases an PCI hotplug slot structure.
 *
 * This function releases memory allocated for PCI hotplug slotstructure.
 *
 * @param[in]      slot         Pointer to PCI hotplug slot structure.
 *
 * @return The function does not return a value.
 */
void pci_slot_fini(struct pci_slot *slot);

/**
 * @brief Gets led state for slot.
 *
 * This function finds slot connected to given identifier
 * and fills slot response related to the slot.
 *
 * @param[in]         device         Requested device name.
 * @param[in]         slot_num       Requested identifier of the slot.
 * @param[in]         slot_res       Pointer to the slot response.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t pci_get_slot(char *device, char *slot_num, struct slot_response *slot_res);

/**
 * @brief Sets led state for slot.
 *
 * This function finds slot connected to given number or device name
 * and set given led state.
 *
 * @param[in]         slot_num       Requested number of the slot.
 * @param[in]         state          IBPI state based on slot request.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t pci_set_slot(char *slot_num, enum ibpi_pattern state);
#endif // PCI_SLOT_H_INCLUDED_
