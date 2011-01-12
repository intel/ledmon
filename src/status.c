/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
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

#include <config.h>

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
    _S_CODE(STATUS_DATA_ERROR);
    _S_CODE(STATUS_INVALID_PATH);
    _S_CODE(STATUS_INVALID_SUBOPTION);
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
    _S_CODE(STATUS_LIST_INIT_ERROR);
    _S_CODE(STATUS_BLOCK_LIST_ERROR);
    _S_CODE(STATUS_VOLUM_LIST_ERROR);
    _S_CODE(STATUS_CNTRL_LIST_ERROR);
    _S_CODE(STATUS_SLAVE_LIST_ERROR);
    _S_CODE(STATUS_CNTNR_LIST_ERROR);
    _S_CODE(STATUS_LEDMON_INIT);
    _S_CODE(STATUS_LEDMON_RUNNING);
    _S_CODE(STATUS_ONEXIT_ERROR);
    _S_CODE(STATUS_INVALID_CONTROLLER);
    _S_CODE(STATUS_CMDLINE_ERROR);
    _S_CODE(STATUS_ENCLO_LIST_ERROR);
  default:
    return "???";
  }  
}
