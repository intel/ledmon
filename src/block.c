/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2017 Intel Corporation.
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "ahci.h"
#include "block.h"
#include "config.h"
#include "dellssd.h"
#include "list.h"
#include "raid.h"
#include "scsi.h"
#include "slave.h"
#include "smp.h"
#include "status.h"
#include "sysfs.h"
#include "utils.h"
#include "vmdssd.h"

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

	if (cntrl->cntrl_type == CNTRL_TYPE_AHCI) {
		result = ahci_sgpio_write;
	} else if (cntrl->cntrl_type == CNTRL_TYPE_SES) {
		result = scsi_ses_write;
	} else if (cntrl->cntrl_type == CNTRL_TYPE_SCSI
		   && dev_directly_attached(path)) {
		result = scsi_smp_fill_buffer;
	} else if (cntrl->cntrl_type == CNTRL_TYPE_DELLSSD) {
		result = dellssd_write;
	} else if (cntrl->cntrl_type == CNTRL_TYPE_VMD) {
		result = vmdssd_write;
	}
	return result;
}

static int do_not_flush(struct block_device *device)
{
	return 1;
}

static flush_message_t _get_flush_fn(struct cntrl_device *cntrl, const char *path)
{
	flush_message_t result = NULL;

	if (cntrl->cntrl_type == CNTRL_TYPE_SCSI && dev_directly_attached(path))
		result = scsi_smp_write_buffer;
	else
		result = do_not_flush;
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

	if (cntrl->cntrl_type == CNTRL_TYPE_SCSI
		|| cntrl->cntrl_type == CNTRL_TYPE_SES)
		result = scsi_get_slot_path(path, cntrl->sysfs_path);
	else if (cntrl->cntrl_type == CNTRL_TYPE_AHCI)
		result = ahci_get_port_path(path);
	else if (cntrl->cntrl_type == CNTRL_TYPE_DELLSSD)
		result = dellssd_get_path(path, cntrl->sysfs_path);
	else if (cntrl->cntrl_type == CNTRL_TYPE_VMD)
		result = vmdssd_get_path(path, cntrl->sysfs_path);
	return result;
}

/**
 * @brief Checks if block device is connected to the given controller.
 *
 * This is the internal function of 'block device' module. It is design to be
 * used as test argument in list_first_that() function. The function checks if
 * a block device is connected to the given storage controller.
 *
 * @param[in]      cntrl          pointer to controller structure to compare to.
 * @param[in]      path           real path to block device in sysfs.
 *
 * @return 1 if a block device is connected to a controller, otherwise the
 *         function will return 0.
 */
static int _compare(struct cntrl_device *cntrl, const char *path)
{
	return (strncmp(cntrl->sysfs_path, path, strlen(cntrl->sysfs_path)) ==
		0);
}

static int is_dellssd(struct block_device *bd)
{
	return (bd->cntrl && bd->cntrl->cntrl_type == CNTRL_TYPE_DELLSSD);
}

static int is_vmd(struct block_device *bd)
{
	return ((bd->cntrl && bd->cntrl->cntrl_type == CNTRL_TYPE_VMD) ||
		 bd->pci_slot);
}

static enum cntrl_type _get_cntrl_type(struct block_device *bd)
{
	enum cntrl_type cntrl = CNTRL_TYPE_UNKNOWN;

	if (!bd->cntrl) {
		if (bd->pci_slot)
			/* This is possible only for VMD */
			cntrl = CNTRL_TYPE_VMD;
		else
			log_debug("Device %s : No ctrl dev!",
				  strstr(bd->sysfs_path, "host"));
	} else {
		cntrl = bd->cntrl->cntrl_type;
	}

	return cntrl;
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
struct cntrl_device *block_get_controller(void *cntrl_list, char *path)
{
	return list_first_that(cntrl_list, _compare, path);
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

/*
 * Allocates a new block device structure. See block.h for details.
 */
struct block_device *block_device_init(void *cntrl_list, const char *path)
{
	struct cntrl_device *cntrl;
	char link[PATH_MAX];
	char *host = NULL;
	struct block_device *device = NULL;
	struct pci_slot *pci_slot = NULL;
	send_message_t send_fn = NULL;
	flush_message_t flush_fn = NULL;
	int host_id = -1;
	char *host_name;

