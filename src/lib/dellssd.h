// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2023, Dell Inc.

/* Dell Backplane LED control */

#include "block.h"
#include "sysfs.h"

int dellssd_write(struct block_device *device, enum led_ibpi_pattern ibpi);
char *dellssd_get_path(const char *cntrl_path);
int get_dell_server_type(struct led_ctx *ctx);
