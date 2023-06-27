/*
 * Dell Backplane LED control
 * Copyright (C) 2023, Dell Inc.
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
#include "sysfs.h"

int dellssd_write(struct block_device *device, enum led_ibpi_pattern ibpi);
char *dellssd_get_path(const char *cntrl_path);
int get_dell_server_type(struct led_ctx *ctx);