	if (realpath(path, link)) {
		pci_slot = vmdssd_find_pci_slot(link);
		cntrl = block_get_controller(cntrl_list, link);
		if (cntrl != NULL) {
			if (cntrl->cntrl_type == CNTRL_TYPE_VMD && !pci_slot)
				return NULL;
			host = _get_host(link, cntrl);
			if (host == NULL)
				return NULL;
			host_name = get_path_hostN(link);
			if (host_name) {
				sscanf(host_name, "host%d", &host_id);
				free(host_name);
			}
			flush_fn = _get_flush_fn(cntrl, link);
			send_fn = _get_send_fn(cntrl, link);
			if (send_fn  == NULL) {
				free(host);
				return NULL;
			}
		} else {
			if (!pci_slot) {
				return NULL;
			} else {
				/* New VMD drive - no cntrl in system */
				flush_fn = NULL;
				send_fn = vmdssd_write;
			}
		}

		device = calloc(1, sizeof(*device));
		if (device) {
			struct _host_type *hosts = cntrl ? cntrl->hosts : NULL;

			device->cntrl = cntrl;
			device->sysfs_path = strdup(link);
			device->pci_slot = pci_slot;
			device->cntrl_path = host;
			device->ibpi = IBPI_PATTERN_UNKNOWN;
			device->ibpi_prev = IBPI_PATTERN_NONE;
			device->send_fn = send_fn;
			device->flush_fn = flush_fn;
			device->timestamp = timestamp;
			device->host = NULL;
			device->host_id = host_id;
			device->encl_index = -1;
			while (hosts) {
				if (hosts->host_id == host_id) {
					device->host = hosts;
					break;
				}
				hosts = hosts->next;
			}
			if (cntrl && (cntrl->cntrl_type == CNTRL_TYPE_SCSI
				|| cntrl->cntrl_type == CNTRL_TYPE_SES)) {
				device->phy_index = cntrl_init_smp(link, cntrl);
				if (!dev_directly_attached(link)
						&& !scsi_get_enclosure(device)) {
					log_warning("Device initialization failed for '%s'",
							path);
					free(device->sysfs_path);
					free(device->cntrl_path);
					free(device);
					device = NULL;
				}
			}
		} else if (host) {
			free(host);
		}
	}
	return device;
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

		/* free(device); */
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
			if (block->ibpi != IBPI_PATTERN_UNKNOWN)
				result->ibpi = block->ibpi;
			else
				result->ibpi = IBPI_PATTERN_ONESHOT_NORMAL;
			result->ibpi_prev = block->ibpi_prev;
			result->send_fn = block->send_fn;
			result->flush_fn = block->flush_fn;
			result->timestamp = block->timestamp;
			result->cntrl = block->cntrl;
			result->pci_slot = block->pci_slot;
			result->host = block->host;
			result->host_id = block->host_id;
			result->phy_index = block->phy_index;
			result->encl_index = block->encl_index;
			strcpy(result->encl_dev, block->encl_dev);
		}
	}
	return result;
}

int block_compare(struct block_device *bd_old, struct block_device *bd_new)
{
	int i = 0;
	enum cntrl_type cntrl_old = CNTRL_TYPE_UNKNOWN;
	enum cntrl_type cntrl_new = CNTRL_TYPE_UNKNOWN;

	if (!is_dellssd(bd_old) && !is_vmd(bd_old) && bd_old->host_id == -1) {
		log_debug("Device %s : No host_id!",
			  strstr(bd_old->sysfs_path, "host"));
		return 0;
	}
	if (!is_dellssd(bd_new) && !is_vmd(bd_new) && bd_new->host_id == -1) {
		log_debug("Device %s : No host_id!",
			  strstr(bd_new->sysfs_path, "host"));
		return 0;
	}

	cntrl_old = _get_cntrl_type(bd_old);
	cntrl_new = _get_cntrl_type(bd_new);

	if (cntrl_old == CNTRL_TYPE_UNKNOWN || cntrl_old != cntrl_new)
		return 0;

	switch (cntrl_old) {
	case CNTRL_TYPE_AHCI:
		/* Missing support for port multipliers. Compare just hostX. */
		i = (bd_old->host_id == bd_new->host_id);
		break;

	case CNTRL_TYPE_SCSI:
	case CNTRL_TYPE_SES:
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
			i = i && (bd_old->encl_index == bd_new->encl_index);
			break;
		}
		/* */
		break;

	case CNTRL_TYPE_VMD:
		/* compare names and address of the drive */
		i = (strcmp(bd_old->sysfs_path, bd_new->sysfs_path) == 0);
		if (!i && bd_old->pci_slot && bd_new->pci_slot)
			i = (strcmp(bd_old->pci_slot->address,
				 bd_new->pci_slot->address) == 0);
		break;

	case CNTRL_TYPE_DELLSSD:
	default:
		/* Just compare names */
		i = (strcmp(bd_old->sysfs_path, bd_new->sysfs_path) == 0);
		break;
	}
	return i;
}
