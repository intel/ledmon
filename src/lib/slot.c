/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2023-2024 Intel Corporation.
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

struct slot_property *find_slot_by_device_name(struct led_ctx *ctx, char *device_name,
					       enum led_cntrl_type cntrl_type)
{
	struct slot_property *slot;

	list_for_each(sysfs_get_slots(ctx), slot) {
		if (slot->c->cntrl_type != cntrl_type)
			continue;

		if (!slot->bl_device || slot->bl_device->devnode[0] == 0)
			continue;

		if (strncmp(basename(slot->bl_device->devnode),
			    basename(device_name), PATH_MAX) == 0)
			return slot;
	}
	return NULL;
}

struct slot_property *find_slot_by_slot_path(struct led_ctx *ctx, char *slot_path,
					     enum led_cntrl_type cntrl_type)
{
	struct slot_property *slot;

	list_for_each(sysfs_get_slots(ctx), slot) {
		if (slot->c->cntrl_type != cntrl_type)
			continue;
		if (strncmp(basename(slot->slot_id), basename(slot_path), PATH_MAX) == 0)
			return slot;
	}
	return NULL;
}

status_t set_slot_pattern(struct slot_property *slot, enum led_ibpi_pattern state)
{
	return slot->c->set_slot_fn(slot, state);
}

enum led_ibpi_pattern get_slot_pattern(struct slot_property *slot)
{
	return slot->c->get_state_fn(slot);
}
