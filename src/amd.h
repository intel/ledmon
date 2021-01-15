/*
 * AMD LED control
 * Copyright (C) 2021, Advanced Micro Devices, Inc.
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

#include "block.h"

enum amd_device_type {AMD_NO_DEVICE, AMD_SATA_DEVICE, AMD_NVME_DEVICE};

struct amd_drive {
	int		ata_port;
	int		port;
	int		drive_bay;
	int		initiator;
	uint8_t		channel;
	uint8_t		slave_addr;
	enum amd_device_type dev;
};

enum amd_led_interfaces {
	AMD_INTF_UNSET,
	AMD_INTF_SGPIO,
	AMD_INTF_IPMI,
};

extern enum amd_led_interfaces amd_interface;

enum amd_ipmi_platforms {
	AMD_PLATFORM_UNSET,
	AMD_PLATFORM_ETHANOL_X,
	AMD_PLATFORM_DAYTONA_X,
};

extern enum amd_ipmi_platforms amd_ipmi_platform;

int amd_em_enabled(const char *path);
int amd_write(struct block_device *device, enum ibpi_pattern ibpi);
char *amd_get_path(const char *cntrl_path, const char *sysfs_path);

int _find_file_path(const char *start_path, const char *filename,
		    char *path, size_t path_len);

/* Register dump formats used for debug output */
#define REG_FMT_2	"%23s: %-4x%23s: %-4x\n"
#define REG_FMT_1	"%23s: %-4x\n"
