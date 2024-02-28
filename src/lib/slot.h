/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2023-2024 Intel Corporation.
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

#ifndef SLOT_H_
#define SLOT_H_

#include <stdio.h>

#include "block.h"
#include "cntrl.h"
#include "led/libled.h"
#include "utils.h"

#include "libled_private.h"

/* Forward decl. */
struct slot_property;


/**
 * @brief slot property attributes which are the same across slots of a specific type
 *
 */
struct slot_property_common {
	/**
	 * Controller type which is being represented by slot.
	 */
	enum led_cntrl_type cntrl_type;

	/**
	 * Pointer to the set slot function.
	 */
	status_t (*set_slot_fn)(struct slot_property *slot, enum led_ibpi_pattern state);

	/**
	 * Pointer to the get led state function.
	 */
	enum led_ibpi_pattern (*get_state_fn)(struct slot_property *slot);
};

/**
 * @brief Encapsulates information for enclosure
 *
 */
struct ses_slot_info {
	struct enclosure_device *encl;
	int slot_num;
};

/**
 * @brief slot property parameters
 *
 * This structure contains slot parameters.
 */
struct slot_property {
	const struct slot_property_common *c;

	/**
	 * Block device (if exists for slot).
	 */
	struct block_device *bl_device;

	/**
	 * Different payload based on slot type
	 */
	union {
		struct pci_slot *pci;
		struct cntrl_device *cntrl;
		struct ses_slot_info ses;
	} slot_spec;

	/**
	 * Unique slot ID.
	 */
	char slot_id[PATH_MAX];
};

/**
 * @brief Find slot device by path to slot.
 *
 * @param[in]        ctx              The library context.
 * @param[in]        slot_path        Path to slot.
 * @param[in]        cntrl_type       Type of controller.
 *
 * @return This function returns related slot property.
 */
struct slot_property *find_slot_by_slot_path(struct led_ctx *ctx, char *slot_path,
					     enum led_cntrl_type cntrl_type);

/**
 * @brief Find slot device by device name.
 *
 * @param[in]        ctx              The library context.
 * @param[in]        device_name       Device name.
 * @param[in]        cntrl_type       Type of controller.
 *
 * @return This function returns related slot property.
 */
struct slot_property *find_slot_by_device_name(struct led_ctx *ctx, char *device_name,
					       enum led_cntrl_type cntrl_type);

/**
 * @brief Set the slot pattern for the given slot
 *
 * @param[in]      slot       The slot to change the ibpi_pattern
 * @param[in]      state      The desired ibpi pattern for the slot
 * @return status_t
 */
status_t set_slot_pattern(struct slot_property *slot, enum led_ibpi_pattern state);

/**
 * @brief Get the slot pattern
 *
 * @param[in]      slot        The slot to retrieve the ibpi pattern
 * @return enum ibpi_pattern
 */
enum led_ibpi_pattern get_slot_pattern(struct slot_property *slot);

#endif // SLOT_H_INCLUDED_
