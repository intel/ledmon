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
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "libled_private.h"
#include "ahci.h"
#include "block.h"
#include "config.h"
#include "dellssd.h"
#include "pci_slot.h"
#include "raid.h"
#include "scsi.h"
#include "tail.h"
#include "smp.h"
#include "status.h"
#include "sysfs.h"
#include "utils.h"
#include "vmdssd.h"
#include "npem.h"
#include "amd.h"

/* Global timestamp value. It shell be used to update a timestamp field of block
   device structure. See block.h for details. */
time_t timestamp = 0;

/**
 * @brief Determines if disk is attached directly or via expander
 */
int dev_directly_attached(const char *path)
{
	if (strstr(path, "/expander") == 0)
		return 1;
	return 0;
}

bool is_virt_nvme(const char * const name)
{
	/* Simple match by name */
	if (is_subpath(name, "nvme", 4) && strchr(name, 'c'))
		return true;
	return false;
}

/**
 * @brief Determines a send function.
 *
 * This is the internal function of 'block device' module. The function tries to
 * determine a LED management protocol based on controller type and the given
 * path to block device in sysfs tree. First it checks whether to use
 * the default send function. If not it tries to read the content
 * of em_message_type field from sysfs tree and determines
 * the LED control protocol.
 *
 * @param[in]    cntrl            type of a controller a device is connected to.
 * @param[in]    path             path to a block device in sysfs tree.
 *
 * @return Pointer to send message function if successful, otherwise the function
 *         returns the NULL pointer and it means either the controller does not
 *         support enclosure management or LED control protocol
 *         is not supported.
 */
static send_message_t _get_send_fn(struct cntrl_device *cntrl, const char *path)
{
	send_message_t result = NULL;

	if (cntrl->cntrl_type == LED_CNTRL_TYPE_AHCI) {
		result = ahci_sgpio_write;
	} else if (cntrl->cntrl_type == LED_CNTRL_TYPE_SCSI
		   && !dev_directly_attached(path)) {
		result = scsi_ses_write;
	} else if (cntrl->cntrl_type == LED_CNTRL_TYPE_SCSI
		   && dev_directly_attached(path)) {
		result = scsi_smp_fill_buffer;
	} else if (cntrl->cntrl_type == LED_CNTRL_TYPE_DELLSSD) {
		result = dellssd_write;
	} else if (cntrl->cntrl_type == LED_CNTRL_TYPE_VMD) {
		result = vmdssd_write;
	} else if (cntrl->cntrl_type == LED_CNTRL_TYPE_NPEM) {
		result = npem_write;
	} else if (cntrl->cntrl_type == LED_CNTRL_TYPE_AMD) {
		result = amd_write;
	}
	return result;
}

static int do_not_flush(struct block_device *device __attribute__ ((unused)))
{
	return 1;
}

static flush_message_t _get_flush_fn(struct cntrl_device *cntrl, const char *path)
{
	flush_message_t result = NULL;

	if (cntrl->cntrl_type == LED_CNTRL_TYPE_SCSI) {
		if (dev_directly_attached(path))
			result = scsi_smp_write_buffer;
		else
			result = scsi_ses_flush;
	} else {
		result = do_not_flush;
	}
	return result;
}

/**
 * @brief Determines a host path to block device.
 *
 * This is the internal function of 'block device' module. The function
 * determines a host path to block device in sysfs.
 *
 * @param[in]      path           path to block device in sysfs.
 * @param[in]      cntrl          controller device the block
 *                                device is connected to.
 *
 * @return Pointer to memory block containing a host path. The memory block
 *         should be freed if one don't need the content.
 */
static char *_get_host(char *path, struct cntrl_device *cntrl)
{
	char *result = NULL;

	if (cntrl->cntrl_type == LED_CNTRL_TYPE_SCSI)
		result = scsi_get_host_path(path, cntrl->sysfs_path);
	else if (cntrl->cntrl_type == LED_CNTRL_TYPE_AHCI)
		result = ahci_get_port_path(path);
	else if (cntrl->cntrl_type == LED_CNTRL_TYPE_DELLSSD)
		result = dellssd_get_path(cntrl->sysfs_path);
	else if (cntrl->cntrl_type == LED_CNTRL_TYPE_VMD)
		result = vmdssd_get_path(cntrl->sysfs_path);
	else if (cntrl->cntrl_type == LED_CNTRL_TYPE_NPEM)
		result = npem_get_path(cntrl->sysfs_path);
	else if (cntrl->cntrl_type == LED_CNTRL_TYPE_AMD)
		result = amd_get_path(path, cntrl->sysfs_path, cntrl->ctx);

	return result;
}

static int is_host_id_supported(const struct block_device *bd)
{
	if (!bd->cntrl)
		return 0;

	switch (bd->cntrl->cntrl_type) {
	case LED_CNTRL_TYPE_DELLSSD:
	case LED_CNTRL_TYPE_VMD:
	case LED_CNTRL_TYPE_NPEM:
		return 0;
	default:
		return 1;
	}
}
/**
 * @brief Determines a storage controller.
 *
 * This is the internal function of 'block device' module. The function gets
 * a pointer to controller structure the device is connected to.
 *
 * @param[in]      cntrl_list     pointer to list of supported controllers.
 * @param[in]      path           path to block device in sysfs tree.
 *
 * @return Pointer to controller structure if successful, otherwise the function
 *         returns NULL pointer. The NULL pointer means that block devices is
 *         connected to unsupported storage controller.
 */
