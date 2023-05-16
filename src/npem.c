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
#include <stdio.h>
#include <string.h>
#include <pci/pci.h>
#include <time.h>
#include <stdbool.h>

#include "config.h"
#include "cntrl.h"
#include "list.h"
#include "npem.h"
#include "sysfs.h"
#include "utils.h"

#define PCI_EXT_CAP_ID_NPEM	0x29	/* Native PCIe Enclosure Management */

#define PCI_NPEM_CAP_REG	0x04	/* NPEM Capability Register */
#define PCI_NPEM_CTRL_REG	0x08	/* NPEM Control Register */
#define PCI_NPEM_STATUS_REG	0x0C    /* NPEM Status Register */

/* NPEM Capable/Enable */
#define PCI_NPEM_CAP		0x001
/* NPEM OK Capable/Control */
#define PCI_NPEM_OK_CAP	0x004
/* NPEM Locate Capable/Control */
#define PCI_NPEM_LOCATE_CAP	0x008
/* NPEM Fail Capable/Control */
#define PCI_NPEM_FAIL_CAP	0x010
/* NPEM Rebuild Capable/Control */
#define PCI_NPEM_REBUILD_CAP	0x020
/* NPEM Predicted Failure Analysis Capable/Control */
#define PCI_NPEM_PFA_CAP	0x040
/* NPEM Hot Spare Capable/Control */
#define PCI_NPEM_HOT_SPARE_CAP	0x080
/* NPEM in a Critical Array Capable/Control */
#define PCI_NPEM_CRA_CAP	0x100
/* NPEM in a Failed Array Capable/Control */
#define PCI_NPEM_FA_CAP	0x200
/* NPEM reserved and enclosure specific */
#define PCI_NPEM_RESERVED	~0xfff

#define PCI_NPEM_STATUS_CC	0x01  /* NPEM Command Completed */

const struct ibpi2value ibpi_to_npem_capability[] = {
	{IBPI_PATTERN_NORMAL, PCI_NPEM_OK_CAP},
	{IBPI_PATTERN_ONESHOT_NORMAL, PCI_NPEM_OK_CAP},
	{IBPI_PATTERN_DEGRADED, PCI_NPEM_CRA_CAP},
	{IBPI_PATTERN_HOTSPARE, PCI_NPEM_HOT_SPARE_CAP},
	{IBPI_PATTERN_REBUILD, PCI_NPEM_REBUILD_CAP},
	{IBPI_PATTERN_FAILED_ARRAY, PCI_NPEM_FA_CAP},
	{IBPI_PATTERN_PFA, PCI_NPEM_PFA_CAP},
	{IBPI_PATTERN_FAILED_DRIVE, PCI_NPEM_FAIL_CAP},
	{IBPI_PATTERN_LOCATE, PCI_NPEM_LOCATE_CAP},
	{IBPI_PATTERN_LOCATE_OFF, PCI_NPEM_OK_CAP},
	{IBPI_PATTERN_UNKNOWN}
};

static struct pci_access *get_pci_access()
{
	struct pci_access *pacc;

	pacc = pci_alloc();
	pci_init(pacc);

	return pacc;
}

static struct pci_dev *get_pci_dev(struct pci_access *pacc, const char *path)
{
	unsigned int domain, bus, dev, fn;
	char dpath[PATH_MAX];
	char *p;

	int ret = 0;

	snprintf(dpath, PATH_MAX, "%s", path);
	p = basename(dpath);

	ret += str_toui(&domain, p, &p, 16);
	ret += str_toui(&bus, p + 1, &p, 16);
	ret += str_toui(&dev, p + 1, &p, 16);
	ret += str_toui(&fn, p + 1, &p, 16);
	if (ret != 0)
		return NULL;

	return pci_get_dev(pacc, domain, bus, dev, fn);
}

static struct pci_cap *get_npem_cap(struct pci_dev *pdev)
{
	return pci_find_cap(pdev, PCI_EXT_CAP_ID_NPEM, PCI_CAP_EXTENDED);
}

static u32 read_npem_register(struct pci_dev *pdev, int reg)
{
	u32 val = 0;
	struct pci_cap *pcap = get_npem_cap(pdev);

	if (!pcap)
		return val;

	return pci_read_long(pdev, pcap->addr + reg);
}

static int write_npem_register(struct pci_dev *pdev, int reg, u32 val)
{
	struct pci_cap *pcap = get_npem_cap(pdev);

	if (!pcap)
		return val;

	return pci_write_long(pdev, pcap->addr + reg, val);
}

static bool is_mask_set(struct pci_dev *pdev, int reg,  u32 mask)
{
	u32 reg_val = read_npem_register(pdev, reg);

	if (reg_val & mask)
		return true;
	return false;
}

int is_npem_capable(const char *path)
{
	u8 val;
	struct pci_access *pacc = get_pci_access();
	struct pci_dev *pdev;

	if (!pacc) {
		log_error("NPEM: Unable to initialize pci access for %s\n", path);
		return 0;
	}

	pdev = get_pci_dev(pacc, path);

	if (!pdev) {
		pci_cleanup(pacc);
		return 0;
	}

	val = read_npem_register(pdev, PCI_NPEM_CAP_REG);

	pci_free_dev(pdev);
	pci_cleanup(pacc);
	return (val & PCI_NPEM_CAP);
}

