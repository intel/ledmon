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

#ifndef NPEM_H_INCLUDED_
#define NPEM_H_INCLUDED_
#include "block.h"
#include "ibpi.h"

int is_npem_capable(const char *path);
int npem_write(struct block_device *device, enum ibpi_pattern ibpi);
char *npem_get_path(const char *cntrl_path);

#endif // NPEM_H_INCLUDED_
