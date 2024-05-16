// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2023, Advanced Micro Devices, Inc.

/* AMD LED control */

#include "block.h"
#include "sysfs.h"

enum amd_device_type {AMD_NO_DEVICE, AMD_SATA_DEVICE, AMD_NVME_DEVICE};

struct amd_drive {
	int		ata_port;
	int		port;
	int		drive_bay;
	int		initiator;
	uint8_t		channel;
	uint8_t		tail_addr;
	enum amd_device_type dev;
	struct led_ctx	*ctx;
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

int amd_em_enabled(const char *path, struct led_ctx *ctx);
int amd_write(struct block_device *device, enum led_ibpi_pattern ibpi);
char *amd_get_path(const char *cntrl_path, const char *sysfs_path, struct led_ctx *ctx);

int _find_file_path(const char *start_path, const char *filename,
		    char *path, size_t path_len, struct led_ctx *ctx);

/* Register dump formats used for debug output */
#define REG_FMT_2	"%23s: %-4x%23s: %-4x\n"
#define REG_FMT_1	"%23s: %-4x\n"