static void npem_wait_command(struct pci_dev *pdev)
{
/*
 * Software must wait for an NPEM command to complete before issuing
 * the next NPEM command. However, if this bit is not set within
 * 1 second limit on command execution, software is permitted to repeat
 * the NPEM command or issue the next NPEM command.
 * PCIe r4.0 sec 7.9.20.4
 *
 * Poll the status register until the Command Completed bit becomes set
 * or timeout is reached.
 */
	time_t start, end;

	/* Check status_cc first to avoid system call if not needed */
	if (is_mask_set(pdev, PCI_NPEM_STATUS_REG, PCI_NPEM_STATUS_CC))
		return;

	start = time(NULL);
	do {
		if (is_mask_set(pdev, PCI_NPEM_STATUS_REG, PCI_NPEM_STATUS_CC))
			return;
		end = time(NULL);
	} while (difftime(end, start) < 1);
}

char *npem_get_path(const char *cntrl_path)
{
	return str_dup(cntrl_path);
}

enum ibpi_pattern npem_get_state(struct slot_property *slot)
{
	u32 reg;
	struct pci_dev *pdev = NULL;
	const struct ibpi2value *ibpi2val;
	const char *path = slot->slot_spec.cntrl->sysfs_path;
	struct pci_access *pacc = get_pci_access();

	if (!pacc) {
		log_error("NPEM: Unable to initialize pci access for %s\n", path);
		return IBPI_PATTERN_UNKNOWN;
	}

	pdev = get_pci_dev(pacc, path);

	if (!pdev) {
		log_error("NPEM: Unable to get pci device for %s\n", path);
		pci_cleanup(pacc);
		return IBPI_PATTERN_UNKNOWN;
	}

	reg = read_npem_register(pdev, PCI_NPEM_CTRL_REG);
	ibpi2val =  get_by_bits(reg, ibpi_to_npem_capability,
				ARRAY_SIZE(ibpi_to_npem_capability));

	pci_free_dev(pdev);
	pci_cleanup(pacc);

	return ibpi2val->ibpi;
}

status_t npem_set_slot(const char *sysfs_path, enum ibpi_pattern state)
{
	struct pci_dev *pdev = NULL;
	struct pci_access *pacc = get_pci_access();
	const struct ibpi2value *ibpi2val;

	u32 val;
	u32 reg;
	u32 cap;

	ibpi2val = get_by_ibpi(state, ibpi_to_npem_capability,
			       ARRAY_SIZE(ibpi_to_npem_capability));

	if (ibpi2val->ibpi == IBPI_PATTERN_UNKNOWN) {
		log_info("NPEM: Controller doesn't support %s pattern\n", ibpi_str[state]);
		return STATUS_INVALID_STATE;
	}
	cap = (u32)ibpi2val->value;

	if (!pacc) {
		log_error("NPEM: Unable to initialize pci access for %s\n", sysfs_path);
		return STATUS_NULL_POINTER;
	}

	pdev = get_pci_dev(pacc, sysfs_path);
	if (!pdev) {
		log_error("NPEM: Unable to get pci device for %s\n", sysfs_path);
		pci_cleanup(pacc);
		return STATUS_NULL_POINTER;
	}

	if (!is_mask_set(pdev, PCI_NPEM_CAP_REG, cap)) {
		log_info("NPEM: Controller %s doesn't support %s pattern\n",
			  sysfs_path, ibpi_str[state]);
		pci_free_dev(pdev);
		pci_cleanup(pacc);
		return STATUS_INVALID_STATE;
	}
	npem_wait_command(pdev);

	reg = read_npem_register(pdev, PCI_NPEM_CTRL_REG);
	val = (reg & PCI_NPEM_RESERVED);
	val = (val | PCI_NPEM_CAP | cap);

	write_npem_register(pdev, PCI_NPEM_CTRL_REG, val);

	pci_free_dev(pdev);
	pci_cleanup(pacc);
	return STATUS_SUCCESS;
}

/*
 * FIXME: Error is not checked, no need to translate to errno based codes.
 */
int npem_write(struct block_device *device, enum ibpi_pattern ibpi)
{
	if (ibpi == device->ibpi_prev)
		return STATUS_SUCCESS;

	if (ibpi < IBPI_PATTERN_NORMAL || ibpi > IBPI_PATTERN_LOCATE_OFF)
		return STATUS_INVALID_STATE;

	return npem_set_slot(device->cntrl->sysfs_path, ibpi);
}

const struct slot_property_common npem_slot_common = {
	.cntrl_type = CNTRL_TYPE_NPEM,
	.get_state_fn = npem_get_state,
	.set_slot_fn = npem_set_state
};

struct slot_property *npem_slot_property_init(struct cntrl_device *npem_cntrl)
{
	struct slot_property *result = calloc(1, sizeof(struct slot_property));
	if (result == NULL)
		return NULL;

	result->bl_device = get_block_device_from_sysfs_path(npem_cntrl->sysfs_path, true);
	result->slot_spec.cntrl = npem_cntrl;
	snprintf(result->slot_id, PATH_MAX, "%s", npem_cntrl->sysfs_path);
	result->c = &npem_slot_common;
	return result;
}

status_t npem_set_state(struct slot_property *slot, enum ibpi_pattern state)
{
	return npem_set_slot(slot->slot_spec.cntrl->sysfs_path, state);
}
