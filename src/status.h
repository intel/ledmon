/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2019 Intel Corporation.
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
	STATUS_IBPI_DETERMINE_ERROR,
	STATUS_INVALID_PATH,
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
	STATUS_LIST_EMPTY,
	STATUS_LIST_INIT_ERROR,
	STATUS_BLOCK_LIST_ERROR,
	STATUS_VOLUM_LIST_ERROR,
	STATUS_CNTRL_LIST_ERROR,
	STATUS_SLAVE_LIST_ERROR,
	STATUS_CNTNR_LIST_ERROR,
	STATUS_INVALID_FORMAT,
	STATUS_LEDMON_INIT,
	STATUS_LEDMON_RUNNING,
	STATUS_ONEXIT_ERROR,
	STATUS_INVALID_CONTROLLER,
	STATUS_NOT_SUPPORTED,
	STATUS_STAT_ERROR,
	STATUS_CMDLINE_ERROR,
	STATUS_NOT_A_PRIVILEGED_USER,
	STATUS_ENCLO_LIST_ERROR,
	STATUS_SLOTS_LIST_ERROR,
	STATUS_CONFIG_FILE_ERROR,
	STATUS_LOG_FILE_ERROR,
};

/**
 */
char *strstatus(status_t scode);

#endif				/* _STATUS_H_INCLUDED_ */
