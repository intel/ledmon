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

int vmdssd_write(struct block_device *device, enum ibpi_pattern ibpi)
{
	char *pci_port;
	char attention_path[PATH_MAX];
	char module_path[PATH_MAX], real_module_path[PATH_MAX];
	char buf[WRITE_BUFFER_SIZE];
	uint16_t val;
	struct pci_slot *slot;
	void *dir;

	pci_port = get_slot_from_syspath(device->sysfs_path);
	if (!pci_port)
		__set_errno_and_return(ENODEV);

	slot = sysfs_pci_slot_first_that(_pci_slot_search, pci_port);
	if (slot == NULL) {
		log_debug("PCI hotplug slot not found for %s\n", device->sysfs_path);
		__set_errno_and_return(ENODEV);
	}

	// check if slot is managed by pciehp driver
	snprintf(module_path, PATH_MAX, "%s/module", slot->sysfs_path);
	dir = scan_dir(module_path);
	if (dir) {
		list_fini(dir);
		realpath(module_path, real_module_path);
		if (strcmp(real_module_path, SYSFS_PCIEHP) != 0)
			__set_errno_and_return(EINVAL);
	}

	if ((ibpi < IBPI_PATTERN_NORMAL) || (ibpi > IBPI_PATTERN_LOCATE_OFF))
		__set_errno_and_return(ERANGE);

	get_ctrl(ibpi, &val);
	snprintf(buf, WRITE_BUFFER_SIZE, "%u", val);
	snprintf(attention_path, PATH_MAX, "%s/attention", slot->sysfs_path);
	if (buf_write(attention_path, buf) != strlen(buf)) {
		log_error("%s write error: %d\n", slot->sysfs_path, errno);
		return -1;
	}

	log_debug("%s before: 0x%x ", device->sysfs_path, slot->attention);
	log_debug("after: 0x%x\n", get_int(slot->sysfs_path, 0, "attention"));

	return 0;
}

char *vmdssd_get_path(const char *path, const char *cntrl_path)
{
	return strdup(cntrl_path);
}
