// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 Intel Corporation.

#ifndef NPEM_H_INCLUDED_
#define NPEM_H_INCLUDED_
#include "block.h"
#include "led/libled.h"
#include "slot.h"
#include "status.h"
#include "sysfs.h"

int is_npem_capable(const char *path, struct led_ctx *ctx);
status_t npem_write(struct block_device *device, enum led_ibpi_pattern ibpi);
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
