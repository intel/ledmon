/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2022-2024 Intel Corporation.
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

#ifndef NPEM_H_INCLUDED_
#define NPEM_H_INCLUDED_
#include "block.h"
#include "led/libled.h"
#include "slot.h"
#include "status.h"
#include "sysfs.h"

int is_npem_capable(const char *path, struct led_ctx *ctx);
int npem_write(struct block_device *device, enum led_ibpi_pattern ibpi);
char *npem_get_path(const char *cntrl_path);

/**
 * @brief Gets slot information.
 *
 * This function fills slot information related to the slot.
 *
 * @param[in]         slot       Pointer to the slot property element.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
enum led_ibpi_pattern npem_get_state(struct slot_property *slot);

/**
 * @brief Sets led state for slot.
 *
 * This function sets given led state for slot.
 *
 * @param[in]         slot           Requested slot.
 * @param[in]         state          IBPI state based on slot request.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t npem_set_state(struct slot_property *slot, enum led_ibpi_pattern state);

/**
 * @brief Initializes a slot_property for a specified npem controller.
 *
 * @param[in]         npem_cntrl       Specified npem controller for this slot
 * @return struct slot_property* if successful, else NULL on allocation failure
 */
struct slot_property *npem_slot_property_init(struct cntrl_device *npem_cntrl);

#endif // NPEM_H_INCLUDED_
