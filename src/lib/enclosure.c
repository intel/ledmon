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
#include <fcntl.h>
#include <inttypes.h>
#include <libgen.h>
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
#include "scsi.h"
#include "sysfs.h"
#include "utils.h"
#include "libled_private.h"

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
	char buf[PATH_MAX];
	char *p, *s;
	char *tmp = strdup(path);

	if (!tmp)
		return 0;

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

static struct ses_slot *find_enclosure_slot_by_index(struct enclosure_device *encl, int index)
{
	for (int i = 0; i < encl->slots_count; i++) {
		if (encl->slots[i].index == index)
			return &encl->slots[i];
	}
	return NULL;
}

static struct block_device *enclosure_get_block_device(struct enclosure_device *encl, int index)
{
	struct ses_slot *s_slot = find_enclosure_slot_by_index(encl, index);

	if (!s_slot) {
		lib_log(encl->ctx, LED_LOG_LEVEL_ERROR,
			"SCSI: Unable to locate slot in enclosure %d\n", index);
		return NULL;
	}

	return locate_block_by_sas_addr(encl->ctx, s_slot->sas_addr);
}

/*
 * Re-loads the ses hardware state for this enclosure, to allow refreshing the
 * state after the hardware has be written.
 */
int enclosure_reload(struct enclosure_device * enclosure)
{
	int fd, ret;

	fd = enclosure_open(enclosure);
	if (fd == -1) {
		return 1;
	}

	ret = ses_load_pages(fd, &enclosure->ses_pages, enclosure->ctx);
	close(fd);
	if (ret != 0)
		return ret;

	ret = ses_get_slots(&enclosure->ses_pages, &enclosure->slots, &enclosure->slots_count);
	if (ret)
		return ret;

	/* If there is an associated block device with a slot, we need to update the block ibpi */
	for (int i = 0; i < enclosure->slots_count; i++) {
		struct block_device *bd = enclosure_get_block_device(enclosure, i);
		struct ses_slot *s_slot = find_enclosure_slot_by_index(enclosure, i);

		if (bd && s_slot) {
			bd->ibpi_prev = bd->ibpi;
			bd->ibpi = s_slot->ibpi_status;
		}
	}
	return 0;
}

/*
 * Allocates memory for enclosure device structure and initializes fields of
 * the structure.
 */
struct enclosure_device *enclosure_device_init(const char *path, struct led_ctx *ctx)
{
	char temp[PATH_MAX] = "\0";
	struct enclosure_device *enclosure;
	int ret;

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
	enclosure->ctx = ctx;

	ret = enclosure_reload(enclosure);
out:
	if (ret) {
		lib_log(ctx, LED_LOG_LEVEL_WARNING,
			"failed to initialize enclosure_device %s\n", path);
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

enum led_ibpi_pattern enclosure_get_state(struct slot_property *sp)
{
	int index = sp->slot_spec.ses.slot_num;
	struct enclosure_device *encl = sp->slot_spec.ses.encl;
	struct ses_slot *s_slot;

	s_slot = find_enclosure_slot_by_index(encl, index);
	if (!s_slot) {
		lib_log(encl->ctx, LED_LOG_LEVEL_ERROR,
			"SCSI: Unable to locate slot in enclosure %d\n", index);
		return LED_IBPI_PATTERN_UNKNOWN;
	}
	return s_slot->ibpi_status;
}

const struct slot_property_common ses_slot_common = {
	.cntrl_type = LED_CNTRL_TYPE_SCSI,
	.get_state_fn = enclosure_get_state,
	.set_slot_fn = enclosure_set_state
};

struct slot_property *enclosure_slot_property_init(struct enclosure_device *encl, int slot_idx)
{
	struct slot_property *result = NULL;
	struct ses_slot *slot_ptr = find_enclosure_slot_by_index(encl, slot_idx);

	result = calloc(1, sizeof(struct slot_property));
	if (result == NULL)
		return NULL;

	result->bl_device = enclosure_get_block_device(encl, slot_idx);
	result->slot_spec.ses.encl = encl;
	result->slot_spec.ses.slot_num = slot_idx;
	snprintf(result->slot_id, PATH_MAX, "%s-%d", encl->dev_path, slot_idx);
	result->c = &ses_slot_common;

	/* If we have an associated block device, set its ibpi value */
	if (result->bl_device && slot_ptr)
		result->bl_device->ibpi = slot_ptr->ibpi_status;

	return result;
}

status_t enclosure_set_state(struct slot_property *sp, enum led_ibpi_pattern state)
{
	struct enclosure_device *enclosure_device = sp->slot_spec.ses.encl;
	int index = sp->slot_spec.ses.slot_num;

	int rc = scsi_ses_write_enclosure(enclosure_device, index, state);
	if (rc != 0) {
		lib_log(enclosure_device->ctx, LED_LOG_LEVEL_ERROR,
			"SCSI: ses write failed %d\n", rc);
		return STATUS_FILE_WRITE_ERROR;
	}

	rc = scsi_ses_flush_enclosure(enclosure_device);
	if (rc != 0) {
		lib_log(enclosure_device->ctx, LED_LOG_LEVEL_ERROR,
			"SCSI: ses flush enclosure failed %d\n", rc);
		return STATUS_FILE_WRITE_ERROR;
	}

	// Reload from hardware to report actual current state.
	rc = enclosure_reload(enclosure_device);
	if (rc != 0) {
		lib_log(enclosure_device->ctx, LED_LOG_LEVEL_ERROR,
			"SCSI: ses enclosure reload error %d\n", rc);
		return STATUS_FILE_READ_ERROR;
	}

	/* If we have an associated block device, update its ibpi value */
	if (sp->bl_device) {
		sp->bl_device->ibpi_prev = sp->bl_device->ibpi;
		sp->bl_device->ibpi = enclosure_get_state(sp);
	}
	return STATUS_SUCCESS;
}
