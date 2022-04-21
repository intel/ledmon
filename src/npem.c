/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2019-2020 Intel Corporation.
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <pci/pci.h>
#include <time.h>

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

const struct ibpi_value ibpi_to_npem_capability[] = {
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

static enum ibpi_pattern npem_capability_to_ibpi(const u32 reg)
{
	const struct ibpi_value *tmp = ibpi_to_npem_capability;

	while (tmp->ibpi != IBPI_PATTERN_UNKNOWN) {
		if (reg & tmp->value)
			break;
		tmp++;
	}
	return tmp->ibpi;
}

static u32 ibpi_to_npem_cap(const enum ibpi_pattern ibpi)
{
	const struct ibpi_value *tmp = ibpi_to_npem_capability;

	while (tmp->ibpi != IBPI_PATTERN_UNKNOWN) {
		if (tmp->ibpi == ibpi)
			break;
		tmp++;
	}
	return tmp->value;
}

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
	char *p = strrchr(path, '/');
	int ret = 0;

	if (!p)
		return NULL;

	ret += str_toui(&domain, p + 1, &p, 16);
	ret += str_toui(&bus, p + 1, &p, 16);
	ret += str_toui(&dev, p + 1, &p, 16);
	ret += str_toui(&fn, p + 1, &p, 16);
	if (ret != 0)
		return NULL;

	return pci_get_dev(pacc, domain, bus, dev, fn);
}

static struct pci_dev *get_pci_dev_by_path(const char *path, struct pci_access *pacc)
{
	pacc = get_pci_access();
	if (!pacc) {
		log_error("NPEM: Unable to initialize pci access for %s\n", path);
		return NULL;
	}

	return get_pci_dev(pacc, path);
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

int is_npem_capable(const char *path)
{
	u8 val;
	struct pci_access *pacc = calloc(1, sizeof(struct pci_access));
	struct pci_dev *pdev;

	pdev = get_pci_dev_by_path(path, pacc);

	if (!pdev) {
		pci_cleanup(pacc);
		return 0;
	}

	val = read_npem_register(pdev, PCI_NPEM_CAP_REG);

	if (pdev)
		pci_free_dev(pdev);
	if (pacc)
		pci_cleanup(pacc);
	return (val & PCI_NPEM_CAP);
}

static int npem_wait_command(struct pci_dev *pdev)
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
	u32 reg;

	time(&start);
	end = start;
	while (difftime(start, end) < 1) {
		reg = read_npem_register(pdev, PCI_NPEM_STATUS_REG);

		if (reg & PCI_NPEM_STATUS_CC) {
			/* status register type is RW1C */
			write_npem_register(pdev, PCI_NPEM_STATUS_REG,
					    PCI_NPEM_STATUS_CC);
			return 0;
		}
		time(&end);
	}
	return 1;
}

int npem_write(struct block_device *device, enum ibpi_pattern ibpi)
{
	struct cntrl_device *npem_cntrl = device->cntrl;
	struct pci_access *pacc = NULL;
	struct pci_dev *pdev = NULL;

	u32 reg;
	u32 val;

	int err = 0;

	if (ibpi == device->ibpi_prev)
		return 0;

	if ((ibpi < IBPI_PATTERN_NORMAL) || (ibpi > IBPI_PATTERN_LOCATE_OFF)) {
		err = -EINVAL;
		goto exit;
	}

	pdev = get_pci_dev_by_path(npem_cntrl->sysfs_path, pacc);
	if (!pdev) {
		log_error("NPEM: Unable to get pci device for %s\n",
			  npem_cntrl->sysfs_path);
		err = -ENXIO;
		goto exit;
	}

	reg = read_npem_register(pdev, PCI_NPEM_CAP_REG);
	if ((reg & ibpi_to_npem_cap(ibpi)) == 0) {
		log_debug("NPEM: Controller %s doesn't support %s pattern\n",
			  npem_cntrl->sysfs_path, ibpi_str[ibpi]);
		ibpi = IBPI_PATTERN_NORMAL;
	}

	reg = read_npem_register(pdev, PCI_NPEM_CTRL_REG);
	val = (reg & PCI_NPEM_RESERVED);
	val = (val | PCI_NPEM_CAP | ibpi_to_npem_cap(ibpi));

	write_npem_register(pdev, PCI_NPEM_CTRL_REG, val);
	if (npem_wait_command(pdev)) {
		log_error("NPEM: Write timeout for %s\n",
			  npem_cntrl->sysfs_path);
		err = -EAGAIN;
	}

exit:
	if (pdev)
		pci_free_dev(pdev);
	if (pacc)
		pci_cleanup(pacc);
	return err;
}

char *npem_get_path(const char *cntrl_path)
{
	return str_dup(cntrl_path);
}

status_t npem_get_slot(char *device, char *slot_num, struct slot_response *slot_res)
{
	struct pci_dev *pdev = NULL;
	struct block_device *block_device = NULL;
	struct pci_access *pacc = NULL;
	status_t status = STATUS_SUCCESS;
	char *path = NULL;
	u32 reg;

	if (device && device[0] != '\0') {
		block_device = get_block_device_from_sysfs_path(basename(device));
		if (block_device)
			path = block_device->cntrl->sysfs_path;
	}
	if (slot_num && slot_num[0] != '\0') {
		path = slot_num;
		if (!is_npem_capable(path))
			return STATUS_NOT_SUPPORTED;
		block_device = get_block_device_from_sysfs_path(path);
	}
	if (block_device)
		snprintf(slot_res->device, PATH_MAX, "/dev/%s", basename(block_device->sysfs_path));
	else
		snprintf(slot_res->device, PATH_MAX, "(empty)");

	if (path)
		pdev = get_pci_dev_by_path(path, pacc);
	if (pdev) {
		reg = read_npem_register(pdev, PCI_NPEM_CTRL_REG);
		slot_res->state = npem_capability_to_ibpi(reg);
		snprintf(slot_res->slot, PATH_MAX, "%s", path);
	} else {
		log_error("NPEM: Unable to get pci device for %s\n", path);
		status = STATUS_NULL_POINTER;
	}

	if (pdev)
		pci_free_dev(pdev);
	if (pacc)
		pci_cleanup(pacc);
	return status;
}

status_t npem_set_slot(char *device, char *slot_num, enum ibpi_pattern state)
{
	struct pci_dev *pdev = NULL;
	struct pci_access *pacc = NULL;
	status_t status = STATUS_SUCCESS;
	u32 val;
	u32 reg;

	pdev = get_pci_dev_by_path(slot_num, pacc);
	if (pdev) {
		reg = read_npem_register(pdev, PCI_NPEM_CTRL_REG);
		val = (reg & PCI_NPEM_RESERVED);
		val = (val | PCI_NPEM_CAP | ibpi_to_npem_cap(state));

		write_npem_register(pdev, PCI_NPEM_CTRL_REG, val);
		if (npem_wait_command(pdev)) {
			log_error("NPEM: Write timeout for %s\n", slot_num);
			status = STATUS_FILE_WRITE_ERROR;
		}
	} else {
		log_error("NPEM: Unable to get pci device for %s\n", slot_num);
		status = STATUS_NULL_POINTER;
	}

	if (pdev)
		pci_free_dev(pdev);
	if (pacc)
		pci_cleanup(pacc);
	return status;
}
