/*
 * AMD NVME LED is controlled by writing slot cap registers.
 * Copyright (C) 2023-, Advanced Micro Devices, Inc.
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <sys/file.h>
#include <sys/types.h>
#include <time.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "config.h"
#include "led/libled.h"
#include "list.h"
#include "utils.h"
#include "amd.h"
#include "amd_nvme_slot.h"
#include "libled_private.h"

#define NVME_LED_CTL_OFFSET	6
#define NVME_LED_CTL_MASK	(0xf << NVME_LED_CTL_OFFSET)

static uint8_t nvme_ibpi_pattern[] = {
	[LED_IBPI_PATTERN_NORMAL]	= 0xf,
	[LED_IBPI_PATTERN_FAILED_DRIVE]	= 0xd,   /* solid on */
	[LED_IBPI_PATTERN_LOCATE]	= 0xb,   /* blink */
	[LED_IBPI_PATTERN_LOCATE_OFF]	= 0xf,
};

#define PCIE_CONFIG_STATUS_REGISTER_OFFSET	0x06
#define PCIE_CONFIG_CAP_POINTER_OFFSET		0x34
#define PCIE_SUPPORT_CAP_LIST			(1 << 4)

#define PCIE_CAP_STRUCTURE_ID			0x10
#define PCIE_CAP_REGISTER_OFFSET		0x03
#define PCIE_SLOT_IMPLEMENTED			(1 << 0)
#define PCIE_SLOT_CAP_REGISTER_OFFSET		0x14
#define PCIE_ATTENTION_INDICATOR_PRESENT	(1 << 3)
#define PCIE_POWER_INDICATOR_PRESENT		(1 << 4)
#define PCIE_SLOT_CONTROL_REGISTER_OFFSET	0x18


static int read_pci_config_byte(FILE *fp, int offset, unsigned char *data)
{
	int ret;

	/* the PCIe config space is 4k, so just check the boundary */
	if ((offset < 0) && (offset > 4095))
		return -1;

	ret = fseek(fp, offset, SEEK_SET);
	if (ret)
		return ret;

	ret = fread(data, 1, 1, fp);
	if (ret != 1)
		return -1;

	return 0;
}

static int read_pci_config_short(FILE *fp, int offset, unsigned short *data)
{
	int ret;

	if ((offset < 0) && (offset > 4094))
		return -1;

	ret = fseek(fp, offset, SEEK_SET);
	if (ret)
		return ret;

	ret = fread(data, 2, 1, fp);
	if (ret != 1)
		return -1;

	return 0;
}

static int write_pci_config_short(FILE *fp, int offset, unsigned short data)
{
	int ret;

	if ((offset < 0) && (offset > 4094))
		return -1;

	ret = fseek(fp, offset, SEEK_SET);
	if (ret)
		return ret;

	ret = fwrite(&data, 2, 1, fp);
	if (ret != 1)
		return -1;

	return 0;
}

static int is_nvme_controller(const char *path)
{
	char temp[PATH_MAX], link[PATH_MAX], *t;

	snprintf(temp, sizeof(temp), "%s/driver", path);
	if (realpath(temp, link) == NULL)
		return 0;

	t = strrchr(link, '/');
	if (t && !strcmp(t + 1, "nvme"))
		return 1;

	return 0;
}

static char *get_nvme_config_path(struct led_ctx *ctx, const char *path)
{
	char *tmp;
	int len1, len2;
	char *nvme_config_path = NULL;

	nvme_config_path = malloc(PATH_MAX);
	if (!nvme_config_path) {
		lib_log(ctx, LED_LOG_LEVEL_ERROR,
			"[nvme] Alloc mem for nvme_config_path fail!\n");
		return NULL;
	}
	/*
	 * For example, if this is the path to an NVMe controller:
	 * "/sys/devices/pci0000:80/0000:80:05.1/0000:81:00.0"
	 * we need to know the PCIe bridge's(NVMe controller attached)
	 * config file path, tmp is "/0000:81:00.0"
	 */
	tmp = strrchr(path, '/');
	len1 = strlen(tmp);
	len2 = strlen(path);

	memset(nvme_config_path, 0, PATH_MAX);
	memcpy(nvme_config_path, path, (len2 - len1));
	nvme_config_path[len2 - len1] = '/';
	/* we can get the PCIe bridge's config path */
	memcpy(nvme_config_path + (len2 - len1) + 1, "config", 6);
	lib_log(ctx, LED_LOG_LEVEL_INFO, "nvme_config_path is： %s\n",
		nvme_config_path);

	return nvme_config_path;
}

