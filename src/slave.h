/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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

#endif /* _SLAVE_H_INCLUDED_ */
