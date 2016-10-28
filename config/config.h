/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2016 Intel Corporation.
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

#ifndef _CONFIG_H_INCLUDED_
#define _CONFIG_H_INCLUDED_

#include <features.h>

/**
 */
#define _HAVE_DMALLOC_H        0

/**
 * @brief Sets errno variable and returns.
 *
 * This macro sets the errno variable and makes function return -1.
 *
 * @param[in]      __val          integer value to be set.
 *
 * @return The macro does not return a value.
 */
#define __set_errno_and_return(__val) { errno = (__val); return -1; }

/**
 */
#define PATH_DELIM          '/'

/**
 */
#define PATH_DELIM_STR      "/"

/**
 */
#define PATH_SEP            ':'

/**
 */
#define PATH_SEP_STR        ":"

/**
 */
#define END_LINE            ('\n')

/**
 */
#define END_LINE_STR        "\n"

#endif				/* _CONFIG_H_INCLUDED_ */