static __inline__ void put_path(void *path)
{
	free(path);
}

static int get_slot_control_reg_offset(struct led_ctx *ctx, FILE *fp)
{
	unsigned char pcie_status_reg;
	unsigned char capabilities_pointer;
	unsigned char capability_id;
	unsigned char capability_reg;
	unsigned char slot_cap_reg;

	read_pci_config_byte(fp, PCIE_CONFIG_STATUS_REGISTER_OFFSET,
			     &pcie_status_reg);
	if (!(pcie_status_reg & PCIE_SUPPORT_CAP_LIST)) {
		lib_log(ctx, LED_LOG_LEVEL_INFO, "[nvme] Not support cap list!\n");
		return -1;
	}

	read_pci_config_byte(fp, PCIE_CONFIG_CAP_POINTER_OFFSET,
			     &capabilities_pointer);
	read_pci_config_byte(fp, capabilities_pointer, &capability_id);

	while ((PCIE_CAP_STRUCTURE_ID != capability_id) && capabilities_pointer) {
		read_pci_config_byte(fp, capabilities_pointer + 1,
				     &capabilities_pointer);
		read_pci_config_byte(fp, capabilities_pointer, &capability_id);
	}

	lib_log(ctx, LED_LOG_LEVEL_INFO, "[nvme]The PCIe capability struct"
		"offset = 0x%x\n", capabilities_pointer);
	if (!capabilities_pointer) {
		lib_log(ctx, LED_LOG_LEVEL_INFO,
			"[nvme]Can't find the PCIe cap structure!\n");
		return -1;
	}

	read_pci_config_byte(fp, capabilities_pointer + PCIE_CAP_REGISTER_OFFSET,
			     &capability_reg);
	if (!(capability_reg & PCIE_SLOT_IMPLEMENTED)) {
		lib_log(ctx, LED_LOG_LEVEL_INFO,
			"[nvme]The PCIe not support slot!\n");
		return -1;
	}

	read_pci_config_byte(fp, capabilities_pointer + PCIE_SLOT_CAP_REGISTER_OFFSET,
			     &slot_cap_reg);
	if (!(slot_cap_reg & PCIE_ATTENTION_INDICATOR_PRESENT)  ||
	    !(slot_cap_reg & PCIE_POWER_INDICATOR_PRESENT)) {
		lib_log(ctx, LED_LOG_LEVEL_INFO,
			"[nvme]The slot cap not support attention and power!\n");
		return -1;
	}

	return (capabilities_pointer + PCIE_SLOT_CONTROL_REGISTER_OFFSET);
}

static int check_nvme_fw_config(struct led_ctx *ctx, const char *path)
{
	FILE *fp;
	char *nvme_config_path;

	nvme_config_path = get_nvme_config_path(ctx, path);
	if (!nvme_config_path) {
		lib_log(ctx, LED_LOG_LEVEL_ERROR, "Get NVMe config path fail\n");
		return -1;
	}

	fp = fopen(nvme_config_path, "rb+");
	if (!fp) {
		lib_log(ctx, LED_LOG_LEVEL_ERROR, "Open %s fail!, please check"
			" the root device of sata controller\n", nvme_config_path);
		goto fail1;
	}

	if (get_slot_control_reg_offset(ctx, fp) < 0) {
		lib_log(ctx, LED_LOG_LEVEL_ERROR,
			"Firmware has not configured with nvme slot control enable!\n");
		goto fail2;
	}

	fclose(fp);
	put_path(nvme_config_path);

	return 0;

fail2:
	fclose(fp);
fail1:
	put_path(nvme_config_path);
	return -1;
}

