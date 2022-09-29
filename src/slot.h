/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2022-2022 Intel Corporation.
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

#ifndef SLOT_H_
#define SLOT_H_

#include <stdio.h>

#include "ibpi.h"
#include "utils.h"

/**
 * @brief slot response parameters
 *
 * This structure contains slot properties.
 */
struct slot_response {
	/**
	 * Name of the device.
	 */
	char device[PATH_MAX];

	/**
	 * Unique slot identifier.
	 */
	char slot[PATH_MAX];

	/**
	 * IBPI state.
	 */
	enum ibpi_pattern state;
};

/**
 * @brief Print address, slot identifier and led state.
 *
 * @param[in]        res        Structure with slot response.
 *
 * @return This function does not return a value.
 */
static inline void print_slot_state(struct slot_response *res)
{
	printf("slot: %-15s led state: %-15s device: %-15s\n",
		res->slot, ibpi2str(res->state), res->device);
}

#endif // SLOT_H_INCLUDED_
