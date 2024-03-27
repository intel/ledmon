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

#ifndef PCI_SLOT_H_INCLUDED_
#define PCI_SLOT_H_INCLUDED_

#include "led/libled.h"
#include "slot.h"
#include "status.h"
#include "sysfs.h"

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

	struct led_ctx *ctx;
};

/**
 * @brief Allocates memory for a PCI hotplug slot structure.
 *
 * This function allocates memory for a new structure describing hotplug slot.
 * It reads the sysfs entries and populates structure fields.
 *
 * @param[in]      path           Path to a PCI hotplug slot in sysfs tree.
 *                                The path begins with "/sys/bus/pci/slots/".
 * @param[in]      ctx            The library context.
 *
 * @return Pointer to PCI hotplug slot structure if successful, otherwise the
 *         function returns NULL pointer. The NULL pointer means either the
 *         specified path is invalid or there's not enough memory in the system
 *         to allocated new structure.
 */
struct pci_slot *pci_slot_init(const char *path, struct led_ctx *ctx);

/**
 * @brief Releases an PCI hotplug slot structure.
 *
 * This function releases memory allocated for PCI hotplug slot structure.
 *
 * @param[in]      slot         Pointer to PCI hotplug slot structure.
 *
 * @return The function does not return a value.
 */
void pci_slot_fini(struct pci_slot *slot);

/**
 * @brief Gets slot information.
 *
 * This function fills slot information related to the slot.
 *
 * @param[in]         slot       Pointer to the slot property element.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
enum led_ibpi_pattern pci_get_state(struct slot_property *slot);


/**
 * @brief Sets led state for slot.
 *
 * This function sets given led state for slot.
 *
 * @param[in]         slot           Requested slot.
 * @param[in]         state          IBPI state based on slot request.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t pci_set_slot(struct slot_property *slot, enum led_ibpi_pattern state);

/**
 * @brief Initializes a slot_property for a specified pci slot.
 *
 * @param[in]      pci_slot        The specified pci slot pointer for this slot
 * @return struct slot_property* if successful, else NULL on allocation failure
 */
struct slot_property *pci_slot_property_init(struct pci_slot *pci_slot);

#endif // PCI_SLOT_H_INCLUDED_
