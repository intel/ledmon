// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 Intel Corporation.

#ifndef _PCI_SLOT_H_INCLUDED_
#define _PCI_SLOT_H_INCLUDED_

/* Project headers */
#include <led/libled.h>

/* Local headers */
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

#endif // _PCI_SLOT_H_INCLUDED_
