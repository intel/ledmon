/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2022-2022 Intel Corporation.
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

#ifndef _CNTRL_H_INCLUDED_
#define _CNTRL_H_INCLUDED_

#include <limits.h>

/**
 * This enumeration type lists all supported storage controller types.
 */
enum cntrl_type {
	CNTRL_TYPE_UNKNOWN = 0,
	CNTRL_TYPE_DELLSSD,
	CNTRL_TYPE_VMD,
	CNTRL_TYPE_SCSI,
	CNTRL_TYPE_AHCI,
	CNTRL_TYPE_NPEM,
	CNTRL_TYPE_AMD,
};


/**
 * @brief Translates string to controller enumerated type
 *
 * @param cntrl_str          string to translate
 * @return enum cntrl_type   enumerated value
 */
enum cntrl_type string_to_cntrl_type(const char *cntrl_str);

/**
 * @brief Translates enumerated type to string representation
 *
 * @param cntrl               Enumerated type to translate to string representation
 * @return const char* const  String representation of enumerated type
 */
const char *const cntrl_type_to_string(enum cntrl_type cntrl);

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
	enum cntrl_type cntrl_type;

	/**
	 * Flag if scsi controller driver is "isci"
	 */
	int isci_present;

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
};

/**
 * @brief Allocates a new controller device structure.
 *
 * This function allocates memory for a new structure of storage controller
 * device. It reads the sysfs entries and populates structure fields.
 * The function registers only supported storage controllers.
 *
 * @param[in]      path           path to storage controller in sysfs tree.
 *
 * @return Pointer to storage controller structure if successful, otherwise the
 *         function returns NULL pointer. The NULL pointer means that controller
 *         device is not supported.
 */
struct cntrl_device *cntrl_device_init(const char *path);

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

/**
 * @brief Prints given controller to stdout.
 *
 * This function prints the path and type of controller device given as
 * argument.
 *
 * @param[in]      ctrl_dev            address to element from a
 *                                     controller list.
 *
 * @return The function does not return a value.
 */
void print_cntrl(struct cntrl_device *ctrl_dev);

#endif				/* _CNTRL_H_INCLUDED_ */
