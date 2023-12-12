/*
 * AMD LED is controlled by writing raw registers.
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

#ifndef _AMD_NVME_SLOT_H_
#define _AMD_NVME_SLOT_H_

#include "block.h"

int _amd_nvme_slot_cap_enabled(const char *path, struct led_ctx *ctx);
int _amd_nvme_slot_write(struct block_device *device, enum led_ibpi_pattern ibpi);
char *_amd_nvme_slot_get_path(const char *cntrl_path, struct led_ctx *ctx);

#endif
