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


#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "block.h"
#include "cntrl.h"
#include "config.h"
#include "config_file.h"
#include "enclosure.h"
#include "ibpi.h"
#include "list.h"
#include "pci_slot.h"
#include "raid.h"
#include "slave.h"
#include "stdio.h"
#include "sysfs.h"
#include "utils.h"

/**
 */
#define SYSFS_CLASS_BLOCK       "/sys/block"
#define SYSFS_CLASS_ENCLOSURE   "/sys/class/enclosure"
#define SYSFS_PCI_DEVICES       "/sys/bus/pci/devices"
#define SYSFS_PCI_SLOTS         "/sys/bus/pci/slots"

/**
 * This is internal variable global to sysfs module only. It is a list of
 * block devices registered in the system. Use sysfs_init()
 * function to initialize the variable. Use sysfs_scan() function to populate
 * the list. Use sysfs_reset() function to delete the content of the list.
 */
static struct list sysfs_block_list;

/**
 * This is internal variable global to sysfs module only. It is a list of
 * RAID volumes registered in the system. Use sysfs_init()
 * function to initialize the variable. Use sysfs_scan() function to populate
 * the list. Use sysfs_reset() function to delete the content of the list.
 */
static struct list volum_list;

/**
 * This is internal variable global to sysfs module only. It is a list of
 * storage controller devices registered in the system and
 * supported by Intel(R) Enclosure LEDs Control Utility. Use sysfs_init()
 * function to initialize the variable. Use sysfs_scan() function to populate
 * the list. Use sysfs_reset() function to delete the content of the list.
 */
static struct list cntrl_list;

/**
 * This is internal variable global to sysfs module only. It is a list of
 * slave devices registered in the system. Use sysfs_init()
 * function to initialize the variable. Use sysfs_scan() function to populate
 * the list. Use sysfs_reset() function to delete the content of the list.
 */
static struct list slave_list;

/**
 * This is internal variable global to sysfs module only. It is a list of
 * RAID containers registered in the system. Use sysfs_init()
 * function to initialize the variable. Use sysfs_scan() function to populate
 * the list. Use sysfs_reset() function to delete the content of the list.
 */
static struct list cntnr_list;

/**
 * This is internal variable global to sysfs module only. It is a to list of
 * enclosures registered in the system.
 */
static struct list enclo_list;

/**
 * This is internal variable global to sysfs module only. It is a list of
 * PCI slots registered in the system. Use sysfs_init()
 * function to initialize the variable. Use sysfs_scan() function to populate
 * the list. Use sysfs_reset() function to delete the content of the list.
 */
static struct list slots_list;

/**
 * @brief Determine device type.
 *
 * This is internal function of sysfs module. The function determines a type of
 * RAID device either it is VOLUME or CONTAINER device. The information required
 * if read from 'metadata_version' attribute from sysfs. External and native
 * RAID devices are reported as volumes with no distinction between both types.
 *
 * @param[in]      path           Path to RAID device in sysfs tree.
 *
 * @return Type of RAID device if successful, otherwise DEVICE_TYPE_UNKNOWN.
 */
static enum device_type _get_device_type(const char *path)
{
	enum device_type result = DEVICE_TYPE_UNKNOWN;
	char *p = get_text(path, "md/metadata_version");
	if (p != NULL) {
		if (strlen(p) > 0) {
			if (strncmp(p, "external:", 9) == 0) {
				if (p[9] == '/')
					result = DEVICE_TYPE_VOLUME;
				else
					result = DEVICE_TYPE_CONTAINER;
			} else {
				result = DEVICE_TYPE_VOLUME;
			}
		}
		free(p);
	}
	return result;
}

/**
 * @brief Gets device major and minor.
 *
 * This is internal function of sysfs module. The function retrieves major and
 * minor of device from sysfs attribute. Each block device has 'dev' attribute
 * where major and minor separated by colon are stored.
 *
 * @param[in]      path           Path to block device in sysfs tree.
 * @param[in]      d_id           Placeholder where major and minor of device
 *                                will be stored. If this argument is NULL the
 *                                behavior of function is unspecified.
 *
 * @return The function does not return a value.
 */
static void _get_id(const char *path, struct device_id *d_id)
{
	char temp[PATH_MAX];

	str_cpy(temp, path, PATH_MAX);
	str_cat(temp, "/dev", PATH_MAX);
	get_id(temp, d_id);
}

/**
 * @brief Adds slave device to RAID volume.
 *
 * This is internal function of sysfs module. The function puts slave device on
 * list of slave devices of RAID volume. The memory is allocated and structure
 * fields populated. RAID device is link to slave device.
 *
 * @param[in]      path           Path to 'md' directory of RAID device in sysfs
 *                                tree.
 * @param[in]      raid           Pointer to RAID device structure corresponding
 *                                to 'path' argument.
 *
 * @return The function does not return a value.
 */
