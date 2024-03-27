/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2022-2024 Intel Corporation.
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

int scsi_get_enclosure(struct led_ctx *ctx, struct block_device *device)
{
	struct enclosure_device *encl;
	uint64_t addr;

	if (!device || !device->sysfs_path)
		return 0;

	addr = get_drive_sas_addr(device->sysfs_path);
	if (!addr)
		return 0;

	list_for_each(sysfs_get_enclosure_devices(ctx), encl) {
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
int scsi_ses_write(struct block_device *device, enum led_ibpi_pattern ibpi)
{
	if (!device || !device->sysfs_path || !device->enclosure ||
	    device->encl_index == -1)
		__set_errno_and_return(EINVAL);

	/* write only if state has changed */
	if (ibpi == device->ibpi_prev)
		return 1;

	if ((ibpi < LED_IBPI_PATTERN_NORMAL) || (ibpi > LED_SES_REQ_FAULT))
		__set_errno_and_return(ERANGE);

	return ses_write_msg(ibpi, &device->enclosure->ses_pages, device->encl_index);
}

int scsi_ses_write_enclosure(struct enclosure_device *enclosure, int idx,
			     enum led_ibpi_pattern ibpi)
{
	if (!enclosure || idx == -1) {
		__set_errno_and_return(EINVAL);
	}

	if ((ibpi < LED_IBPI_PATTERN_NORMAL) || (ibpi > LED_SES_REQ_FAULT))
		__set_errno_and_return(ERANGE);

	return ses_write_msg(ibpi, &enclosure->ses_pages, idx);
}

int scsi_ses_flush_enclosure(struct enclosure_device *enclosure)
{
	int ret = 0;
	int fd = enclosure_open(enclosure);
	if (fd == -1)
		return 1;

	ret = ses_send_diag(fd, &enclosure->ses_pages);
	close(fd);

	if (ret)
		return ret;

	return enclosure_reload(enclosure);
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

	if (ret)
		return ret;

	return enclosure_reload(device->enclosure);
}

char *scsi_get_host_path(const char *path, const char *ctrl_path)
{
	char *host;
	char host_path[PATH_MAX] = { 0 };
	size_t ctrl_path_len = strnlen(ctrl_path, PATH_MAX);

	if (strncmp(path, ctrl_path, ctrl_path_len) != 0)
		return NULL;
	host = get_path_hostN(path);
	if (host) {
		snprintf(host_path, sizeof(host_path), "%s/%s/bsg/sas_%s",
			 ctrl_path, host, host);
		free(host);
	}
	return strdup(host_path);
}


struct block_device *locate_block_by_sas_addr(struct led_ctx *ctx, uint64_t sas_address)
{
	struct block_device *block_device;
	list_for_each(sysfs_get_block_devices(ctx), block_device) {
		uint64_t tmp = get_drive_sas_addr(block_device->sysfs_path);
		if (tmp != 0 && tmp == sas_address) {
			return block_device;
		}
	}
	return NULL;
}
