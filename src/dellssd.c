/*
 * Dell Backplane LED control
 * Copyright (C) 2011, Dell Inc.
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

#include <sys/ioctl.h>
#include <linux/ipmi.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "ahci.h"
#include "cntrl.h"
#include "config/config.h"
#include "dellssd.h"
#include "ibpi.h"
#include "list.h"
#include "raid.h"
#include "scsi.h"
#include "slave.h"
#include "smp.h"
#include "status.h"
#include "sysfs.h"
#include "utils.h"
#include "ipmi.h"

#define BP_PRESENT       (1L << 0)
#define BP_ONLINE        (1L << 1)
#define BP_HOTSPARE      (1L << 2)
#define BP_IDENTIFY      (1L << 3)
#define BP_REBUILDING    (1L << 4)
#define BP_FAULT         (1L << 5)
#define BP_PREDICT       (1L << 6)
#define BP_CRITICALARRAY (1L << 9)
#define BP_FAILEDARRAY   (1L << 10)

static const unsigned int ibpi2ssd[] = {
	[IBPI_PATTERN_UNKNOWN]        = BP_ONLINE,
	[IBPI_PATTERN_ONESHOT_NORMAL] = BP_ONLINE,
	[IBPI_PATTERN_NORMAL]         = BP_ONLINE,
	[IBPI_PATTERN_DEGRADED]       = BP_CRITICALARRAY|BP_ONLINE,
	[IBPI_PATTERN_REBUILD]        = BP_REBUILDING|BP_ONLINE,
	[IBPI_PATTERN_FAILED_ARRAY]   = BP_FAILEDARRAY|BP_ONLINE,
	[IBPI_PATTERN_HOTSPARE]       = BP_HOTSPARE|BP_ONLINE,
	[IBPI_PATTERN_PFA]            = BP_PREDICT|BP_ONLINE,
	[IBPI_PATTERN_FAILED_DRIVE]   = BP_FAULT|BP_ONLINE,
	[IBPI_PATTERN_LOCATE]         = BP_IDENTIFY|BP_ONLINE,
	[IBPI_PATTERN_LOCATE_OFF]     = BP_ONLINE
};

#define DELL_OEM_NETFN                      0x30

#define DELL_OEM_STORAGE_CMD                0xD5
#define DELL_OEM_STORAGE_GETDRVMAP_12G      0x07
#define DELL_OEM_STORAGE_SETDRVSTATUS_12G   0x04
#define DELL_OEM_STORAGE_GETDRVMAP_13G      0x17
#define DELL_OEM_STORAGE_SETDRVSTATUS_13G   0x14
#define DELL_OEM_STORAGE_GETDRVMAP_14G      0x37
#define DELL_OEM_STORAGE_SETDRVSTATUS_14G   0x34

#define APP_NETFN			    0x06
#define APP_GET_SYSTEM_INFO		    0x59
#define DELL_GET_IDRAC_INFO		    0xDD

enum {
  DELL_12G_MONOLITHIC = 0x10,
  DELL_12G_MODULAR    = 0x11,
  DELL_13G_MONOLITHIC = 0x20,
  DELL_13G_MODULAR    = 0x21,
  DELL_14G_MONOLITHIC = 0x30,
  DELL_14G_MODULAR    = 0x31,
};

int get_dell_server_type()
{
	static int gen;
	uint8_t data[4], rdata[20];
	int rc, rlen;

	/* Don't requery if we already know have ID */
	if (gen)
		return gen;

	/* Get Dell Generation */
	memset(data, 0, sizeof(data));
	memset(rdata, 0, sizeof(rdata));
	data[0] = 0x00;
	data[1] = DELL_GET_IDRAC_INFO;
	data[2] = 0x02;
	data[3] = 0x00;
	rc = ipmicmd(BMC_SA, 0, APP_NETFN, APP_GET_SYSTEM_INFO, 4, data,
		     20, &rlen, rdata);
	if (rc) {
		log_debug("Unable to issue IPMI command GetSystemInfo\n");
		return 0;
	}
	switch (rdata[10]) {
	case DELL_12G_MONOLITHIC:
	case DELL_12G_MODULAR:
	case DELL_13G_MONOLITHIC:
	case DELL_13G_MODULAR:
	case DELL_14G_MONOLITHIC:
	case DELL_14G_MODULAR:
		gen = rdata[10];
		return gen;
	default:
		log_debug("Unable to determine Dell Server type\n");
		break;
	}
	return 0;
}