static int config_nvme_slot(struct led_ctx *ctx, FILE *fp, enum led_ibpi_pattern ibpi)
{
	unsigned char cap_reg_offset;
	unsigned short slot_ctl_reg;

	cap_reg_offset = get_slot_control_reg_offset(ctx, fp);
	if (cap_reg_offset < 0) {
		lib_log(ctx, LED_LOG_LEVEL_ERROR, "Get pcie cap structure offset fail!\n");
		return -1;
	}

	read_pci_config_short(fp, cap_reg_offset, &slot_ctl_reg);

	lib_log(ctx, LED_LOG_LEVEL_INFO,
		"slot ctl register offset is 0x%02x, value = 0x%02x, ibpi = 0x%02x!\n",
		(unsigned int)cap_reg_offset, slot_ctl_reg, ibpi);

	slot_ctl_reg &= (~NVME_LED_CTL_MASK);
	slot_ctl_reg |= ((nvme_ibpi_pattern[ibpi] << NVME_LED_CTL_OFFSET) &
			  NVME_LED_CTL_MASK);

	lib_log(ctx, LED_LOG_LEVEL_INFO, "slot ctl register = 0x%02x!\n", slot_ctl_reg);
	write_pci_config_short(fp, cap_reg_offset, slot_ctl_reg);

	return 0;
}

static int _set_nvme_slot_register(struct block_device *device, enum led_ibpi_pattern ibpi)
{
	int ret;
	FILE *fp;
	struct led_ctx *ctx = device->cntrl->ctx;

	if (!is_nvme_controller(device->cntrl->sysfs_path)) {
		lib_log(ctx, LED_LOG_LEVEL_ERROR, "The device(%s) is not nvme!\n",
			device->cntrl_path);
		return -1;
	}

	fp = fopen(device->cntrl_path, "rb+");
	if (!fp) {
		lib_log(ctx, LED_LOG_LEVEL_ERROR,
			"Open %s fail!\n", device->cntrl_path);
		return -1;
	}

	ret = config_nvme_slot(ctx, fp, ibpi);
	if (ret) {
		lib_log(ctx, LED_LOG_LEVEL_ERROR, "config nvme slot fail!\n");
		fclose(fp);
		return -1;
	}

	fclose(fp);

	device->ibpi_prev = ibpi;

	return 0;
}

int _amd_nvme_slot_cap_enabled(const char *path, struct led_ctx *ctx)
{
	if (is_nvme_controller(path)) {
		/* check the sgpio for sata or pcie slot for nvme */
		if (check_nvme_fw_config(ctx, path))
			return 0;
	} else {
		lib_log(ctx, LED_LOG_LEVEL_ERROR,
			"This methord only support nvme led ctl %s\n", path);
		return 0;
	}

	/* set the default led pattern */
	return 1;
}

int _amd_nvme_slot_write(struct block_device *device, enum led_ibpi_pattern ibpi)
{
	if (!device || !device->sysfs_path || !device->cntrl_path)
		__set_errno_and_return(ENOTSUP);

	if ((ibpi < LED_IBPI_PATTERN_NORMAL) || (ibpi > LED_IBPI_PATTERN_LOCATE_OFF))
		__set_errno_and_return(ERANGE);

	if ((ibpi == LED_IBPI_PATTERN_DEGRADED) ||
		(ibpi == LED_IBPI_PATTERN_FAILED_ARRAY))
		__set_errno_and_return(ENOTSUP);

	return _set_nvme_slot_register(device, ibpi);
}

char *_amd_nvme_slot_get_path(const char *cntrl_path, struct led_ctx *ctx)
{
	if (is_nvme_controller(cntrl_path))
		return get_nvme_config_path(ctx, cntrl_path);

	return NULL;
}
