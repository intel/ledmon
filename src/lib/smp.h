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

#include "led/libled.h"
#include "block.h"
#include "sysfs.h"

#ifndef _SMP_H_INCLUDED
#define _SMP_H_INCLUDED

/* smp constants */
#define SMP_FRAME_TYPE_REQ	0x40
#define SMP_FRAME_TYPE_RESP	0x41

#define SMP_FUNC_GPIO_READ	0x02
#define SMP_FUNC_GPIO_WRITE	0x82

#define SMP_FRAME_CRC_LEN		sizeof(uint32_t)
#define SMP_DATA_CHUNK_SIZE	sizeof(uint32_t)

/* gpio constants */
/* gpio register types */
#define GPIO_REG_TYPE_CFG		0x00
#define GPIO_REG_TYPE_RX		0x01
#define GPIO_REG_TYPE_RX_GP	0x02
#define GPIO_REG_TYPE_TX		0x03
#define GPIO_REG_TYPE_TX_GP	0x04

/* gpio register indexes */
#define GPIO_REG_IND_CFG_0	0x00
#define GPIO_REG_IND_CFG_1	0x01

#define GPIO_REG_IND_RX_0		0x00
#define GPIO_REG_IND_RX_1		0x01

#define GPIO_REG_IND_TX_0		0x00
#define GPIO_REG_IND_TX_1		0x01

#define SG_RESPONSE_TIMEOUT (5 * 1000)	/* 1000 as milliseconds multiplier */
#define SCSI_MAX_CDB_LENGTH	0x10

#define GPIO_STATUS_OK			0x00
#define GPIO_STATUS_FAILURE 0x80

struct gpio_tx_register_byte {
	unsigned char error:3;
	unsigned char locate:2;
	unsigned char activity:3;
} __attribute__ ((__packed__));

/**
 * @brief Write message to outbound raw byte stream buffer.
 *
 * @param[in]      device         Path to a smp device in sysfs.
 * @param[in]      ibpi           IBPI pattern to visualize.
 *
 * @return 1 if successful or -1 in case of error
 *         and errno is set to appropriate error code.
 */
int scsi_smp_fill_buffer(struct block_device *device, enum led_ibpi_pattern ibpi);

/**
 * @brief Sends message to SMP device.
 *
 * This function triggers gpio order to control LEDs of
 * the given component.
 *
 * @param[in]      device         Path to a smp device in sysfs.
 *
 * @return Number of bytes written to device if successful or -1 in case of error
 *         and errno is set to appropriate error code.
 */
int scsi_smp_write_buffer(struct block_device *device);

/**
 * @brief Init smp and gets phy index,
 *
 * @param[in]      path            Path to the device in sysfs. It can be NULL
 *                                 to just initialize cntrl and not to get the
 *                                 phy.
 * @param[in]      cntrl           Controller device to be initialized.
 *
 * @return Phy index on success if path and cntrl weren't NULL
 *         0 if error occurred or path was NULL.
 */
int cntrl_init_smp(const char *path, struct cntrl_device *cntrl);

/**
 * @brief Write GPIO data
 *
 * @param[in]      path            Path to the device in sysfs.
 *                                 phy.
 *
 * @param[in]      smp_reg_type    GPIO register type
 *
 * @param[in]      smp_reg_index   GPIO register index
 *
 * @param[in]      smp_reg_count   GPIO register count
 *
 * @param[in]      data            Data to be written
 *
 * @return written register count
 *         <0 if error occurred
 */
int smp_write_gpio(const char *path, int smp_reg_type,
			   int smp_reg_index, int smp_reg_count, void *data,
			   size_t len);

#endif				/* _SCSI_H_INCLUDED_ */
