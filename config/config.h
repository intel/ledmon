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
