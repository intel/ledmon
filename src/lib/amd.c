/*
 * AMD LED control
 * Copyright (C) 2023, Advanced Micro Devices, Inc.
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
#include "led/libled.h"
#include "list.h"
#include "utils.h"
#include "amd.h"
#include "amd_sgpio.h"
#include "amd_ipmi.h"
#include "libled_private.h"

enum amd_led_interfaces amd_interface = AMD_INTF_UNSET;
enum amd_ipmi_platforms amd_ipmi_platform = AMD_PLATFORM_UNSET;

int _find_file_path(const char *start_path, const char *filename,
		    char *path, size_t path_len, struct led_ctx *ctx)
{
	int rc, found;
	struct stat sbuf;
	struct list dir;
	char *dir_name;
	const char *dir_path;

	rc = scan_dir(start_path, &dir);
	if (rc) {
		lib_log(ctx, LED_LOG_LEVEL_INFO, "Failed to scan %s", start_path);
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
						path, path_len, ctx);
			if (found)
				break;
		}
	}

	list_erase(&dir);
	return found;
}

/* For AMD platforms to use IPMI for LED control we need to know
 * the platform we're running on. This enables us to select the
 * proper channel and tail address when making IPMI requests.
 * Platforms not checked for IPMI enablement default to using SGPIO.
 */
int amd_em_enabled(const char *path, struct led_ctx *ctx)
{
	char *platform;
	int rc;
	char buf[BUF_SZ_SM];

	/* Default to SGPIO interface */
	amd_interface = AMD_INTF_SGPIO;

	platform = get_text_to_dest("/sys/class/dmi/id", "product_name", buf, sizeof(buf));
	if (!platform)
		return 0;

	/* Check IPMI platforms */
	if (!strncmp(platform, "ETHANOL_X", 9)) {
		amd_interface = AMD_INTF_IPMI;
		amd_ipmi_platform = AMD_PLATFORM_ETHANOL_X;
	} else if (!strncmp(platform, "DAYTONA_X", 9)) {
		amd_interface = AMD_INTF_IPMI;
		amd_ipmi_platform = AMD_PLATFORM_DAYTONA_X;
	}

	switch (amd_interface) {
	case AMD_INTF_SGPIO:
		rc = _amd_sgpio_em_enabled(path, ctx);
		break;
	case AMD_INTF_IPMI:
		rc = _amd_ipmi_em_enabled(path, ctx);
		break;
	default:
		lib_log(ctx, LED_LOG_LEVEL_ERROR,
			"Unknown interface for AMD %s platform\n", platform);
		rc = -EOPNOTSUPP;
		break;
	}

	return rc;
}

int amd_write(struct block_device *device, enum led_ibpi_pattern ibpi)
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
		rc = _amd_ipmi_write(device, ibpi);
		break;
	case AMD_INTF_UNSET:
	default:
		lib_log(device->cntrl->ctx, LED_LOG_LEVEL_ERROR,
			"Unsupported AMD interface %u\n", amd_interface);
		rc = -EOPNOTSUPP;
		break;
	}

	return rc;
}

char *amd_get_path(const char *cntrl_path, const char *sysfs_path, struct led_ctx *ctx)
{
	char *path;

	switch (amd_interface) {
	case AMD_INTF_SGPIO:
		path = _amd_sgpio_get_path(sysfs_path, ctx);
		break;
	case AMD_INTF_IPMI:
		path = _amd_ipmi_get_path(cntrl_path, sysfs_path);
		break;
	case AMD_INTF_UNSET:
	default:
		lib_log(ctx, LED_LOG_LEVEL_ERROR, "Unsupported AMD interface\n");
		path = NULL;
		break;
	}

	return path;
}
