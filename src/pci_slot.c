/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2016 Intel Corporation.
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

#include <config.h>

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "pci_slot.h"
#include "utils.h"

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
		free(result);
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
		if (slot->sysfs_path)
			free(slot->sysfs_path);
		if (slot->address)
			free(slot->address);
	}
}
