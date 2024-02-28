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

#ifndef _STATUS_H_INCLUDED_
#define _STATUS_H_INCLUDED_

/**
 */
typedef int status_t;

/**
 */
enum status_code {
	STATUS_SUCCESS = 0,
	STATUS_BUFFER_OVERFLOW,
	STATUS_NULL_POINTER,
	STATUS_OUT_OF_MEMORY,
	STATUS_OUT_OF_RANGE,
	STATUS_INVALID_NODE,
	STATUS_DATA_ERROR,
	STATUS_INVALID_PATH=8,
	STATUS_INVALID_SUBOPTION,
	STATUS_INVALID_STATE,
	STATUS_SIZE_ERROR,
	STATUS_FILE_OPEN_ERROR,
	STATUS_FILE_READ_ERROR,
	STATUS_FILE_WRITE_ERROR,
	STATUS_FILE_LOCK_ERROR,
	STATUS_DIR_OPEN_ERROR,
	STATUS_SYSFS_PATH_ERROR,
	STATUS_SYSFS_INIT_ERROR,
	STATUS_SYSFS_SCAN_ERROR,
	STATUS_SYSFS_RESET_ERROR,
	STATUS_LIST_INIT_ERROR=22,
	STATUS_BLOCK_LIST_ERROR,
	STATUS_VOLUM_LIST_ERROR,
	STATUS_CNTRL_LIST_ERROR,
	STATUS_TAIL_LIST_ERROR,
	STATUS_CNTNR_LIST_ERROR,
	STATUS_INVALID_FORMAT,
	STATUS_CMDLINE_ERROR=35,
	STATUS_ENCLO_LIST_ERROR=37,
	STATUS_SLOTS_LIST_ERROR,
	STATUS_CONFIG_FILE_ERROR,
};

#endif				/* _STATUS_H_INCLUDED_ */
