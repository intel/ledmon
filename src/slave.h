/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2016 Intel Corporation.
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

#ifndef _SLAVE_H_INCLUDED_
#define _SLAVE_H_INCLUDED_

/**
 */
#define SLAVE_STATE_UNKNOWN      0x00
#define SLAVE_STATE_IN_SYNC      0x01
#define SLAVE_STATE_SPARE        0x02
#define SLAVE_STATE_FAULTY       0x04
#define SLAVE_STATE_WRITE_MOSTLY 0x08
#define SLAVE_STATE_BLOCKED      0x10

/**
 */
struct slave_device {
	struct raid_device *raid;
	unsigned int errors;
	unsigned int slot;
	struct block_device *block;
	unsigned char state;
};

/**
 */
struct slave_device *slave_device_init(const char *path, void *block_list);

/**
 */
void slave_device_fini(struct slave_device *device);

#endif				/* _SLAVE_H_INCLUDED_ */