struct cntrl_device *block_get_controller(const struct list *cntrl_list, char *path)
{
	struct cntrl_device *cntrl;
	struct cntrl_device *non_npem_cntrl = NULL;

	if (!cntrl_list || !path)
		return non_npem_cntrl;

	list_for_each(cntrl_list, cntrl) {
		if (cntrl) {
			if (strncmp(cntrl->sysfs_path, path,
				strnlen(cntrl->sysfs_path, PATH_MAX)) == 0) {
				if (cntrl->cntrl_type == LED_CNTRL_TYPE_NPEM)
					return cntrl;
				non_npem_cntrl = cntrl;
			}
		}
	}
	return non_npem_cntrl;
}

struct _host_type *block_get_host(struct cntrl_device *cntrl, int host_id)
{
	struct _host_type *hosts = NULL;

	if (!cntrl)
		return hosts;

	hosts = cntrl->hosts;
	while (hosts) {
		if (hosts->host_id == host_id)
			break;
		hosts = hosts->next;
	}
	return hosts;
}

/**
 * @brief Determine devnode for block device.
 *
 * This routine handles nvme multipath if namespace and nvme controller numbers matches, it does
 * not dereference dual-ports connections.
 */
static void block_set_devnode(struct block_device *device)
{
	struct stat st;
	char tmp[PATH_MAX];
	char *name = basename(device->sysfs_path);
	char *n, *c;
	int ret;

	if (is_virt_nvme(name)) {
		char buf[NAME_MAX];

		str_cpy(buf, name, NAME_MAX);
		c = strrchr(buf, 'c');
		n = strrchr(buf, 'n');

		if (!n || !c)
			return;
		*c = '\0';

		ret = snprintf(tmp, PATH_MAX, SYSTEM_DEV_DIR "/%s%s", buf, n);
	} else {
		ret = snprintf(tmp, PATH_MAX, SYSTEM_DEV_DIR "/%s", name);
	}

	if (ret >= PATH_MAX)
		return;

	if (stat(tmp, &st) == 0)
		str_cpy(device->devnode, tmp, PATH_MAX);
}

struct block_device *get_block_device_from_sysfs_path(struct led_ctx *ctx, char *sub_path,
						      bool sub_path_to_end)
{
	struct block_device *device;

	list_for_each(sysfs_get_block_devices(ctx), device) {
			char *start_loc;
			if ((start_loc = strstr(device->sysfs_path, sub_path))) {
				char following = start_loc[strnlen(sub_path, PATH_MAX)];
				if (following == '/' || following == '\n' || following == '\0')
					return device;
				if (sub_path_to_end)
					return device;
			}
	}

	return NULL;
}

/*
 * Allocates a new block device structure. See block.h for details.
 */
struct block_device *block_device_init(const struct list *cntrl_list, const char *path)
{
	struct cntrl_device *cntrl;
	char link[PATH_MAX];
	char *host = NULL;
	struct block_device *device = NULL;
	int host_id = -1;
	char *host_name = NULL;
	struct _host_type *hosts = NULL;

	if (!realpath(path, link))
		goto error;

	cntrl = block_get_controller(cntrl_list, link);
	if (!cntrl)
		goto error;

	if (cntrl->cntrl_type == LED_CNTRL_TYPE_VMD &&
			!vmdssd_find_pci_slot(cntrl->ctx, link))
		goto error;
	host = _get_host(link, cntrl);
	if (host == NULL)
		goto error;
	host_name = get_path_hostN(link);
	if (host_name) {
		if (sscanf(host_name, "host%d", &host_id) != 1)
			host_id = -1;
		free(host_name);
	}

	device = calloc(1, sizeof(*device));
	if (!device)
		goto error;

	hosts = cntrl->hosts;
	device->cntrl = cntrl;
	device->sysfs_path = strdup(link);
	if (!device->sysfs_path)
		goto error;
	device->cntrl_path = host;
	block_set_devnode(device);
	device->ibpi = LED_IBPI_PATTERN_UNKNOWN;
	device->ibpi_prev = LED_IBPI_PATTERN_NONE;
	device->send_fn = _get_send_fn(cntrl, link);
	if (!device->send_fn)
		goto error;
	device->flush_fn = _get_flush_fn(cntrl, link);
	device->timestamp = timestamp;
	device->host = NULL;
	device->host_id = host_id;
	device->encl_index = -1;
	device->raid_dev = NULL;
	while (hosts) {
		if (hosts->host_id == host_id) {
			device->host = hosts;
			break;
		}
		hosts = hosts->next;
	}
	if (cntrl && cntrl->cntrl_type == LED_CNTRL_TYPE_SCSI) {
		device->phy_index = cntrl_init_smp(link, cntrl);
		if (!dev_directly_attached(link)
				&& !scsi_get_enclosure(cntrl->ctx, device)) {
			lib_log(cntrl->ctx, LED_LOG_LEVEL_DEBUG,
				"Device initialization failed for '%s'", path);
			goto error;
		}
	}

