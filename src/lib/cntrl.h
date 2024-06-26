// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 Intel Corporation.

#ifndef _CNTRL_H_INCLUDED_
#define _CNTRL_H_INCLUDED_

#include "sysfs.h"
#include "led/libled.h"

#include <limits.h>

/**
 * @brief Storage controller device structure.
 *
 * This structure describes a storage controller device existing in the system.
 */
struct cntrl_device {
	/**
	* Path to the device in sysfs tree.
	*/
	char sysfs_path[PATH_MAX];

	/**
	 * Type of storage controller device.
	 */
	enum led_cntrl_type cntrl_type;

	/**
	 * Flag if scsi controller driver is "isci"
	 */
	int isci_present;

	/**
	 * Applicable to VMD controllers. Domain address.
	 */
	char domain[PATH_MAX];

	struct _host_type {
		/**
		 * ibpi state buffer for directly attached devices
		 */
		struct gpio_tx_register_byte *ibpi_state_buffer;
		/**
		 * outbound raw byte stream
		 */
		unsigned char bitstream[4];
		/**
		 * bitstream's flush flag
		 */
		int flush;
		/**
		 * host identifier for different hba instances
		 */
		int host_id;
		/**
		 * number of total phy ports
		 */
		int ports;
		/**
		 * pointer to next structure
		 */
		struct _host_type *next;
	} *hosts;

	struct led_ctx *ctx;
};

/**
 * @brief Allocates a new controller device structure.
 *
 * This function allocates memory for a new structure of storage controller
 * device. It reads the sysfs entries and populates structure fields.
 * The function registers only supported storage controllers.
 *
 * @param[in]      path           path to storage controller in sysfs tree.
 * @param[in]      ctx            library context.
 *
 * @return Pointer to storage controller structure if successful, otherwise the
 *         function returns NULL pointer. The NULL pointer means that controller
 *         device is not supported.
 */
struct cntrl_device *cntrl_device_init(const char *path, struct led_ctx *ctx);

/**
 * @brief Releases a controller device structure.
 *
 * This function releases memory allocated for controller device structure.
 *
 * @param[in]     device         pointer to controller device structure.
 *
 * @return The function does not return a value.
 */
void cntrl_device_fini(struct cntrl_device *device);

#endif				/* _CNTRL_H_INCLUDED_ */