static void _slave_vol_add(const char *path, struct raid_device *raid)
{
	struct slave_device *device;

	char *t = rindex(path, '/');
	if (strncmp(t + 1, "dev-", 4) == 0) {
		device = slave_device_init(path, &sysfs_block_list);
		if (device) {
			device->raid = raid;
			list_append(&slave_list, device);
		}
	}
}

/**
 * @brief Matches two slave devices.
 *
 * This is internal function of sysfs module. The function compares two slave
 * devices together. Slave device is identical to other slave device if both if
 * both devices are associated with the same block device.
 *
 * @param[in]      s1             Pointer to slave device to compare with.
 * @param[in]      s2             Pointer to slave to compare to.
 *
 * @return 1 if slave devices matches, otherwise the function returns 0.
 */
static int _match(const void *item, const void *param)
{
	const struct slave_device *s1 = item;
	const struct slave_device *s2 = param;

	return (s1->block == s2->block);
}

/**
 * @brief Checks for duplicate entries on list of slave devices.
 *
 * This is internal function of sysfs module. The functions checks if the given
 * slave device is already on list with slave devices. This function is used by
 * _slave_cnt_add() function to avoid duplicate entries.
 *
 * @param[in]      slave          Pointer to slave device structure to check.
 *
 * @return 1 the given device is on the list, otherwise the function returns 0.
 */
static int _is_duplicate(struct slave_device *slave)
{
	return (list_first_that(&slave_list, _match, slave) != NULL);
}

/**
 * @brief Checks if given disk can be removed from sysfs_block_list if
 * metatada is not present.
 *
 * This is internal function (action) of sysfs module. The slave_list keeps
 * all devices with metadata (raid devices). If disk is not included in slave
 * list there is not metadata on it.
 *
 * @return 1 if can be removed, otherwise 0.
 */
static int _is_non_raid_device(struct block_device *block_device)
{
	struct node *node = list_head(&slave_list);

	while (node) {
		struct slave_device *slave_device = node->item;

		if (strcmp(slave_device->block->sysfs_path,
			block_device->sysfs_path) == 0) {
			return 0;
		}
		node = list_next(node);
	}
	return 1;
}

/**
 */
static void _slave_cnt_add(const char *path, struct raid_device *raid)
{
	struct slave_device *device;

	char *t = rindex(path, '/');
	if (strncmp(t + 1, "dev-", 4) == 0) {
		device = slave_device_init(path, &sysfs_block_list);
		if (device) {
			if (!_is_duplicate(device)) {
				device->raid = raid;
				list_append(&slave_list, device);
			} else {
				slave_device_fini(device);
			}
		}
	}
}

/**
 */
static void _link_volum(struct raid_device *device)
{
	char temp[PATH_MAX];

	str_cpy(temp, device->sysfs_path, PATH_MAX);
	str_cat(temp, "/md", PATH_MAX);

	struct list *dir = scan_dir(temp);
	if (dir) {
		list_for_each_parm(dir, _slave_vol_add, device);
		list_fini(dir);
	}
}

/**
 */
static void _link_cntnr(struct raid_device *device)
{
	char temp[PATH_MAX];

	str_cpy(temp, device->sysfs_path, PATH_MAX);
	str_cat(temp, "/md", PATH_MAX);

	struct list *dir = scan_dir(temp);
	if (dir) {
		list_for_each_parm(dir, _slave_cnt_add, device);
		list_fini(dir);
	}
}

/**
 */
static void _block_add(const char *path)
{
	struct block_device *device = block_device_init(&cntrl_list, path);
	if (device)
		list_append(&sysfs_block_list, device);
}

/**
 */
static void _volum_add(const char *path, unsigned int device_num)
{
	struct raid_device *device =
	    raid_device_init(path, device_num, DEVICE_TYPE_VOLUME);
	if (device)
		list_append(&volum_list, device);
}

/**
 */
static void _cntnr_add(const char *path, unsigned int device_num)
{
	struct raid_device *device =
	    raid_device_init(path, device_num, DEVICE_TYPE_CONTAINER);
	if (device)
		list_append(&cntnr_list, device);
}

/**
 */
static void _raid_add(const char *path)
{
	struct device_id device_id;

	_get_id(path, &device_id);
	if (device_id.major == 9) {
		switch (_get_device_type(path)) {
		case DEVICE_TYPE_VOLUME:
			_volum_add(path, device_id.minor);
			break;
		case DEVICE_TYPE_CONTAINER:
			_cntnr_add(path, device_id.minor);
			break;
		case DEVICE_TYPE_UNKNOWN:
			break;
		}
	}
}

/**
 */
static void _cntrl_add(const char *path)
{
	struct cntrl_device *device = cntrl_device_init(path);
	if (device)
		list_append(&cntrl_list, device);
}

