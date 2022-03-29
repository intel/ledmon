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
	result->attention = get_int(path, -1, "attention");

	if (result->attention == -1) {
		pci_slot_fini(result);
		return NULL;
	}

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
 * Finds slot identifier in sysfs path.
 */
char *pci_get_slot_number_from_path(const char *sysfs_path)
{
	if (sysfs_path)
		return strrchr(sysfs_path, '/') + 1;

	return NULL;
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

	list_for_each(sysfs_get_pci_slots(), slot) {
		temp = pci_get_slot_number_from_path(slot->sysfs_path);
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
static status_t set_slot_parameters(struct pci_slot *slot, struct slot_response *slot_res)
{
	struct block_device *bl_device;

	slot_res->state = attention_to_ibpi(get_int(slot->sysfs_path, -1, "attention"));
	char* slot_num = pci_get_slot_number_from_path(slot->sysfs_path);

	if (!slot_num) {
		log_debug("Could not parse sysfs path of the pci slot.");
		return STATUS_NULL_POINTER;
	}
	snprintf(slot_res->slot, PATH_MAX, "%s", slot_num);

	bl_device = find_block_device_by_sub_path(slot->address);
	if (bl_device && bl_device->sysfs_path) {
		char *dev_name = strrchr(bl_device->sysfs_path, '/') + 1;

		if (!dev_name) {
			log_debug("Could not parse sysfs path of the block device.");
			return STATUS_NULL_POINTER;
		}
		snprintf(slot_res->device, PATH_MAX, "/dev/%s", dev_name);
	} else {
		snprintf(slot_res->device, PATH_MAX, "(empty)");
	}

	return STATUS_SUCCESS;
}

status_t pci_get_slot(char *device, char *slot_num, struct slot_response *slot_res)
{
	struct pci_slot *slot = NULL;
	struct block_device *block_device = NULL;

	if (device && device[0] != '\0') {
		char *sub_path = strrchr(device, '/') + 1;

		block_device = find_block_device_by_sub_path(sub_path);
		if (block_device) {
			slot = vmdssd_find_pci_slot(block_device->sysfs_path);
		} else {
			log_error("Device %s not found.", device);
			return STATUS_NULL_POINTER;
		}
	}

	if (slot_num && slot_num[0] != '\0')
		slot = find_pci_slot_by_number(slot_num);

	if (slot) {
		return set_slot_parameters(slot, slot_res);
	} else {
		log_error("Specified slot was not found.");
		return STATUS_NULL_POINTER;
	}
}

status_t pci_set_slot(char *device, char *slot_num, enum ibpi_pattern state)
{
	struct slot_response *slot_res;
	struct pci_slot *slot = NULL;
	status_t status = STATUS_SUCCESS;

	slot_res = calloc(1, sizeof(struct slot_response));
	if (slot_res == NULL)
		return STATUS_NULL_POINTER;

	status = pci_get_slot(device, slot_num, slot_res);

	if (status != STATUS_SUCCESS)
		goto error;

	if (slot_res->state == state) {
		log_warning("Led state: %s is already set for the slot.", ibpi2str(state));
		status = STATUS_INVALID_STATE;
		goto error;
	}

	slot = find_pci_slot_by_number(slot_res->slot);
	if (slot) {
		status = vmdssd_write_attention_buf(slot, state);
		if (status != STATUS_SUCCESS)
			goto error;
		sysfs_scan();
	} else {
		log_error("Slot %s not found.", slot_num);
		status = STATUS_NULL_POINTER;
		goto error;
	}

error:
	free(slot_res);
	return status;
}
