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
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "cntrl.h"
#include "config.h"
#include "config_file.h"
#include "list.h"
#include "smp.h"
#include "status.h"
#include "sysfs.h"
#include "utils.h"

/**
 * @brief Name of controllers types.
 *
 * This is internal array with names of controller types. Array can be use to
 * translate enumeration type into the string.
 */
static const char * const ctrl_type_str[] = {
	[CNTRL_TYPE_UNKNOWN] = "?",
	[CNTRL_TYPE_DELLSSD] = "Dell SSD",
	[CNTRL_TYPE_VMD]     = "VMD",
	[CNTRL_TYPE_SCSI]    = "SCSI",
	[CNTRL_TYPE_AHCI]    = "AHCI"
};

/**
 */
static int _is_storage_controller(const char *path)
{
	uint64_t class = get_uint64(path, 0, "class");
	return (class & 0xff0000L) == 0x010000L;
}

/**
 */
static int _is_isci_cntrl(const char *path)
{
	return sysfs_check_driver(path, "isci");
}

/**
 */
static int _is_ahci_cntrl(const char *path)
{
	char temp[PATH_MAX], link[PATH_MAX], *t;

	str_cpy(temp, path, PATH_MAX);
	str_cat(temp, "/driver", PATH_MAX);

	if (realpath(temp, link) == NULL)
		return 0;

	t = strrchr(link, '/');
	if ((t != NULL) && (strcmp(t + 1, "ahci") != 0))
		return 0;

	return get_uint64(path, 0, "vendor") == 0x8086L;
}

static int _is_dellssd_cntrl(const char *path)
{
	uint64_t vdr, dev, svdr, cls;

	vdr = get_uint64(path, 0, "vendor");
	dev = get_uint64(path, 0, "device");
	cls = get_uint64(path, 0, "class");
	svdr = get_uint64(path, 0, "subsystem_vendor");

	return ((vdr == 0x1344L && dev == 0x5150L) || /* micron ssd */
	        (svdr == 0x1028 && cls == 0x10802));  /* nvmhci ssd */
}

/**
 */
static int _is_smp_cntrl(const char *path)
{
	int result = 0;
	void *dir = scan_dir(path);
	char *p;
	char host_path[PATH_MAX] = { 0 };
	char *host;
	if (dir) {
		host = list_head(dir);
		while (host) {
			p = strrchr(host, '/');
			if (!p++)
				break;
			if (strncmp(p, "host", strlen("host")) == 0) {
				snprintf(host_path, sizeof(host_path),
					"%s/%s/bsg/sas_%s", path, p, p);
				result = smp_write_gpio(host_path,
					GPIO_REG_TYPE_TX,
					0,
					0,
					"",
					0) == 0;
			}
			host = list_next(host);
		}
		list_fini(dir);
	}

	return result;
}

static int _is_vmd_cntrl(const char *path)
{
	return sysfs_check_driver(path, "vmd");
}

/**
 * @brief Determines the type of controller.
 *
 * This is internal function of 'controller device' module. The function
 * determines the type of controller device. It might be AHCI, SCSI or
 * UNKNOWN device type.
 *
 * @param[in]      path           path to controller device in sysfs tree.
 *
 * @return The type of controller device. If the type returned is
 *         CNTRL_TYPE_UNKNOWN this means a controller device is not
 *         supported.
 */
static enum cntrl_type _get_type(const char *path)
{
	enum cntrl_type type = CNTRL_TYPE_UNKNOWN;

	if (_is_vmd_cntrl(path)) {
		type = CNTRL_TYPE_VMD;
	} else if (_is_dellssd_cntrl(path)) {
		type = CNTRL_TYPE_DELLSSD;
	} else if (_is_storage_controller(path)) {
		if (_is_ahci_cntrl(path))
			type = CNTRL_TYPE_AHCI;
		else if (_is_isci_cntrl(path)
				|| sysfs_enclosure_attached_to_cntrl(path)
				|| _is_smp_cntrl(path))
			type = CNTRL_TYPE_SCSI;
	}
	return type;
}

struct _host_type *alloc_host(int id, struct _host_type *next)
{
	struct _host_type *host = NULL;
	host = malloc(sizeof(struct _host_type));
	if (host) {
		host->host_id = id;
		host->ibpi_state_buffer = NULL;
		memset(host->bitstream, 0, sizeof(host->bitstream));
		host->flush = 0;
		host->ports = 0;
		host->next = next;
	}
	return host;
}

void free_hosts(struct _host_type *h)
{
	struct _host_type *t;
	while (h) {
		t = h->next;
		free(h->ibpi_state_buffer);
		free(h);
		h = t;
	}
}

void _find_host(const char *path, struct _host_type **hosts)
{
	const int host_len = sizeof("host") - 1;
	char *p;
	int index = -1;
	struct _host_type *th;
	DIR *d;
	struct dirent *de;

	p = strrchr(path, '/');
	if (!p++)
		return;
	if (strncmp(p, "host", host_len - 1) == 0) {
		index = atoi(p + host_len);
		th = alloc_host(index, (*hosts) ? (*hosts) : NULL);
		if (!th)
			return;

		d = opendir(path);
		if(!d) {
			free(th);
			return;
		}

		while ((de = readdir(d))) {
			if (strncmp(de->d_name, "phy-", strlen("phy-")) == 0) {
				th->ports++;
			}
		}

		closedir(d);

		*hosts = th;
	}
}

