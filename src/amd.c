/*
 * AMD LED control
 * Copyright (C) 2019, Advanced Micro Devices, Inc.
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <sys/file.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "config.h"
#include "ibpi.h"
#include "list.h"
#include "utils.h"
#include "amd.h"
#include "amd_sgpio.h"

enum amd_led_interfaces amd_interface = AMD_INTF_UNSET;
enum amd_platforms amd_platform = AMD_PLATFORM_UNSET;

int _find_file_path(const char *start_path, const char *filename,
		    char *path, size_t path_len)
{
	int rc, found;
	struct stat sbuf;
	struct list dir;
	char *dir_name;
	const char *dir_path;

	rc = scan_dir(start_path, &dir);
	if (rc) {
		log_info("Failed to scan %s", start_path);
		return 0;
	}

	found = 0;
	list_for_each(&dir, dir_path) {
		dir_name = strrchr(dir_path, '/');
		if (!dir_name)
			continue;

		/* skip past the leading '/' */
		dir_name++;

		if (strncmp(dir_name, filename, strlen(filename)) == 0) {
			char tmp[PATH_MAX + 1];

			strncpy(tmp, dir_path, path_len);
			snprintf(path, path_len, "%s", dirname(tmp));

			found = 1;
			break;
		}

		if (lstat(dir_path, &sbuf) == -1)
			continue;

		if (S_ISDIR(sbuf.st_mode)) {
			found = _find_file_path(dir_path, filename,
						path, path_len);
			if (found)
				break;
		}
	}

	list_erase(&dir);
	return found;
}

static void _get_amd_led_interface(void)
{
	char *name;

	name = get_text("/sys/class/dmi/id", "product_name");
	if (!name)
		return;

	if (!strncmp(name, "ETHANOL_X", 9)) {
		amd_interface = AMD_INTF_IPMI;
		amd_platform = AMD_PLATFORM_ETHANOL_X;
	} else if (!strncmp(name, "DAYTONA_X", 9)) {
		amd_interface = AMD_INTF_IPMI;
		amd_platform = AMD_PLATFORM_DAYTONA_X;
	} else if (!strncmp(name, "GRANDSTAND", 10)) {
		amd_interface = AMD_INTF_SGPIO;
		amd_platform = AMD_PLATFORM_GRANDSTAND;
	} else if (!strncmp(name, "Speedway", 8)) {
		amd_interface = AMD_INTF_SGPIO;
		amd_platform = AMD_PLATFORM_SPEEDWAY;
	}

	free(name);
}

int amd_em_enabled(const char *path)
{
	int rc;

	_get_amd_led_interface();

	switch (amd_interface) {
	case AMD_INTF_SGPIO:
		rc = _amd_sgpio_em_enabled(path);
		break;
	case AMD_INTF_IPMI:
		log_error("IPMI not supported\n");
		rc = -EOPNOTSUPP;
		break;
	default:
		log_error("Unsupported AMD interface\n");
		rc = -EOPNOTSUPP;
		break;
	}

	return rc;
}

int amd_write(struct block_device *device, enum ibpi_pattern ibpi)
{
	int rc;

	/* write only if state has changed */
	if (ibpi == device->ibpi_prev)
		return 1;

	switch (amd_interface) {
	case AMD_INTF_SGPIO:
		rc = _amd_sgpio_write(device, ibpi);
		break;
	case AMD_INTF_IPMI:
		log_error("IPMI not supported\n");
		rc = -EOPNOTSUPP;
		break;
	case AMD_INTF_UNSET:
	default:
		log_error("Unsupported AMD interface\n");
		rc = -EOPNOTSUPP;
		break;
	}

	return rc;
}

char *amd_get_path(const char *cntrl_path)
{
	char *path;

	switch (amd_interface) {
	case AMD_INTF_SGPIO:
		path = _amd_sgpio_get_path(cntrl_path);
		break;
	case AMD_INTF_IPMI:
		log_error("IPMI not supported\n");
		path = NULL;
		break;
	case AMD_INTF_UNSET:
	default:
		log_error("Unsupported AMD interface\n");
		path = NULL;
		break;
	}

	return path;
}
