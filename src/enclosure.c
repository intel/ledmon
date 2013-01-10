/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2013 Intel Corporation.
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

#include <config.h>

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

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
static uint64_t _get_sas_address(const char *path)
{
	char tmp[PATH_MAX], buf[BUFFER_MAX];
	char *p, *s;

	str_cpy(tmp, path, PATH_MAX);
	if ((p = strstr(tmp, "/expander")) == NULL) {
		return 0;
	}
	if ((s = strchr(p + 1, PATH_DELIM)) == NULL) {
		return 0;
	}
	*s = '\0';

	str_cpy(buf, p, s - p + 1);
	str_cat(tmp, "/sas_device", PATH_MAX);
	str_cat(tmp, buf, PATH_MAX);

	return get_uint64(tmp, 0, "sas_address");
}

/*
 * Allocates memory for enclosure device structure and initializes fields of
 * the structure.
 */
struct enclosure_device *enclosure_device_init(const char *path)
{
	char temp[PATH_MAX];
	struct enclosure_device *result = NULL;

	if (realpath(path, temp)) {
		if ((result = malloc(sizeof(struct enclosure_device))) == NULL) {
			return NULL;
		}
		result->sysfs_path = str_dup(temp);
		result->sas_address = _get_sas_address(temp);
	}
	return result;
}

/*
 * The function returns memory allocated for fields of enclosure structure to
 * the system.
 */
void enclosure_device_fini(struct enclosure_device *enclosure)
{
	if (enclosure) {
		if (enclosure->sysfs_path) {
			free(enclosure->sysfs_path);
		}
		/* free(enclosure); */
	}
}
