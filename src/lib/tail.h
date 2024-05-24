// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 Intel Corporation.

#ifndef _TAIL_H_INCLUDED_
#define _TAIL_H_INCLUDED_

#include "block.h"
#include "raid.h"

/**
 */
#define TAIL_STATE_UNKNOWN      0x00
#define TAIL_STATE_IN_SYNC      0x01
#define TAIL_STATE_SPARE        0x02
#define TAIL_STATE_FAULTY       0x04
#define TAIL_STATE_WRITE_MOSTLY 0x08
#define TAIL_STATE_BLOCKED      0x10

/**
 */
struct tail_device {
	struct raid_device *raid;
	unsigned int errors;
	unsigned int slot;
	struct block_device *block;
	unsigned char state;
};

/**
 */
struct tail_device *tail_device_init(const char *path, struct list *block_list);

/**
 */
void tail_device_fini(struct tail_device *device);

#endif				/* _TAIL_H_INCLUDED_ */
