// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 Intel Corporation.

#ifndef _SCSI_H_INCLUDED
#define _SCSI_H_INCLUDED

#include "block.h"
#include "led/libled.h"
#include "sysfs.h"

/**
 * @brief Gets the sas host path of the device.
 *
 * This function returns a sysfs path to the sas host that the device
 * belongs to.
 *
 * @param[in]      path           Canonical sysfs path to block device.
 *
 * @return A sysfs path to host device associated with the given
 *         block device if successful, otherwise NULL pointer.
 */
char *scsi_get_host_path(const char *path, const char *cntrl_path);

/**
 * @brief Prepares SES message based on ibpi pattern.
 *
 * @param[in]      device         Sysfs path of a drive in enclosure.
 * @param[in]      ibpi           IBPI pattern to visualize.
 *
 * @return STATUS_SUCCESS on success, otherwise a valid status_t status code.
 */
status_t scsi_ses_write(struct block_device *device, enum led_ibpi_pattern ibpi);

/**
 * @brief Sends message to SES processor of an enclosure.
 *
 * This function send a message to an enclosure in order to control LEDs of
 * the given slot/component. It uses interface of ENCLOSURE kernel module to
 * control LEDs.
 *
 * @param[in]      device         Sysfs path of a drive in enclosure.
 *
 * @return 0 on success, otherwise error.
 */
int scsi_ses_flush(struct block_device *device);

/**
 * @brief Assigns enclosure device to block device.
 *
 * @param[in]      ctx           The library context.
 * @param[in]      device        Path to block device.
 *
 * @return 1 on success, 0 otherwise.
 * */
int scsi_get_enclosure(struct led_ctx *ctx, struct block_device *device);

/**
 * @brief Locates a block device by sas_address
 *
 * This function walks the list of block devices searching for one that
 * matches the sas_address.
 *
 * @param[in]      ctx           The library context.
 * @param[in]       sas_address   sas address of block device
 *
 * @return address of block_device if found, else NULL
 */
struct block_device *locate_block_by_sas_addr(struct led_ctx *ctx, uint64_t sas_address);

/**
 * @brief Given an enclosure_device pointer and slot index, prepare the SES message
 *
 * @param[in]      enclosure      Enclosure
 * @param[in]      idx            slot in enclosure
 * @param[in]      ibpi           IBPI pattern to visualize.
 *
 * @return STATUS_SUCCESS on success, otherwise a valid status_t status code.
 */
status_t scsi_ses_write_enclosure(struct enclosure_device *enclosure, int idx,
				  enum led_ibpi_pattern ibpi);

/**
 * @brief Sends message to SES processor of an enclosure.
 *
 * This function sends a message to an enclosure in order to control LEDs of
 * the given slot/component. It uses the interface of ENCLOSURE kernel module to
 * control LEDs.
 *
 * @param[in]      enclosure       Pointer to enclosure to flush
 *
 * @return 0 on success, otherwise error.
 */
int scsi_ses_flush_enclosure(struct enclosure_device *enclosure);

#endif				/* _SCSI_H_INCLUDED_ */
