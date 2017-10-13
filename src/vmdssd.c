/*
 * Copyright (c) 2016, Intel Corporation
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

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "list.h"
#include "pci_slot.h"
#include "status.h"
#include "sysfs.h"
#include "utils.h"
#include "vmdssd.h"

#define ATTENTION_OFF        0b1111 /* Attention Off, Power Off */
#define ATTENTION_LOCATE     0b0111 /* Attention Off, Power On */
#define ATTENTION_REBUILD    0b0101 /* Attention On, Power On */
#define ATTENTION_FAILURE    0b1101 /* Attention On, Power Off */

#define SYSFS_PCIEHP         "/sys/module/pciehp"

static int _pci_slot_search(struct pci_slot *slot, const char *address)
{
	return (strcmp(slot->address, address) == 0);
}

static char *get_slot_from_syspath(char *path)
{
	char *cur, *ret = NULL;
	char *temp_path = strdup(path);
	if (!temp_path)
		return NULL;

	cur = strtok(temp_path, "/");
	while (cur != NULL) {
		char *next = strtok(NULL, "/");

		if ((next != NULL) && strcmp(next, "nvme") == 0)
			break;
		cur = next;
	}

	cur = strtok(cur, ".");
	if (cur)
		ret = strdup(cur);
	free(temp_path);

	return ret;
}

static void get_ctrl(enum ibpi_pattern ibpi, uint16_t *new)
{
	switch (ibpi) {
	case IBPI_PATTERN_LOCATE:
		*new = ATTENTION_LOCATE;
		break;
	case IBPI_PATTERN_FAILED_DRIVE:
		*new = ATTENTION_FAILURE;
	break;
	case IBPI_PATTERN_REBUILD:
		*new = ATTENTION_REBUILD;
		break;
	default:
		*new = ATTENTION_OFF;
		break;
	}
}

static int check_slot_module(const char *slot_path)
{
	char module_path[PATH_MAX], real_module_path[PATH_MAX];
	void *dir;

	// check if slot is managed by pciehp driver
	snprintf(module_path, PATH_MAX, "%s/module", slot_path);
	dir = scan_dir(module_path);
	if (dir) {
		list_fini(dir);
		realpath(module_path, real_module_path);
		if (strcmp(real_module_path, SYSFS_PCIEHP) != 0)
			__set_errno_and_return(EINVAL);
	} else {
		__set_errno_and_return(ENOENT);
	}

	return 0;
}

struct pci_slot *vmdssd_find_pci_slot(char *device_path)
{
	char *pci_addr;
	struct pci_slot *slot;

	pci_addr = get_slot_from_syspath(device_path);
	if (!pci_addr)
		return NULL;

	slot = sysfs_pci_slot_first_that(_pci_slot_search, pci_addr);
	if (slot == NULL || check_slot_module(slot->sysfs_path) < 0)
		return NULL;

	return slot;
}

int vmdssd_write(struct block_device *device, enum ibpi_pattern ibpi)
{
	char attention_path[PATH_MAX];
	char buf[WRITE_BUFFER_SIZE];
	uint16_t val;
	struct pci_slot *slot;
	char *short_name = strrchr(device->sysfs_path, '/');

	if (short_name)
		short_name++;
	else
		short_name = device->sysfs_path;

	if ((ibpi < IBPI_PATTERN_NORMAL) || (ibpi > IBPI_PATTERN_LOCATE_OFF))
		__set_errno_and_return(ERANGE);

	slot = vmdssd_find_pci_slot(device->sysfs_path);
	if (!slot) {
		log_debug("PCI hotplug slot not found for %s\n", short_name);
		__set_errno_and_return(ENODEV);
	}

	log_debug("%s before: 0x%x\n", short_name,
		  get_int(slot->sysfs_path, 0, "attention"));

	get_ctrl(ibpi, &val);
	snprintf(buf, WRITE_BUFFER_SIZE, "%u", val);
	snprintf(attention_path, PATH_MAX, "%s/attention", slot->sysfs_path);
	if (buf_write(attention_path, buf) != strlen(buf)) {
		log_error("%s write error: %d\n", slot->sysfs_path, errno);
		return -1;
	}

	log_debug("%s after: 0x%x\n", short_name,
		  get_int(slot->sysfs_path, 0, "attention"));

	return 0;
}

char *vmdssd_get_path(const char *path, const char *cntrl_path)
{
	return strdup(cntrl_path);
}