/**
 * @brief Get all instances of separate hosts on isci controller.
 *
 */
static struct _host_type *_cntrl_get_hosts(const char *path)
{
	struct _host_type *hosts = NULL;
	void *dir = scan_dir(path);
	if (dir) {
		list_for_each_parm(dir, _find_host, &hosts);
		list_fini(dir);
	}
	return hosts;
}

/**
 * @brief Check if enclosure management is enabled.
 *
 * This is internal function of 'controller device' module. The function checks
 * whether enclosure management is enabled for AHCI controller.
 *
 * @param[in]      path           path to controller device in sysfs tree.
 *
 * @return 1 if enclosure management is enabled, otherwise the function returns 0.
 */
static unsigned int _ahci_em_messages(const char *path)
{
	/* first, open ...driver/module/parameters/ahci_em_messages
	 * then open /sys/module/libahci/parameters/ahci_em_messages
	 * and check if it is set to enabled.
	 * if so, check if 'holders' of libahci points the same driver name
	 * as device given by path
	 */
	char buf[PATH_MAX];
	char *link, *name;
	DIR *dh;
	struct dirent *de = NULL;

	/* old kernel (prior to 2.6.36) */
	if (get_int(path, 0, "driver/module/parameters/ahci_em_messages") != 0)
		return 1;

	/* parameter type changed from int to bool since kernel v3.13 */
	if (!get_int("", 0, "sys/module/libahci/parameters/ahci_em_messages")) {
		if (!get_bool("", 0, "sys/module/libahci/parameters/ahci_em_messages"))
			return 0;
	}

	if (snprintf(buf, sizeof(buf), "%s/%s", path, "driver") < 0)
		return 0;

	link = realpath(buf, NULL);
	if (!link)
		return 0;

	name = strrchr(link, '/');
	if (!name++) {
		free(link);
		return 0;
	}

	/* check if the directory /sys/module/libahci/holders exists */
	dh = opendir("/sys/module/libahci/holders");
	if (dh) {
		/* name contain controller name (ie. ahci),*/
		/* so check if libahci holds this driver   */
		while ((de = readdir(dh))) {
			if (!strcmp(de->d_name, name))
				break;
		}
		closedir(dh);
		free(link);
		return de ? 1 : 0;
	} else {
		free(link);
		return 1;
	}
}

static int match_cntrl(const void *item, const void *param)
{
	const char *pattern = item;
	const char *name = param;

	return match_string(name, pattern);
}

/*
 * Allocates memory for a new controller device structure. See cntrl.h for
 * details.
 */
struct cntrl_device *cntrl_device_init(const char *path)
{
	unsigned int em_enabled;
	enum cntrl_type type;
	struct cntrl_device *device = NULL;

	type = _get_type(path);
	if (type != CNTRL_TYPE_UNKNOWN) {
		switch (type) {
		case CNTRL_TYPE_DELLSSD:
		case CNTRL_TYPE_SCSI:
		case CNTRL_TYPE_VMD:
			em_enabled = 1;
			break;
		case CNTRL_TYPE_AHCI:
			em_enabled = _ahci_em_messages(path);
			break;
		default:
			em_enabled = 0;
		}
		if (em_enabled) {
			if (conf.cntrls_whitelist) {
				char *cntrl = list_first_that(conf.cntrls_whitelist, match_cntrl, path);
				if (!cntrl) {
					log_debug("%s not found on whitelist, ignoring", path);
					return NULL;
				}
			} else if (conf.cntrls_blacklist) {
				char *cntrl = list_first_that(conf.cntrls_blacklist, match_cntrl, path);
				if (cntrl) {
					log_debug("%s found on blacklist, ignoring", path);
					return NULL;
				}
			}
			device = malloc(sizeof(struct cntrl_device));
			if (device) {
				if (type == CNTRL_TYPE_SCSI) {
					device->isci_present = _is_isci_cntrl(path);
					device->hosts = _cntrl_get_hosts(path);
				} else {
					device->isci_present = 0;
					device->hosts = NULL;
				}
				device->cntrl_type = type;
				device->sysfs_path = strdup(path);
			}
		} else {
			log_error
			    ("controller discovery: %s - enclosure " \
			     "management not supported.", path);
		}
	}
	return device;
}

/*
 * Frees memory allocated for controller device structure. See cntrl.h for
 * details.
 */
void cntrl_device_fini(struct cntrl_device *device)
{
	if (device) {
		free(device->sysfs_path);
		free_hosts(device->hosts);
	}
}

void print_cntrl(struct cntrl_device *ctrl_dev)
{
		printf("%s (%s)\n", ctrl_dev->sysfs_path,
			ctrl_type_str[ctrl_dev->cntrl_type]);
}
