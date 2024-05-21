// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2023, Advanced Micro Devices, Inc.

/* AMD IPMI LED control */

#include "block.h"

int _amd_ipmi_em_enabled(const char *path, struct led_ctx *ctx);
status_t _amd_ipmi_write(struct block_device *device, enum led_ibpi_pattern ibpi);
char *_amd_ipmi_get_path(const char *cntrl_path, const char *sysfs_path);
