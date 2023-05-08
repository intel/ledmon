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

void print_slot_state(struct slot_property *slot_property)
{
	char device_name[PATH_MAX];
	enum ibpi_pattern led_state = slot_property->c->get_state_fn(slot_property);

	if (slot_property->bl_device)
		snprintf(device_name, PATH_MAX, "/dev/%s",
			 basename(slot_property->bl_device->sysfs_path));
	else
		snprintf(device_name, PATH_MAX, "(empty)");

	printf("slot: %-15s led state: %-15s device: %-15s\n",
		basename(slot_property->slot_id), ibpi2str(led_state), device_name);
}

struct slot_property *find_slot_by_device_name(char *device_name, enum cntrl_type cntrl_type)
{
	struct slot_property *slot;

	list_for_each(sysfs_get_slots(), slot) {
		if (slot->c->cntrl_type != cntrl_type)
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

	list_for_each(sysfs_get_slots(), slot) {
		if (slot->c->cntrl_type != cntrl_type)
			continue;
		if (strncmp(basename(slot->slot_id), slot_path, PATH_MAX) == 0)
			return slot;
	}
	return NULL;
}

status_t set_slot_pattern(struct slot_property *slot, enum ibpi_pattern state)
{
	return slot->c->set_slot_fn(slot, state);
}

enum ibpi_pattern get_slot_pattern(struct slot_property *slot)
{
	return slot->c->get_state_fn(slot);
}