/**
 */
static void _enclo_add(const char *path)
{
	struct enclosure_device *device = enclosure_device_init(path);
	if (device)
		list_append(&enclo_list, device);
}

/**
 */
static void _slots_add(const char *path)
{
	struct pci_slot *device = pci_slot_init(path);
	if (device)
		list_append(&slots_list, device);
}

/**
 */
static void _check_raid(const char *path)
{
	char *t = strrchr(path, '/');
	if (strncmp(t + 1, "md", 2) == 0)
		_raid_add(path);
}

/**
 */
static void _check_cntrl(const char *path)
{
	char link[PATH_MAX];
	if (realpath(path, link) != NULL)
		_cntrl_add(link);
}

/**
 */
static void _check_enclo(const char *path)
{
	char link[PATH_MAX];
	if (realpath(path, link) != NULL)
		_enclo_add(link);
}

/**
 */
static void _check_slots(const char *path)
{
	_slots_add(path);
}

static void _scan_block(void)
{
	struct list *dir = scan_dir(SYSFS_CLASS_BLOCK);
	if (dir) {
		list_for_each(dir, _block_add);
		list_fini(dir);
	}
}

static void _scan_raid(void)
{
	struct list *dir = scan_dir(SYSFS_CLASS_BLOCK);
	if (dir) {
		list_for_each(dir, _check_raid);
		list_fini(dir);
	}
}

static void _scan_cntrl(void)
{
	struct list *dir = scan_dir(SYSFS_PCI_DEVICES);
	if (dir) {
		list_for_each(dir, _check_cntrl);
		list_fini(dir);
	}
}

static void _scan_slave(void)
{
	list_for_each(&volum_list, _link_volum);
	list_for_each(&cntnr_list, _link_cntnr);
	if (conf.raid_memebers_only) {
		struct node *next_node, *node = list_head(&sysfs_block_list);

		while (node != NULL) {
			next_node = list_next(node);
			if (_is_non_raid_device(node->item)) {
				list_remove(node);
				block_device_fini(node->item);
				free(node);
			}
			node = next_node;
		}
	}
}

static void _scan_enclo(void)
{
	struct list *dir = scan_dir(SYSFS_CLASS_ENCLOSURE);
	if (dir) {
		list_for_each(dir, _check_enclo);
		list_fini(dir);
	}
}

static void _scan_slots(void)
{
	struct list *dir = scan_dir(SYSFS_PCI_SLOTS);
	if (dir) {
		list_for_each(dir, _check_slots);
		list_fini(dir);
	}
}

/**
 */
static int _is_failed_array(struct raid_device *raid)
{
	if (raid->degraded > 0) {
		switch (raid->level) {
		case RAID_LEVEL_1:
		case RAID_LEVEL_10:
			return (raid->degraded == raid->raid_disks);
		case RAID_LEVEL_4:
		case RAID_LEVEL_5:
			return (raid->degraded > 1);
		case RAID_LEVEL_6:
			return (raid->degraded > 2);
		case RAID_LEVEL_LINEAR:
		case RAID_LEVEL_UNKNOWN:
		case RAID_LEVEL_0:
			break;
		case RAID_LEVEL_FAULTY:
			return 1;
		}
	}
	return -1;
}

/**
 */
static void _set_block_state(struct block_device *block, enum ibpi_pattern ibpi)
{
	char *debug_dev = strrchr(block->sysfs_path, '/');
	debug_dev = debug_dev ? debug_dev + 1 : block->sysfs_path;
	log_debug("(%s): device: %s, state: %s", __func__, debug_dev,
		  ibpi_str[ibpi]);
	if (block->ibpi < ibpi)
		block->ibpi = ibpi;
}

/**
 */
static void _set_array_state(struct raid_device *raid,
			     struct block_device *block)
{
	switch (raid->sync_action) {
	case RAID_ACTION_UNKNOWN:
	case RAID_ACTION_IDLE:
	case RAID_ACTION_FROZEN:
		_set_block_state(block, IBPI_PATTERN_NORMAL);
		break;
	case RAID_ACTION_RESHAPE:
		if (conf.blink_on_migration)
			_set_block_state(block, IBPI_PATTERN_REBUILD);
		break;
	case RAID_ACTION_CHECK:
	case RAID_ACTION_RESYNC:
	case RAID_ACTION_REPAIR:
		if (conf.blink_on_init)
			_set_block_state(block, IBPI_PATTERN_REBUILD);
		break;
	case RAID_ACTION_RECOVER:
		if (conf.rebuild_blink_on_all)
			_set_block_state(block, IBPI_PATTERN_REBUILD);
		break;
	}
}

/**
 */
