/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2017 Intel Corporation.
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

#ifndef _CNTRL_H_INCLUDED_
#define _CNTRL_H_INCLUDED_

/**
 * This enumeration type lists all supported storage controller types.
 */
enum cntrl_type {
	CNTRL_TYPE_UNKNOWN = 0,
	CNTRL_TYPE_DELLSSD,
	CNTRL_TYPE_VMD,
	CNTRL_TYPE_SCSI,
	CNTRL_TYPE_AHCI
};

/**
 * @brief Storage controller device structure.
 *
 * This structure describes a storage controller device existing in the system.
 */
struct cntrl_device {
	/**
	* Path to the device in sysfs tree.
	*/
	char *sysfs_path;

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
 * To be more specific it only frees memory allocated for the fields of the
 * structure. It is due to the way list is implemented for the purpose of this
 * utility.
 *
 * @param[in]     device         pointer to controller device structure.
 *
 * @return The function does not return a value.
 */
void cntrl_device_fini(struct cntrl_device *device);

/**
 * @brief Prints given controllers list.
 *
 * This is internal function of monitor service. This function prints the path
 * and type of controller for every controller on given in argument list.
 *
 * @param[in]      ctrl_list           address to first element from
 *                                     controllers list.
 *
 * @return The function does not return a value.
 */
void cntrl_print_all(struct cntrl_device *ctrl_list);

#endif				/* _CNTRL_H_INCLUDED_ */
