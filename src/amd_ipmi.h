/*
 * AMD IPMI LED control
 * Copyright (C) 2019, Advanced Micro Devices, Inc.
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

int _amd_ipmi_em_enabled(const char *path);
int _amd_ipmi_write(struct block_device *device, enum ibpi_pattern ibpi);
char *_amd_ipmi_get_path(const char *cntrl_path, const char *sysfs_path);