	return device;
error:
	free(host);
	if (device) {
		free(device->sysfs_path);
		free(device);
	}
	return NULL;
}

/**
 * Frees memory allocated for block device structure. See block.h for details.
 */
void block_device_fini(struct block_device *device)
{
	if (device) {
		if (device->sysfs_path)
			free(device->sysfs_path);

		if (device->cntrl_path)
			free(device->cntrl_path);

		if (device->raid_dev)
			raid_device_fini(device->raid_dev);

		free(device);
	}
}

/*
 * Duplicates a block device structure. See block.h for details.
 */
struct block_device *block_device_duplicate(struct block_device *block)
{
	struct block_device *result = NULL;

	if (block) {
		result = calloc(1, sizeof(*result));
		if (result) {
			result->sysfs_path = strdup(block->sysfs_path);
			result->cntrl_path = strdup(block->cntrl_path);

			if (!result->sysfs_path || !result->cntrl_path) {
				free(result->sysfs_path);
				free(result->cntrl_path);
				free(result);
				return NULL;
			}

			if (block->ibpi != LED_IBPI_PATTERN_UNKNOWN)
				result->ibpi = block->ibpi;
			else
				result->ibpi = LED_IBPI_PATTERN_ONESHOT_NORMAL;
			result->ibpi_prev = block->ibpi_prev;
			result->send_fn = block->send_fn;
			result->flush_fn = block->flush_fn;
			result->timestamp = block->timestamp;
			result->cntrl = block->cntrl;
			result->host = block->host;
			result->host_id = block->host_id;
			result->phy_index = block->phy_index;
			result->encl_index = block->encl_index;
			result->enclosure = block->enclosure;
			result->raid_dev =
				raid_device_duplicate(block->raid_dev);
		}
	}
	return result;
}

int block_compare(const struct block_device *bd_old,
		  const struct block_device *bd_new)
{
	int i = 0;

	if (is_host_id_supported(bd_old) && bd_old->host_id == -1) {
		lib_log(bd_old->cntrl->ctx, LED_LOG_LEVEL_DEBUG,
			"Device %s : No host_id!", strstr(bd_old->sysfs_path, "host"));
		return 0;
	}
	if (is_host_id_supported(bd_new) && bd_new->host_id == -1) {
		lib_log(bd_old->cntrl->ctx, LED_LOG_LEVEL_DEBUG,
			"Device %s : No host_id!", strstr(bd_new->sysfs_path, "host"));
		return 0;
	}

	if (bd_old->cntrl->cntrl_type != bd_new->cntrl->cntrl_type)
		return 0;

	switch (bd_old->cntrl->cntrl_type) {
	case LED_CNTRL_TYPE_AHCI:
		/* Missing support for port multipliers. Compare just hostX. */
		i = (bd_old->host_id == bd_new->host_id);
		break;

	case LED_CNTRL_TYPE_SCSI:
		/* Host and phy is not enough. They might be DA or EA. */
		if (dev_directly_attached(bd_old->sysfs_path) &&
		    dev_directly_attached(bd_new->sysfs_path)) {
			/* Just compare host & phy */
			i = (bd_old->host_id == bd_new->host_id) &&
			    (bd_old->phy_index == bd_new->phy_index);
			break;
		}
		if (!dev_directly_attached(bd_old->sysfs_path) &&
		    !dev_directly_attached(bd_new->sysfs_path)) {
			/* Both expander attached */
			i = (bd_old->host_id == bd_new->host_id) &&
			    (bd_old->phy_index == bd_new->phy_index);
			i = i && (bd_old->enclosure == bd_new->enclosure);
			i = i && (bd_old->encl_index == bd_new->encl_index);
			break;
		}
		/* */
		break;

	case LED_CNTRL_TYPE_VMD:
		/* compare names and address of the drive */
		i = (strcmp(bd_old->sysfs_path, bd_new->sysfs_path) == 0);
		if (!i) {
			struct pci_slot *old_slot, *new_slot;

			old_slot = vmdssd_find_pci_slot(bd_old->cntrl->ctx, bd_old->sysfs_path);
			new_slot = vmdssd_find_pci_slot(bd_old->cntrl->ctx, bd_new->sysfs_path);
			if (old_slot && new_slot)
				i = (strcmp(old_slot->address, new_slot->address) == 0);
		}
		break;

	case LED_CNTRL_TYPE_NPEM:
		/* check controller to determine slot. */
		i = (strcmp(bd_old->cntrl_path, bd_new->cntrl_path) == 0);
		break;

	case LED_CNTRL_TYPE_DELLSSD:
	default:
		/* Just compare names */
		i = (strcmp(bd_old->sysfs_path, bd_new->sysfs_path) == 0);
		break;
	}
	return i;
}
