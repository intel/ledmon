/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009,2011 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _CNTRL_H_INCLUDED_
#define _CNTRL_H_INCLUDED_

/**
 * This enumeration type lists all supported storage controller types.
 */
enum cntrl_type {
  CNTRL_TYPE_UNKNOWN = 0,
  CNTRL_TYPE_DELLSSD,
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
    void *ibpi_state_buffer;
    /**
     * host identifier for different hba instances
     */
    int host_id;
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
 * device. It reads the sysfs entries and populates structure fields. The function
 * registers only supported storage controllers.
 *
 * @param[in]      path           path to storage controller in sysfs tree.
 *
 * @return Pointer to storage controller structure if successful, otherwise the
 *         function returns NULL pointer. The NULL pointer means that controller
 *         device is not supported.
 */
struct cntrl_device * cntrl_device_init(const char *path);

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

#endif /* _CNTRL_H_INCLUDED_ */
