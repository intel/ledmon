// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2009 Intel Corporation.

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
#include <lib/utils.h>

#define RUN_DIR "/var/run/"
#define PID ".pid"

/**
 */
status_t pidfile_create(const char *name)
{
	char buf[PATH_MAX];
	int fd, count;

	snprintf(buf, sizeof(buf), "%s/%s%s", RUN_DIR, name, PID);

	fd = open(buf, O_WRONLY | O_CREAT, 0640);
	if (fd < 0)
		return STATUS_FILE_OPEN_ERROR;
	if (lockf(fd, F_TLOCK, 0) < 0) {
		close(fd);
		return STATUS_FILE_LOCK_ERROR;
	}
	snprintf(buf, PATH_MAX, "%d\n", getpid());
	count = write(fd, buf, strlen(buf));
	close(fd);
	return (count < 0) ? STATUS_FILE_WRITE_ERROR : STATUS_SUCCESS;
}

/**
 */
int pidfile_remove(const char *name)
{
	char buf[PATH_MAX];

	snprintf(buf, sizeof(buf), "%s/%s%s", RUN_DIR, name, PID);
	return unlink(buf);
}

/**
 * @brief Test whether process with given pid is still alive
 *
 * @return STATUS_SUCCESS if process is alive and other error if not or
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
	char buf[BUF_SZ_NUM];
	pid_t tp;

	snprintf(path, sizeof(path), "%s/%s%s", RUN_DIR, name, PID);

	p = buf_read_to_dest(path, buf, sizeof(buf));
	if (p == NULL)
		return STATUS_INVALID_PATH;
	if (str_toi(&tp, p, NULL, 10) != 0)
		return STATUS_DATA_ERROR;
	if (pid)
		*pid = tp;

	return ping_proc(tp);
}
