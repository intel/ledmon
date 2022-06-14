/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2016-2018 Intel Corporation.
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

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "config.h"
#include "pci_slot.h"
#include "sysfs.h"
#include "utils.h"
#include "vmdssd.h"

/*
 * Allocates memory for PCI hotplug slot structure and initializes fields of
 * the structure.
 */
struct pci_slot *pci_slot_init(const char *path)
{
	struct pci_slot *result = NULL;

	result = malloc(sizeof(struct pci_slot));
	if (result == NULL)
		return NULL;
	result->sysfs_path = str_dup(path);
	result->address = get_text(path, "address");

	return result;
}

/*
 * The function returns memory allocated for fields of hotplug slot structure
 * to the system.
 */
void pci_slot_fini(struct pci_slot *slot)
{
	if (slot) {
		free(slot->sysfs_path);
		free(slot->address);
		free(slot);
	}
}

/**
 * @brief Finds PCI slot by number of the slot.
 *
 * @param[in]       slot_number       Number of the slot
 *
 * @return Struct with pci slot if successful, otherwise the function returns NULL pointer.
 */
static struct pci_slot *find_pci_slot_by_number(char *slot_number)
{
	struct pci_slot *slot = NULL;
	char *temp;

	if (slot_number == NULL)
		return NULL;

	list_for_each(sysfs_get_pci_slots(), slot) {
		temp = basename(slot->sysfs_path);
		if (temp && strncmp(temp, slot_number, PATH_MAX) == 0)
			return slot;
	}
	return NULL;
}

/**
 * @brief Sets the slot response.
 *
 * @param[in]        slot       Struct with PCI slot parameters.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
static status_t set_slot_response(struct pci_slot *slot, struct slot_response *slot_res)
{
	struct block_device *bl_device;
	status_t status = STATUS_SUCCESS;
	int attention = get_int(slot->sysfs_path, -1, "attention");

	if (attention == -1)
		return STATUS_INVALID_STATE;

	slot_res->state = get_ibpi_for_value(attention, ibpi_to_attention);
	snprintf(slot_res->slot, PATH_MAX, "%s", basename(slot->sysfs_path));

	bl_device = get_block_device_from_sysfs_path(slot->address);
	if (bl_device)
		snprintf(slot_res->device, PATH_MAX, "/dev/%s", basename(bl_device->sysfs_path));
	else
		snprintf(slot_res->device, PATH_MAX, "(empty)");

	return status;
}

status_t pci_get_slot(char *device, char *slot_path, struct slot_response *slot_res)
{
	struct pci_slot *slot = NULL;
	struct block_device *block_device = NULL;

	if (device && device[0] != '\0') {
		char *sub_path = basename(device);
		if (sub_path == NULL) {
			log_error("Device name %s is invalid.", device);
			return STATUS_DATA_ERROR;
		}

		block_device = get_block_device_from_sysfs_path(sub_path + 1);
		if (block_device == NULL) {
			log_error("Device %s not found.", device);
			return STATUS_DATA_ERROR;
		}
		slot = vmdssd_find_pci_slot(block_device->sysfs_path);
	} else if (slot_path && slot_path[0] != '\0') {
		slot = find_pci_slot_by_number(basename(slot_path));
	}

	if (slot == NULL) {
		log_error("Specified slot was not found.");
		return STATUS_DATA_ERROR;
	}

	return set_slot_response(slot, slot_res);
}

status_t pci_set_slot(char *slot_path, enum ibpi_pattern state)
{
	struct pci_slot *slot = NULL;

	slot = find_pci_slot_by_number(basename(slot_path));
	if (slot == NULL) {
		log_error("Slot %s not found.", slot_path);
		return STATUS_NULL_POINTER;
	}

	return vmdssd_write_attention_buf(slot, state);
}
