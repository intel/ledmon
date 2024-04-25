// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2009 Intel Corporation.

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
