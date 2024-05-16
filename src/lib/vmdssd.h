// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 Intel Corporation.

#ifndef _VMDSSD_H
#define _VMDSSD_H

#include <stdbool.h>

#include "block.h"
#include "led/libled.h"
#include "utils.h"
#include "sysfs.h"

#define ATTENTION_OFF        0xF  /* (1111) Attention Off, Power Off */
#define ATTENTION_LOCATE     0x7  /* (0111) Attention Off, Power On */
#define ATTENTION_REBUILD    0x5  /* (0101) Attention On, Power On */
#define ATTENTION_FAILURE    0xD  /* (1101) Attention On, Power Off */

int vmdssd_write(struct block_device *device, enum led_ibpi_pattern ibpi);
char *vmdssd_get_path(const char *cntrl_path);
char *vmdssd_get_domain(const char *path);
struct pci_slot *vmdssd_find_pci_slot(struct led_ctx *ctx, char *device_path);
status_t vmdssd_write_attention_buf(struct pci_slot *slot, enum led_ibpi_pattern ibpi);
enum led_ibpi_pattern vmdssd_get_attention(struct pci_slot *slot);
bool vmdssd_check_slot_module(struct led_ctx *ctx, const char *slot_path);

#endif
