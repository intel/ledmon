/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2023-2023 Intel Corporation.
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

#include <stdio.h>
#include <string.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "enclosure.h"
#include "npem.h"
#include "slot.h"
#include "sysfs.h"
#include "pci_slot.h"

struct slot_property *slot_init(void *cntrl, enum cntrl_type cntrl_type)
{
	struct slot_property *result = NULL;

	result = malloc(sizeof(struct slot_property));
	if (result == NULL)
		return NULL;

	switch (cntrl_type) {
	case CNTRL_TYPE_VMD:
		result->get_slot_fn = pci_get_slot;
		result->set_slot_fn = pci_set_slot;
		pci_get_slot(cntrl, result);
		break;
	case CNTRL_TYPE_NPEM:
		result->get_slot_fn = npem_get_slot;
		result->set_slot_fn = npem_set_slot;
		npem_get_slot(cntrl, result);
		break;
	case CNTRL_TYPE_SCSI:
		result->get_slot_fn = enclosure_get_slot;
		result->set_slot_fn = enclosure_set_slot;
		enclosure_get_slot(cntrl, result);
		break;
	default:
		free(result);
		return NULL;
	}

	result->slot = cntrl;
	result->cntrl_type = cntrl_type;

	return result;
}

void slot_fini(struct slot_property *slot_property)
{
	if (slot_property)
		free(slot_property);
}

char *get_slot_path(struct slot_property *slot_property)
{
	static char slot[PATH_MAX];

	switch (slot_property->cntrl_type) {
	case CNTRL_TYPE_VMD:
	{
		struct pci_slot *pci_slot = (struct pci_slot *)slot_property->slot;

		snprintf(slot, PATH_MAX, "%s", pci_slot->sysfs_path);
		break;
	}
	case CNTRL_TYPE_NPEM:
	{
		struct cntrl_device *cntrl_device = (struct cntrl_device *)slot_property->slot;

		snprintf(slot, PATH_MAX, "%s", cntrl_device->sysfs_path);
		break;
	}
	case CNTRL_TYPE_SCSI:
	{
		struct enclosure_device *encl = (struct enclosure_device *)slot_property->slot;

		snprintf(slot, PATH_MAX, "%s", encl->dev_path);
		break;
	}
	default:
		snprintf(slot, PATH_MAX, "(empty)");
	}

	return slot;
}

void print_slot_state(struct slot_property *slot_property)
{
	char device_name[PATH_MAX];
	char *slot = get_slot_path(slot_property);

	if (slot_property->bl_device)
		snprintf(device_name, PATH_MAX, "/dev/%s",
			 basename(slot_property->bl_device->sysfs_path));
	else
		snprintf(device_name, PATH_MAX, "(empty)");

	printf("slot: %-15s led state: %-15s device: %-15s\n",
		basename(slot), ibpi2str(slot_property->state), device_name);
}

struct slot_property *find_slot_by_device_name(char *device_name, enum cntrl_type cntrl_type)
{
	struct slot_property *slot;

	list_for_each(sysfs_get_slots(), slot) {
		if (slot->cntrl_type != cntrl_type)
			continue;
		if (slot->bl_device == NULL)
			continue;
		if (strncmp(basename(slot->bl_device->sysfs_path),
			    basename(device_name), PATH_MAX) == 0)
			return slot;
	}
	return NULL;
}

struct slot_property *find_slot_by_slot_path(char *slot_path, enum cntrl_type cntrl_type)
{
	struct slot_property *slot;
	char *path;

	list_for_each(sysfs_get_slots(), slot) {
		if (slot->cntrl_type != cntrl_type)
			continue;
		path = get_slot_path(slot);
		if (strncmp(basename(path), slot_path, PATH_MAX) == 0)
			return slot;
	}
	return NULL;
}
