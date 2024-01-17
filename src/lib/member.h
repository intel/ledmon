/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2022-2023 Intel Corporation.
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

#ifndef _MEMBER_H_INCLUDED_
#define _MEMBER_H_INCLUDED_

#include "block.h"
#include "raid.h"

/**
 */
#define MEMBER_STATE_UNKNOWN      0x00
#define MEMBER_STATE_IN_SYNC      0x01
#define MEMBER_STATE_SPARE        0x02
#define MEMBER_STATE_FAULTY       0x04
#define MEMBER_STATE_WRITE_MOSTLY 0x08
#define MEMBER_STATE_BLOCKED      0x10

/**
 */
struct member_device {
	struct raid_device *raid;
	unsigned int errors;
	unsigned int slot;
	struct block_device *block;
	unsigned char state;
};

/**
 */
struct member_device *member_device_init(const char *path, struct list *block_list);

/**
 */
void member_device_fini(struct member_device *device);

#endif				/* _MEMBER_H_INCLUDED_ */
