/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2021 Intel Corporation.
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

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "cntrl.h"
#include "config.h"
#include "enclosure.h"
#include "list.h"
#include "scsi.h"
#include "ses.h"
#include "status.h"
#include "sysfs.h"
#include "utils.h"

static char *get_drive_end_dev(const char *path)
{
	char *s, *c, *p;

	c = strstr(path, "end_device");
	if (!c)
		return NULL;
	s = strchr(c, '/');
	if (!s)
		return NULL;
	p = calloc(s - c + 1, sizeof(*p));
	if (!p)
		return NULL;

	strncpy(p, c, s - c);
	return p;
}

static uint64_t get_drive_sas_addr(const char *path)
{
	uint64_t ret = 0;
	size_t size = strlen(path) * 2;
	char *buff, *end_dev;

	/* Make big buffer. */
	buff = malloc(size + 1);
	if (!buff)
		return ret;

	end_dev = get_drive_end_dev(path);
	if (!end_dev) {
		free(buff);
		return ret;
	}

	snprintf(buff, size, "/sys/class/sas_end_device/%s/device/sas_device/%s",
		 end_dev, end_dev);

	ret = get_uint64(buff, ret, "sas_address");

	free(end_dev);
	free(buff);

	return ret;
}

int scsi_get_enclosure(struct block_device *device)
{
	struct enclosure_device *encl;
	uint64_t addr;

	if (!device || !device->sysfs_path)
		return 0;

	addr = get_drive_sas_addr(device->sysfs_path);
	if (!addr)
		return 0;

	list_for_each(sysfs_get_enclosure_devices(), encl) {
		int i;

		for (i = 0; i < encl->slots_count; i++) {
			if (encl->slots[i].sas_addr == addr) {
				device->enclosure = encl;
				device->encl_index = device->enclosure->slots[i].index;
				return 1;
			}
		}
	}

	return 0;
}

/**
 */
int scsi_ses_write(struct block_device *device, enum ibpi_pattern ibpi)
{
	if (!device || !device->sysfs_path || !device->enclosure ||
	    device->encl_index == -1)
		__set_errno_and_return(EINVAL);

	/* write only if state has changed */
	if (ibpi == device->ibpi_prev)
		return 1;

	if ((ibpi < IBPI_PATTERN_NORMAL) || (ibpi > SES_REQ_FAULT))
		__set_errno_and_return(ERANGE);

	return ses_write_msg(ibpi, &device->enclosure->ses_pages, device->encl_index);
}

int scsi_ses_flush(struct block_device *device)
{
	int ret;
	int fd;

	if (!device || !device->enclosure)
		return 1;

	if (!device->enclosure->ses_pages.changes)
		return 0;

	fd = enclosure_open(device->enclosure);
	if (fd == -1)
		return 1;

	ret = ses_send_diag(fd, &device->enclosure->ses_pages);

	close(fd);

	return ret;
}

char *scsi_get_host_path(const char *path, const char *ctrl_path)
{
	char *host;
	char host_path[PATH_MAX] = { 0 };
	size_t ctrl_path_len = strlen(ctrl_path);

	if (strncmp(path, ctrl_path, ctrl_path_len) != 0)
		return NULL;
	host = get_path_hostN(path);
	if (host) {
		snprintf(host_path, sizeof(host_path), "%s/%s/bsg/sas_%s",
			 ctrl_path, host, host);
		free(host);
	}
	return str_dup(host_path);
}
