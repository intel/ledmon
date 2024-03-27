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
#include "libled_private.h"

/*
 * Allocates memory for PCI hotplug slot structure and initializes fields of
 * the structure.
 */
struct pci_slot *pci_slot_init(const char *path, struct led_ctx *ctx)
{
	struct pci_slot *result = NULL;

	result = calloc(1, sizeof(struct pci_slot));
	if (result == NULL)
		return NULL;
	result->sysfs_path = strdup(path);
	if (!result->sysfs_path) {
		goto error;
	}
	result->address = get_text(path, "address");
	if (!result->address)
		goto error;

	result->ctx = ctx;

	return result;
error:
	free(result->address);
	free(result->sysfs_path);
	free(result);
	return NULL;
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

const struct slot_property_common pci_slot_common = {
	.cntrl_type = LED_CNTRL_TYPE_VMD,
	.get_state_fn = pci_get_state,
	.set_slot_fn = pci_set_slot
};

struct slot_property *pci_slot_property_init(struct pci_slot *pci_slot)
{
	struct slot_property *result = calloc(1, sizeof(struct slot_property));
	if (result == NULL)
		return NULL;

	result->bl_device = get_block_device_from_sysfs_path(pci_slot->ctx,
							     pci_slot->address, true);
	result->slot_spec.pci = pci_slot;
	snprintf(result->slot_id, PATH_MAX, "%s", pci_slot->sysfs_path);
	result->c = &pci_slot_common;

	return result;
}

status_t pci_set_slot(struct slot_property *slot, enum led_ibpi_pattern state)
{
	return vmdssd_write_attention_buf(slot->slot_spec.pci, state);
}

enum led_ibpi_pattern pci_get_state(struct slot_property *slot)
{
	return vmdssd_get_attention(slot->slot_spec.pci);
}
