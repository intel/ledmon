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

 /**
 * State of the Amber LED of the PCI slot.
 */
	int attention;
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
 * This function releases memory allocated for PCI hotplug slotstructure. To be
 * more specific it only frees memory allocated for the fields of the structure.
 * It is due to the way list is implemented for the purpose of this utility.
 *
 * @param[in]      slot         Pointer to PCI hotplug slot structure.
 *
 * @return The function does not return a value.
 */
void pci_slot_fini(struct pci_slot *slot);

#endif // PCI_SLOT_H_INCLUDED_
