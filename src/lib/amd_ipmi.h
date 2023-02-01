/*
 * AMD IPMI LED control
 * Copyright (C) 2023, Advanced Micro Devices, Inc.
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

#include "block.h"

int _amd_ipmi_em_enabled(const char *path, struct led_ctx *ctx);
int _amd_ipmi_write(struct block_device *device, enum led_ibpi_pattern ibpi);
char *_amd_ipmi_get_path(const char *cntrl_path, const char *sysfs_path);
