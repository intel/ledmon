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
#define END_LINE            '\n'

/**
 */
#define END_LINE_STR        "\n"

#endif /* _CONFIG_H_INCLUDED_ */