static int ipmi_setled(int b, int d, int f, int state)
{
	uint8_t data[20], rdata[20];
	int rc, rlen, bay = 0xFF, slot = 0xFF, devfn, gen = 0;

	/* Check if this is a supported Dell server */
	gen = get_dell_server_type();
	if (!gen)
		return 0;
	devfn = (((d & 0x1F) << 3) | (f & 0x7));

	/* Get mapping of BDF to bay:slot */
	memset(data, 0, sizeof(data));
	memset(rdata, 0, sizeof(rdata));
	data[0] = 0x01;				/* get         */
	data[2] = 0x06;				/* length lsb  */
	data[3] = 0x00;				/* length msb  */
	data[4] = 0x00;				/* offset lsb  */
	data[5] = 0x00;				/* offset msb  */
	data[6] = b;				/* bus         */
	data[7] = devfn;			/* devfn       */
	switch (gen) {
	case DELL_12G_MONOLITHIC:
	case DELL_12G_MODULAR:
		data[1] = DELL_OEM_STORAGE_GETDRVMAP_12G;
		break;
	case DELL_13G_MONOLITHIC:
	case DELL_13G_MODULAR:
		data[1] = DELL_OEM_STORAGE_GETDRVMAP_13G;
		break;
	case DELL_14G_MONOLITHIC:
	case DELL_14G_MODULAR:
		data[1] = DELL_OEM_STORAGE_GETDRVMAP_14G;
		break;
	}
	rc = ipmicmd(BMC_SA, 0, DELL_OEM_NETFN, DELL_OEM_STORAGE_CMD, 8, data,
		     20, &rlen, rdata);
	if (!rc) {
		bay = rdata[7];
		slot = rdata[8];
	}
	if (bay == 0xFF || slot == 0xFF) {
		log_error("Unable to determine bay/slot for device %.2x:%.2x.%x\n",
			  b, d, f);
		return 0;
	}

	/* Set Bay:Slot to Mask */
	memset(data, 0, sizeof(data));
	memset(rdata, 0, sizeof(rdata));
	data[0] = 0x00;					/* set              */
	data[2] = 0x0e;					/* length lsb       */
	data[3] = 0x00;					/* length msb       */
	data[4] = 0x00;					/* offset lsb       */
	data[5] = 0x00;					/* offset msb       */
	data[6] = 0x0e;					/* length lsb       */
	data[7] = 0x00;					/* length msb       */
	data[8] = bay;					/* bayid            */
	data[9] = slot;					/* slotid           */
	data[10] = state & 0xff;			/* state LSB        */
	data[11] = state >> 8;				/* state MSB        */
	switch (gen) {
	case DELL_12G_MONOLITHIC:
	case DELL_12G_MODULAR:
		data[1] = DELL_OEM_STORAGE_SETDRVSTATUS_12G;
		break;
	case DELL_13G_MONOLITHIC:
	case DELL_13G_MODULAR:
		data[1] = DELL_OEM_STORAGE_SETDRVSTATUS_13G;
		break;
	case DELL_14G_MONOLITHIC:
	case DELL_14G_MODULAR:
		data[1] = DELL_OEM_STORAGE_SETDRVSTATUS_14G;
		break;
	}
	rc = ipmicmd(BMC_SA, 0, DELL_OEM_NETFN, DELL_OEM_STORAGE_CMD, 20, data,
		     20, &rlen, rdata);
	if (rc) {
		log_error("Unable to issue SetDriveState for %.2x:%.2x.%x\n",
			  b,d,f);
	}
	return 0;
}

char *dellssd_get_path(const char *cntrl_path)
{
	return str_dup(cntrl_path);
}

int dellssd_write(struct block_device *device, enum ibpi_pattern ibpi)
{
	unsigned int mask, bus, dev, fun;
	char *t;

	/* write only if state has changed */
	if (ibpi == device->ibpi_prev)
		return 1;

	if ((ibpi < IBPI_PATTERN_NORMAL) || (ibpi > IBPI_PATTERN_LOCATE_OFF))
		__set_errno_and_return(ERANGE);
	mask = ibpi2ssd[ibpi];
	t = strrchr(device->cntrl_path, '/');
	if (t != NULL) {
		/* Extract PCI bus:device.function */
		if (sscanf(t + 1, "%*x:%x:%x.%x", &bus, &dev, &fun) == 3)
			ipmi_setled(bus, dev, fun, mask);
	}
	return 0;
}
