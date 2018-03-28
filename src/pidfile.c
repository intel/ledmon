/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2017 Intel Corporation.
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

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "config.h"
#include "pidfile.h"
#include "status.h"
#include "utils.h"

/**
 */
status_t pidfile_create(const char *name)
{
	char buf[PATH_MAX];
	int fd, count;

	str_cpy(buf, "/var/run/", PATH_MAX);
	str_cat(buf, name, PATH_MAX);
	str_cat(buf, ".pid", PATH_MAX);

	fd = open(buf, O_WRONLY | O_CREAT, 0640);
	if (fd < 0)
		return STATUS_FILE_OPEN_ERROR;
	if (lockf(fd, F_TLOCK, 0) < 0) {
		close(fd);
		return STATUS_FILE_LOCK_ERROR;
	}
	sprintf(buf, "%d\n", getpid());
	count = write(fd, buf, strlen(buf));
	close(fd);
	return (count < 0) ? STATUS_FILE_WRITE_ERROR : STATUS_SUCCESS;
}

/**
 */
int pidfile_remove(const char *name)
{
	char buf[PATH_MAX];

	str_cpy(buf, "/var/run/", PATH_MAX);
	str_cat(buf, name, PATH_MAX);
	str_cat(buf, ".pid", PATH_MAX);

	return unlink(buf);
}

/**
 * @brief Test whether process with given pid is still alive
 *
 * @return STATUS_SUCCESS if proces is alive and other error if not or
 * if there is an error
 */
int ping_proc(pid_t pid)
{
	if (pid <= 0)
		return STATUS_INVALID_PATH;
	if (kill(pid, 1) == 0)
		return STATUS_SUCCESS;
	return STATUS_INVALID_PATH;
}

/**
 */
status_t pidfile_check(const char *name, pid_t *pid)
{
	char path[PATH_MAX], *p;
	pid_t tp;

	str_cpy(path, "/var/run/", PATH_MAX);
	str_cat(path, name, PATH_MAX);
	str_cat(path, ".pid", PATH_MAX);

	p = buf_read(path);
	if (p == NULL)
		return STATUS_INVALID_PATH;
	tp = atoi(p);
	if (pid)
		*pid = tp;
	free(p);
	return ping_proc(tp);
}
