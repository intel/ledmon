/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2022-2023 Intel Corporation.
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

#define ATTENTION_OFF        0xF  /* (1111) Attention Off, Power Off */
#define ATTENTION_LOCATE     0x7  /* (0111) Attention Off, Power On */
#define ATTENTION_REBUILD    0x5  /* (0101) Attention On, Power On */
#define ATTENTION_FAILURE    0xD  /* (1101) Attention On, Power Off */

struct ibpi2value ibpi_to_attention[] = {
	{IBPI_PATTERN_NORMAL, ATTENTION_OFF},
	{IBPI_PATTERN_LOCATE, ATTENTION_LOCATE},
	{IBPI_PATTERN_FAILED_DRIVE, ATTENTION_FAILURE},
	{IBPI_PATTERN_REBUILD, ATTENTION_REBUILD},
	{IBPI_PATTERN_LOCATE_OFF, ATTENTION_OFF},
	{IBPI_PATTERN_ONESHOT_NORMAL, ATTENTION_OFF},
	{IBPI_PATTERN_UNKNOWN}
};

#define SYSFS_PCIEHP         "/sys/module/pciehp"
#define SYSFS_VMD            "/sys/bus/pci/drivers/vmd"

static char *get_slot_from_syspath(char *path)
{
	char *cur, *ret = NULL;
	char *temp_path = str_dup(path);

	cur = strtok(temp_path, "/");
	while (cur != NULL) {
		char *next = strtok(NULL, "/");

		if ((next != NULL) && strcmp(next, "nvme") == 0)
			break;
		cur = next;
	}

	cur = strtok(cur, ".");
	if (cur)
		ret = str_dup(cur);
	free(temp_path);

	return ret;
}

char *vmdssd_get_domain(const char *path)
{
	char domain_path[PATH_MAX], real_domain_path[PATH_MAX];

	snprintf(domain_path, PATH_MAX, "%s/%s/domain",
		 SYSFS_VMD, basename(path));
	if (realpath(domain_path, real_domain_path) == NULL)
		return NULL;

	return strtok(basename(real_domain_path), ":");
}

bool vmdssd_check_slot_module(const char *slot_path)
{
	char *address;
	struct cntrl_device *cntrl;

	address = get_text(slot_path, "address");
	if (address == NULL)
		return false;

	// check if slot address contains vmd domain
	list_for_each(sysfs_get_cntrl_devices(), cntrl) {
		if (cntrl->cntrl_type == CNTRL_TYPE_VMD) {
			if (cntrl->domain == NULL)
				continue;
			if (strstr(address, cntrl->domain) == NULL)
				continue;
			return true;
		}
	}

	return false;
}

struct pci_slot *vmdssd_find_pci_slot(char *device_path)
{
	char *pci_addr;
	struct pci_slot *slot = NULL;

	pci_addr = get_slot_from_syspath(device_path);
	if (!pci_addr)
		return NULL;

	list_for_each(sysfs_get_pci_slots(), slot) {
		if (strcmp(slot->address, pci_addr) == 0)
			break;
		slot = NULL;
	}
	free(pci_addr);
	if (slot == NULL || !vmdssd_check_slot_module(slot->sysfs_path))
		return NULL;

	return slot;
}

enum ibpi_pattern vmdssd_get_attention(struct pci_slot *slot)
{
	int attention = get_int(slot->sysfs_path, -1, "attention");
	const struct ibpi2value *ibpi2val;

	if (attention == -1)
		return IBPI_PATTERN_UNKNOWN;

	ibpi2val = get_by_value(attention, ibpi_to_attention, ARRAY_SIZE(ibpi_to_attention));
	return ibpi2val->ibpi;
}

status_t vmdssd_write_attention_buf(struct pci_slot *slot, enum ibpi_pattern ibpi)
{
	char attention_path[PATH_MAX];
	char buf[WRITE_BUFFER_SIZE];
	const struct ibpi2value *ibpi2val;

	uint16_t val;

	log_debug("%s before: 0x%x\n", slot->address,
		  get_int(slot->sysfs_path, 0, "attention"));

	ibpi2val = get_by_ibpi(ibpi, ibpi_to_attention, ARRAY_SIZE(ibpi_to_attention));
	if (ibpi2val->ibpi == IBPI_PATTERN_UNKNOWN) {
		log_info("VMD: Controller doesn't support %s pattern\n", ibpi_str[ibpi]);
		return STATUS_INVALID_STATE;
	}
	val = (uint16_t)ibpi2val->value;

	snprintf(buf, WRITE_BUFFER_SIZE, "%u", val);
	snprintf(attention_path, PATH_MAX, "%s/attention", slot->sysfs_path);
	if (buf_write(attention_path, buf) != (ssize_t) strnlen(buf, WRITE_BUFFER_SIZE)) {
		log_error("%s write error: %d\n", slot->sysfs_path, errno);
		return STATUS_FILE_WRITE_ERROR;
	}
	log_debug("%s after: 0x%x\n", slot->address,
		  get_int(slot->sysfs_path, 0, "attention"));

	return STATUS_SUCCESS;
}

int vmdssd_write(struct block_device *device, enum ibpi_pattern ibpi)
{
	struct pci_slot *slot;
	char *short_name = strrchr(device->sysfs_path, '/');

	if (short_name)
		short_name++;
	else
		short_name = device->sysfs_path;

	if (ibpi == device->ibpi_prev)
		return 0;

	if ((ibpi < IBPI_PATTERN_NORMAL) || (ibpi > IBPI_PATTERN_LOCATE_OFF))
		__set_errno_and_return(ERANGE);

	slot = vmdssd_find_pci_slot(device->sysfs_path);
	if (!slot) {
		log_debug("PCI hotplug slot not found for %s\n", short_name);
		__set_errno_and_return(ENODEV);
	}

	return vmdssd_write_attention_buf(slot, ibpi);
}

char *vmdssd_get_path(const char *cntrl_path)
{
	return str_dup(cntrl_path);
}
