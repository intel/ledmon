/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2022 Intel Corporation.
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
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "config.h"
#include "enclosure.h"
#include "utils.h"

/**
 * @brief Gets SAS address of an enclosure device.
 *
 * This is internal function of enclosure module. The function reads a
 * SAS address of an enclosure from sysfs attribute.
 *
 * @param[in]      path           Path to enclosure device in sysfs tree.
 *
 * @return SAS address of an enclosure if successful, otherwise 0.
 */
#define SAS_DEVICE "/sas_device"
static uint64_t _get_sas_address(const char *path)
{
	char *tmp = str_dup(path);
	char buf[PATH_MAX];
	char *p, *s;

	p = strstr(tmp, "/expander");
	if (p == NULL)
		goto out;
	s = strchr(p + 1, PATH_DELIM);
	if (s == NULL)
		goto out;
	*s = '\0';
	snprintf(buf, sizeof(buf), "%s%s%s", tmp, SAS_DEVICE, p);
	free(tmp);
	return get_uint64(buf, 0, "sas_address");

out:
	free(tmp);
	return 0;
}

#define SCSI_GEN "device/scsi_generic"

static char *_get_dev_sg(const char *encl_path)
{
	char *ret = NULL;
	DIR *d;
	struct dirent *de;
	size_t sg_path_size = strlen(encl_path) + strlen(SCSI_GEN) + 2;
	char *sg_path = malloc(sg_path_size);

	if (!sg_path)
		return NULL;

	snprintf(sg_path, sg_path_size, "%s/%s", encl_path, SCSI_GEN);

	/* /sys/class/enclosure/X/device/scsi_generic path is expected. */

	d = opendir(sg_path);
	free(sg_path);
	if (!d)
		return NULL;
	while ((de = readdir(d))) {
		if ((strcmp(de->d_name, ".") == 0) ||
		    (strcmp(de->d_name, "..") == 0))
			continue;
		break;
	}
	if (de) {
		size_t size = strlen("/dev/") + strlen(de->d_name) + 1;
		ret = malloc(size);
		if (ret)
			snprintf(ret, size, "/dev/%s", de->d_name);
	}
	closedir(d);
	return ret;
}

/*
 * Allocates memory for enclosure device structure and initializes fields of
 * the structure.
 */
struct enclosure_device *enclosure_device_init(const char *path)
{
	char temp[PATH_MAX] = "\0";
	struct enclosure_device *enclosure;
	int ret;
	int fd;

	if (!realpath(path, temp))
		return NULL;

	enclosure = calloc(1, sizeof(struct enclosure_device));
	if (enclosure == NULL) {
		ret = 1;
		goto out;
	}

	memccpy(enclosure->sysfs_path, temp, '\0', PATH_MAX - 1);
	enclosure->sas_address = _get_sas_address(temp);
	enclosure->dev_path = _get_dev_sg(temp);

	fd = enclosure_open(enclosure);
	if (fd == -1) {
		ret = 1;
		goto out;
	}

	ret = ses_load_pages(fd, &enclosure->ses_pages);
	close(fd);
	if (ret)
		goto out;

	ret = ses_get_slots(&enclosure->ses_pages, &enclosure->slots, &enclosure->slots_count);
out:
	if (ret) {
		log_warning("failed to initialize enclosure_device %s\n", path);
		enclosure_device_fini(enclosure);
		enclosure = NULL;
	}
	return enclosure;
}

/*
 * The function returns memory allocated for fields of enclosure structure to
 * the system.
 */
void enclosure_device_fini(struct enclosure_device *enclosure)
{
	if (enclosure) {
		free(enclosure->slots);
		free(enclosure->dev_path);
		free(enclosure);
	}
}

int enclosure_open(const struct enclosure_device *enclosure)
{
	int fd = -1;

	if (enclosure->dev_path)
		fd = open(enclosure->dev_path, O_RDWR);

	return fd;
}