static void _determine(struct slave_device *device)
{
	if (device->block->raid_path != NULL)
		free(device->block->raid_path);
	device->block->raid_path = strdup(device->raid->sysfs_path);

	if ((device->
	     state & (SLAVE_STATE_BLOCKED | SLAVE_STATE_WRITE_MOSTLY)) != 0) {
		_set_block_state(device->block, IBPI_PATTERN_NORMAL);
	} else if ((device->state & SLAVE_STATE_FAULTY) != 0) {
		_set_block_state(device->block, IBPI_PATTERN_FAILED_DRIVE);
	} else if ((device->state & SLAVE_STATE_SPARE) != 0) {
		if (_is_failed_array(device->raid) == 0) {
			if (device->raid->sync_action != RAID_ACTION_RESHAPE ||
			    conf.blink_on_migration == 1)
				_set_block_state(device->block,
						 IBPI_PATTERN_REBUILD);
		} else {
			_set_block_state(device->block, IBPI_PATTERN_HOTSPARE);
		}
	} else if ((device->state & SLAVE_STATE_IN_SYNC) != 0) {
		switch (_is_failed_array(device->raid)) {
		case 0:
			_set_block_state(device->block, IBPI_PATTERN_DEGRADED);
			break;
		case 1:
			_set_block_state(device->block,
					 IBPI_PATTERN_FAILED_ARRAY);
			break;
		}
		_set_array_state(device->raid, device->block);
	}
}

void sysfs_init(void)
{
	list_init(&sysfs_block_list);
	list_init(&volum_list);
	list_init(&cntrl_list);
	list_init(&slave_list);
	list_init(&cntnr_list);
	list_init(&enclo_list);
	list_init(&slots_list);
}

void sysfs_reset(void)
{
	list_for_each(&sysfs_block_list, block_device_fini);
	list_clear(&sysfs_block_list);

	list_for_each(&volum_list, raid_device_fini);
	list_clear(&volum_list);

	list_for_each(&cntrl_list, cntrl_device_fini);
	list_clear(&cntrl_list);

	list_for_each(&slave_list, slave_device_fini);
	list_clear(&slave_list);

	list_for_each(&cntnr_list, raid_device_fini);
	list_clear(&cntnr_list);

	list_for_each(&enclo_list, enclosure_device_fini);
	list_clear(&enclo_list);

	list_for_each(&slots_list, pci_slot_fini);
	list_clear(&slots_list);
}

void sysfs_scan(void)
{
	_scan_enclo();
	_scan_cntrl();
	_scan_slots();
	_scan_block();
	_scan_raid();
	_scan_slave();

	list_for_each(&slave_list, _determine);
}

/*
 * The function reutrns list of enclosure devices attached to SAS/SCSI storage
 * controller(s).
 */
struct list *sysfs_get_enclosure_devices(void)
{
	return &enclo_list;
}

/*
 * The function returns list of controller devices present in the system.
 */
struct list *sysfs_get_cntrl_devices(void)
{
	return &cntrl_list;
}

/*
 * The function returns list of RAID volumes present in the system.
 */
struct list *sysfs_get_volumes(void)
{
	return &volum_list;
}

/*
 * The function executes action() function for each block device on the list.
 * See sysfs.h for details.
 */
void __sysfs_block_device_for_each(action_t action, void *parm)
{
	__list_for_each(&sysfs_block_list, action, parm);
}

/*
 * The function is looking for block device according to criteria defined by
 * 'test' function. See sysfs.h for details.
 */
struct block_device *sysfs_block_device_first_that(test_t test, void *parm)
{
	return list_first_that(&sysfs_block_list, test, parm);
}

/*
 * The function is looking for PCI hotplug slot according to criteria defined
 * by 'test' function. See sysfs.h for details.
 */
struct pci_slot *sysfs_pci_slot_first_that(test_t test, void *parm)
{
	return list_first_that(&slots_list, test, parm);
}

/**
 */
static int _enclo_match(const void *item, const void *param)
{
	const struct enclosure_device *device = item;
	const char *path = param;

	return (device->sysfs_path != NULL) &&
	    (strncmp(device->sysfs_path, path, strlen(path)) == 0);
}

/*
 * The function checks if the given storage controller has enclosure device(s)
 * attached.
 */
int sysfs_enclosure_attached_to_cntrl(const char *path)
{
	return (list_first_that(&enclo_list, _enclo_match, path) != NULL);
}

/*
 * This function checks driver type.
 */
int sysfs_check_driver(const char *path, const char *driver)
{
	char buf[PATH_MAX];
	char driver_path[PATH_MAX];
	char *link;
	int found = 0;
	str_cpy(buf, path, PATH_MAX);
	str_cat(buf, "/driver", PATH_MAX);
	snprintf(driver_path, PATH_MAX, "/%s", driver);

	link = realpath(buf, NULL);
	if (link && strstr(link, driver_path))
		found = 1;
	free(link);
	return found;
}
