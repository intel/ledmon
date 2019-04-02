/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2018 Intel Corporation.
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
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "ahci.h"
#include "config.h"
#include "utils.h"

/**
 * Time interval in nano seconds to wait before enclosure management message
 * is being sent to AHCI controller.
 */
#define EM_MSG_WAIT       1500000	/* 0.0015 seconds */

/**
 * This array maps IBPI pattern to value recognized by AHCI driver. The driver
 * uses this control number to issue SGPIO signals appropriately.
 */
static const unsigned int ibpi2sgpio[] = {
	[IBPI_PATTERN_REBUILD]        = 0x00480000,
	[IBPI_PATTERN_FAILED_DRIVE]   = 0x00400000,
	[IBPI_PATTERN_LOCATE]         = 0x00080000,
	[IBPI_PATTERN_UNKNOWN]        = 0x00000000,
	[IBPI_PATTERN_ONESHOT_NORMAL] = 0x00000000,
	[IBPI_PATTERN_NORMAL]         = 0x00000000,
	[IBPI_PATTERN_LOCATE_OFF]     = 0x00000000,
#ifdef DEBUG_IBPI
	[IBPI_PATTERN_DEGRADED]       = 0x00200000,
	[IBPI_PATTERN_FAILED_ARRAY]   = 0x00280000,
	[IBPI_PATTERN_HOTSPARE]       = 0x01800000,
	[IBPI_PATTERN_PFA]            = 0x01400000
#else
	[IBPI_PATTERN_DEGRADED]       = 0x00000000,
	[IBPI_PATTERN_FAILED_ARRAY]   = 0x00000000,
	[IBPI_PATTERN_HOTSPARE]       = 0x00000000,
	[IBPI_PATTERN_PFA]            = 0x00000000
#endif
};

/*
 * The function sends a LED control message to AHCI controller. It uses
 * SGPIO to control the LEDs. See ahci.h for details.
 */
int ahci_sgpio_write(struct block_device *device, enum ibpi_pattern ibpi)
{
	char temp[WRITE_BUFFER_SIZE];
	char path[PATH_MAX];
	char *sysfs_path = device->cntrl_path;
	const struct timespec waittime = {
		.tv_sec = 0,
		.tv_nsec = EM_MSG_WAIT
	};

	/* write only if state has changed */
	if (ibpi == device->ibpi_prev)
		return 1;

	if (sysfs_path == NULL)
		__set_errno_and_return(EINVAL);
	if ((ibpi < IBPI_PATTERN_NORMAL) || (ibpi > IBPI_PATTERN_LOCATE_OFF))
		__set_errno_and_return(ERANGE);

	sprintf(temp, "%u", ibpi2sgpio[ibpi]);

	snprintf(path, sizeof(path), "%s/em_message", sysfs_path);

	nanosleep(&waittime, NULL);
	return buf_write(path, temp) > 0;
}

#define SCSI_HOST "/scsi_host"
/*
 * The function return path to SATA port in sysfs tree. See ahci.h for details.
 */

char *ahci_get_port_path(const char *path)
{
	char *p;
	char tmp[PATH_MAX];
	char *buf;
	size_t buf_size;

	p = strstr(path, "/target");
	if (p == NULL)
		return NULL;

	if (sizeof(tmp) <= (p - path))
		return NULL;
	strncpy(tmp, path, p - path);
	tmp[p - path] = '\0';
	p = strrchr(tmp, PATH_DELIM);
	if (p == NULL)
		return NULL;

	buf_size = strlen(tmp) + strlen(p) + strlen(SCSI_HOST) + 1;
	buf = malloc(buf_size);
	if (buf == NULL)
		return NULL;

	snprintf(buf, buf_size, "%s%s%s", tmp, SCSI_HOST, p);
	return buf;
}
