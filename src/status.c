/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2018 Intel Corporation.
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

#include "config.h"
#include "status.h"

/**
 */
#define _S_CODE(__code) \
	case __code: return #__code

/**
 */
char *strstatus(status_t scode)
{
	switch (scode) {
		_S_CODE(STATUS_SUCCESS);
		_S_CODE(STATUS_BUFFER_OVERFLOW);
		_S_CODE(STATUS_INVALID_NODE);
		_S_CODE(STATUS_OUT_OF_MEMORY);
		_S_CODE(STATUS_OUT_OF_RANGE);
		_S_CODE(STATUS_DATA_ERROR);
		_S_CODE(STATUS_IBPI_DETERMINE_ERROR);
		_S_CODE(STATUS_INVALID_PATH);
		_S_CODE(STATUS_INVALID_SUBOPTION);
		_S_CODE(STATUS_INVALID_STATE);
		_S_CODE(STATUS_NULL_POINTER);
		_S_CODE(STATUS_SIZE_ERROR);
		_S_CODE(STATUS_FILE_OPEN_ERROR);
		_S_CODE(STATUS_FILE_READ_ERROR);
		_S_CODE(STATUS_FILE_WRITE_ERROR);
		_S_CODE(STATUS_FILE_LOCK_ERROR);
		_S_CODE(STATUS_SYSFS_PATH_ERROR);
		_S_CODE(STATUS_SYSFS_INIT_ERROR);
		_S_CODE(STATUS_SYSFS_SCAN_ERROR);
		_S_CODE(STATUS_SYSFS_RESET_ERROR);
		_S_CODE(STATUS_DIR_OPEN_ERROR);
		_S_CODE(STATUS_LIST_EMPTY);
		_S_CODE(STATUS_LIST_INIT_ERROR);
		_S_CODE(STATUS_BLOCK_LIST_ERROR);
		_S_CODE(STATUS_VOLUM_LIST_ERROR);
		_S_CODE(STATUS_CNTRL_LIST_ERROR);
		_S_CODE(STATUS_SLAVE_LIST_ERROR);
		_S_CODE(STATUS_CNTNR_LIST_ERROR);
		_S_CODE(STATUS_INVALID_FORMAT);
		_S_CODE(STATUS_LEDMON_INIT);
		_S_CODE(STATUS_LEDMON_RUNNING);
		_S_CODE(STATUS_ONEXIT_ERROR);
		_S_CODE(STATUS_INVALID_CONTROLLER);
		_S_CODE(STATUS_NOT_SUPPORTED);
		_S_CODE(STATUS_STAT_ERROR);
		_S_CODE(STATUS_CMDLINE_ERROR);
		_S_CODE(STATUS_NOT_A_PRIVILEGED_USER);
		_S_CODE(STATUS_ENCLO_LIST_ERROR);
		_S_CODE(STATUS_SLOTS_LIST_ERROR);
		_S_CODE(STATUS_CONFIG_FILE_ERROR);
		_S_CODE(STATUS_LOG_FILE_ERROR);
	default:
		return "???";
	}
}
