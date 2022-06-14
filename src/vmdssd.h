/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (c) 2016-2022, Intel Corporation
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

#ifndef _VMDSSD_H
#define _VMDSSD_H

#include "block.h"
#include "ibpi.h"
#include "utils.h"

#define ATTENTION_OFF        0xF  /* (1111) Attention Off, Power Off */
#define ATTENTION_LOCATE     0x7  /* (0111) Attention Off, Power On */
#define ATTENTION_REBUILD    0x5  /* (0101) Attention On, Power On */
#define ATTENTION_FAILURE    0xD  /* (1101) Attention On, Power Off */

extern struct ibpi_value ibpi_to_attention[];

int vmdssd_write(struct block_device *device, enum ibpi_pattern ibpi);
char *vmdssd_get_path(const char *cntrl_path);
struct pci_slot *vmdssd_find_pci_slot(char *device_path);
status_t vmdssd_write_attention_buf(struct pci_slot *slot, enum ibpi_pattern ibpi);

#endif
