// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 Intel Corporation.

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
