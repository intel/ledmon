/*
 * AMD SGPIO LED control
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

#ifndef _AMD_SGPIO_INCLUDED_
#define _AMD_SGPIO_INCLUDED_

#include "block.h"
#include "led/libled.h"

#include <stdint.h>

typedef uint8_t drive_led_t;

struct drive_leds {
	drive_led_t	error;
	drive_led_t	locate;
	drive_led_t	activity;
} __attribute__ ((__packed__));

struct cache_entry {
	struct drive_leds leds[4];
	uint8_t blink_gen_a;
	uint8_t blink_gen_b;
	uint16_t reserved;
} __attribute__ ((__packed__));

int _amd_sgpio_em_enabled(const char *path, struct led_ctx *ctx);
int _amd_sgpio_write(struct block_device *device, enum led_ibpi_pattern ibpi);
char *_amd_sgpio_get_path(const char *cntrl_path, struct led_ctx *ctx);

void amd_sgpio_cache_free(struct led_ctx *ctx);

#endif
