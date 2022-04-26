/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2019-2020 Intel Corporation.
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

#ifndef NPEM_H_INCLUDED_
#define NPEM_H_INCLUDED_
#include "block.h"
#include "ibpi.h"
#include "slot.h"
#include "status.h"

int is_npem_capable(const char *path);
int npem_write(struct block_device *device, enum ibpi_pattern ibpi);
char *npem_get_path(const char *cntrl_path);

/**
 * @brief Gets led state for slot.
 *
 * This function finds slot connected to given identifier
 * and fills slot response related to the slot.
 *
 * @param[in]         device         Requested device name.
 * @param[in]         slot_num       Requested identifier of the slot.
 * @param[in]         slot_res       Pointer to the slot response.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t npem_get_slot(char *device, char *slot_num, struct slot_response *slot_res);

/**
 * @brief Sets led state for slot.
 *
 * This function finds slot connected to given number or device name
 * and set given led state.
 *
 * @param[in]         slot_num       Requested number of the slot.
 * @param[in]         state          IBPI state based on slot request.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t npem_set_slot(char *slot_num, enum ibpi_pattern state);
#endif // NPEM_H_INCLUDED_
