/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2023 Intel Corporation.
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

#ifndef _PIDFILE_H_INCLUDED_
#define _PIDFILE_H_INCLUDED_

#include <unistd.h>

#include "status.h"

/**
 */
status_t pidfile_create(const char *name);

/**
 */
int pidfile_remove(const char *name);

/**
 */
status_t pidfile_check(const char *name, pid_t *pid);

#endif				/* _PIDFILE_H_INCLUDED_ */
