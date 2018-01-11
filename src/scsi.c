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
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include <scsi/sg_lib.h>
#include <scsi/sg_cmds_extra.h>

#include "cntrl.h"
#include "config.h"
#include "enclosure.h"
#include "list.h"
#include "scsi.h"
#include "ses.h"
#include "status.h"
#include "sysfs.h"
#include "utils.h"

static int debug = 0;

static void print_page10(struct ses_pages *);

static void send_diag_slot(struct ses_pages *, int, const unsigned char *, int);

static int ses_set_message(enum ibpi_pattern, unsigned char *);

static int get_ses_page(int fd, struct ses_page *p, int pg_code)
{
	int ret;
	int retry_count = 3;

	do {
		ret = sg_ll_receive_diag(fd, 1, pg_code, p->buf, sizeof(p->buf),
					 0, debug);
	} while (ret && retry_count--);

	if (!ret)
		p->len = (p->buf[2] << 8) + p->buf[3] + 4;
	return ret;
}

static int process_page1(struct ses_pages *sp)
{
	int num_encl;		/* number of subenclosures */
	unsigned char *ed;	/* Enclosure Descriptor */
	int len = 0;
	int sum_headers = 0;	/* Number of Type descriptor headers */
	int i = 0;

	/* How many enclosures is in the main enclosure? */
	num_encl = sp->page1->buf[1] + 1;
	/* Go to Enclosure Descriptor */
	ed = sp->page1->buf + 8;
	for (i = 0; i < num_encl; i++, ed += len) {
		if (ed + 3 > sp->page1->buf + sp->page1->len) {
			log_debug
			    ("SES: Error, response pare 1 truncated at %d\n",
			     i);
			return 1;
		}
		sum_headers += ed[2];
		len = ed[3] + 4;
		if (len < 40) {
			log_debug("SES: Response too short for page 1\n");
			continue;
		}
	}

	sp->page1_types = (struct type_descriptor_header *)ed;
	sp->page1_types_len = sum_headers;

	/* ed is on type descr header */
	for (i = 0; i < sum_headers; i++, ed += 4) {
		if (ed > sp->page1->buf + sp->page1->len) {
			log_debug("SES: Response page 1 truncated at %d\n", i);
			return 1;
		}
	}
	return 0;
}

static struct ses_pages *ses_init(void)
{
	struct ses_pages *sp;
	sp = calloc(1, sizeof(*sp));
	if (!sp)
		return NULL;
	sp->page1 = calloc(1, sizeof(struct ses_page));
	if (!sp->page1)
		goto sp1;
	sp->page2 = calloc(1, sizeof(struct ses_page));
	if (!sp->page2)
		goto sp2;
	sp->page10 = calloc(1, sizeof(struct ses_page));
	if (!sp->page10)
		goto sp10;
	return sp;

 sp10:
	free(sp->page2);
 sp2:
	free(sp->page1);
 sp1:
	free(sp);
	return NULL;
}

static void ses_free(struct ses_pages *sp)
{
	if (!sp)
		return;
	free(sp->page1);
	free(sp->page2);
	free(sp->page10);
	free(sp);
}

static void dump_p10(unsigned char *p)
{
	int i;
	printf("----------------------------------------------\n");
	for (i = 0; i < 8; i++, p += 16) {
		printf("%p: %02x %02x %02x %02x %02x %02x %02x " \
		       "%02x %02x %02x %02x %02x %02x %02x %02x %02x\n", p,
		       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
		       p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
	}
}

/* Enclosure is already opened. */
static int is_addr_in_encl(int fd, uint64_t addr, int *idx)
{
	/* Get page10 and read address. If it fits, return 1. */
	struct ses_pages *sp = ses_init();
	unsigned char *add_desc = NULL;
	unsigned char *ap = NULL, *addr_p = NULL;
	int i, j, len = 0;
	uint64_t addr_cmp;

	if (!sp)
		return 0;
	/* Start processing */
	/* Read configuration. */
	if (get_ses_page(fd, sp->page1, ENCL_CFG_DIAG_STATUS))
		goto err;

	if (process_page1(sp))
		goto err;

	/* Get Enclosure Status - needed? */
	if (get_ses_page(fd, sp->page2, ENCL_CTRL_DIAG_STATUS))
		goto err;

	/* Additional Element Status */
	if (get_ses_page(fd, sp->page10, ENCL_ADDITIONAL_EL_STATUS))
		goto err;

	if (debug)
		print_page10(sp);

	/* Check Page10 for address. Extract index. */
	ap = add_desc = sp->page10->buf + 8;
	for (i = 0; i < sp->page1_types_len; i++) {
		struct type_descriptor_header *t = &sp->page1_types[i];

		if (t->element_type == SES_DEVICE_SLOT ||
		    t->element_type == SES_ARRAY_DEVICE_SLOT) {
			for (j = 0; j < t->num_of_elements; j++, ap += len) {
				if (debug)
					dump_p10(ap);
				/* Get Additional Element Status Descriptor */
				/* length (x-1) */
				len = ap[1] + 2;
				if ((ap[0] & 0xf) != SCSI_PROTOCOL_SAS)
					continue;	/* need SAS PROTO */
				/* It is a SAS protocol, go on */
				if ((ap[0] & 0x10))	/* Check EIP */
					addr_p = ap + 8;
				else
					addr_p = ap + 4;
				/* Process only PHY 0 descriptor. */

				/* Convert be64 to le64 */
				addr_cmp = ((uint64_t)addr_p[12] << 8*7) |
					   ((uint64_t)addr_p[13] << 8*6) |
					   ((uint64_t)addr_p[14] << 8*5) |
					   ((uint64_t)addr_p[15] << 8*4) |
					   ((uint64_t)addr_p[16] << 8*3) |
					   ((uint64_t)addr_p[17] << 8*2) |
					   ((uint64_t)addr_p[18] << 8*1) |
					   ((uint64_t)addr_p[19]);

				if (addr == addr_cmp) {
					if (idx)
						*idx = ap[0] & 0x10 ? ap[3] : j;
					ses_free(sp);
					return 1;
				}
			}
		}
	}
 err:
	ses_free(sp);
	return 0;
}

static int enclosure_open(const struct enclosure_device *enclosure)
{
	int fd = -1;

	if (enclosure->dev_path)
		fd = open(enclosure->dev_path, O_RDWR);

	return fd;
}

static void ses_send_to_idx(int fd, int idx, enum ibpi_pattern ibpi)
{
	unsigned char msg[4] = { 0 };
	struct ses_pages *sp;

	sp = ses_init();
	if (!sp)
		return;

	/* Start processing */
	/* Read configuration. */
	if (get_ses_page(fd, sp->page1, ENCL_CFG_DIAG_STATUS))
		goto err;

	if (process_page1(sp))
		goto err;

	/* Get Enclosure Status */
	if (get_ses_page(fd, sp->page2, ENCL_CTRL_DIAG_STATUS))
		goto err;

	if (ses_set_message(ibpi, msg))
		goto err;	/* unknown message */

	send_diag_slot(sp, fd, msg, idx);

 err:
	ses_free(sp);
	return;
}

static void print_page10(struct ses_pages *sp)
{
	unsigned char *ai = sp->page10->buf + 8;
	int i = 0, len = 0, eip = 0;
	unsigned char *sas = NULL;

	while (ai < sp->page10->buf + sp->page10->len) {
		printf("%s()[%d]: Inv: %d, EIP: %d, Proto: 0x%04x\n", __func__,
		       i++, ((ai[0] & 0x80) >> 7), ((ai[0] & 0x10) >> 4),
		       ai[0] & 0xf);
		printf("\tDescriptor len (x-1): %d\n", ai[1] + 1);
		eip = ai[0] && 0x10;
		if (eip)
			printf("\tElement Index: %d\n", ai[3]);
		len = ai[1] + 2;
		if ((ai[0] & 0xf) == SCSI_PROTOCOL_SAS) {
			if (eip)
				sas = ai + 4;
			else
				sas = ai + 2;
			printf("\tProtocol SAS:\n");
			printf("\tNumber of phy descriptors: %d\n", sas[0]);
			printf("\tNot all phys: %d, descriptor type: 0x%1x\n",
			       (sas[1] & 1), ((sas[1] & 0xc0) >> 6));
			if (eip) {
				printf("\tDevice slot number: %d\n", sas[3]);
				sas += 2;
			}
			sas += 2;
			printf("\tDevice type: 0x%01x\n",
			       ((sas[0] & 0x70) >> 4));
			printf("\tSMP Initiator Port: 0x%01x\n",
			       ((sas[2] & 2) >> 1));
			printf("\tSTP Initiator Port: 0x%01x\n",
			       ((sas[2] & 4) >> 2));
			printf("\tSSP Initiator Port: 0x%01x\n",
			       ((sas[2] & 8) >> 3));
			printf("\tSATA DEVICE: 0x%01x\n", (sas[3] & 1));
			printf("\tSMP Target Port: 0x%01x\n",
			       ((sas[3] & 2) >> 1));
			printf("\tSTP Target Port: 0x%01x\n",
			       ((sas[3] & 4) >> 2));
			printf("\tSSP Target Port: 0x%01x\n",
			       ((sas[3] & 8) >> 3));
			printf("\tSATA Port Selector: 0x%01x\n",
			       ((sas[3] & 0X80) >> 7));
			printf
			    ("\tAttached SAS Address: 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
			     sas[4], sas[5], sas[6], sas[7], sas[8], sas[9],
			     sas[10], sas[11]);
			printf
			    ("\tSAS Address: 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",
			     sas[12], sas[13], sas[14], sas[15], sas[16],
			     sas[17], sas[18], sas[19]);
			printf("\tPHY Identified: 0x%01x\n", sas[20]);
		} else
			printf("\tProtocol not SAS: 0x%02x, skipping\n",
			       (ai[0] & 0xf));
		/* */
		ai += len;
	}
	return;
}

static void send_diag_slot(struct ses_pages *sp, int fd,
			   const unsigned char *info, int idx)
{
	unsigned char *desc = sp->page2->buf + 8;	/* Move do descriptors */
	int i, j;
	int slot = 0;

	memset(desc, 0, sp->page2->len - 8);

	for (i = 0; i < sp->page1_types_len; i++) {
		struct type_descriptor_header *t = &sp->page1_types[i];

		if (t->element_type == SES_DEVICE_SLOT ||
		    t->element_type == SES_ARRAY_DEVICE_SLOT) {
			desc += 4;	/* At first, skip overall header. */
			for (j = 0; j < t->num_of_elements; j++, desc += 4) {
				if (slot++ == idx) {
					memcpy(desc, info, 4);
					/* set select */
					desc[0] |= 0x80;
					/* keep PRDFAIL */
					desc[0] |= 0x40;
					/* clear reserved flags */
					desc[0] &= 0xf0;
					break;
				}
			}
		}
	}
	if (sg_ll_send_diag(fd, 0, 1, 0, 0, 0, 0,
			    sp->page2->buf, sp->page2->len, 0, debug)) {
		return;
	}
	return;
}

static int ses_set_message(enum ibpi_pattern ibpi, unsigned char *u)
{
	switch (ibpi) {
	case IBPI_PATTERN_UNKNOWN:
	case IBPI_PATTERN_ONESHOT_NORMAL:
	case IBPI_PATTERN_NORMAL:
		_clr_msg(u);
		_set_ok(u);
		break;
	case IBPI_PATTERN_FAILED_ARRAY:
		_set_ifa(u);
		break;
	case IBPI_PATTERN_DEGRADED:
		_set_ica(u);
		break;
	case IBPI_PATTERN_REBUILD:
	case IBPI_PATTERN_REBUILD_P:
		_set_rebuild(u);
		break;
	case IBPI_PATTERN_FAILED_DRIVE:
		_set_fault(u);
		break;
	case IBPI_PATTERN_LOCATE_OFF:
		_clr_ident(u);
		break;
	case IBPI_PATTERN_LOCATE:
		_set_ident(u);
		break;
	case IBPI_PATTERN_HOTSPARE:
		_set_hspare(u);
		break;
	case IBPI_PATTERN_PFA:
		_set_rsvd_dev(u);
		break;
		/* SES MESSAGES */
	case SES_REQ_ABORT:
		_set_abrt(u);
		break;
	case SES_REQ_REBUILD:
		_set_rebuild(u);
		break;
	case SES_REQ_IFA:
		_set_ifa(u);
		break;
	case SES_REQ_ICA:
		_set_ica(u);
		break;
	case SES_REQ_CONS_CHECK:
		_set_cons_check(u);
		break;
	case SES_REQ_HOSTSPARE:
		_set_hspare(u);
		break;
	case SES_REQ_RSVD_DEV:
		_set_rsvd_dev(u);
		break;
	case SES_REQ_OK:
		_set_ok(u);
		break;
	case SES_REQ_IDENT:
		_set_ident(u);
		break;
	case SES_REQ_RM:
		_set_rm(u);
		break;
	case SES_REQ_INS:
		_set_ins(u);
		break;
	case SES_REQ_MISSING:
		_set_miss(u);
		break;
	case SES_REQ_DNR:
		_set_dnr(u);
		break;
	case SES_REQ_ACTIVE:
		_set_actv(u);
		break;
	case SES_REQ_EN_BB:
		_set_enbb(u);
		break;
	case SES_REQ_EN_BA:
		_set_enba(u);
		break;
	case SES_REQ_DEV_OFF:
		_set_off(u);
		break;
	case SES_REQ_FAULT:
		_set_fault(u);
		break;
	default:
		return 1;
	}
	return 0;
}

static char *get_drive_end_dev(const char *path)
{
	char *s, *c, *p;

	c = strstr(path, "end_device");
	if (!c)
		return NULL;
	s = strchr(c, '/');
	if (!s)
		return NULL;
	p = calloc(s - c + 1, sizeof(*p));
	if (!p)
		return NULL;

	strncpy(p, c, s - c);
	return p;
}

static uint64_t get_drive_sas_addr(const char *path)
{
	uint64_t ret = 0;
	size_t size = strlen(path) * 2;
	char *buff, *end_dev;

	/* Make big buffer. */
	buff = malloc(size);
	if (!buff)
		return ret;

	end_dev = get_drive_end_dev(path);
	if (!end_dev) {
		free(buff);
		return ret;
	}

	snprintf(buff, size, "/sys/class/sas_end_device/%s/device/sas_device/%s",
		 end_dev, end_dev);

	ret = get_uint64(buff, ret, "sas_address");

	free(end_dev);
	free(buff);

	return ret;
}

/**
 */
static int _slot_match(const char *slot_path, const char *device_path)
{
	char temp[PATH_MAX], link[PATH_MAX];

	str_cpy(temp, slot_path, PATH_MAX);
	str_cat(temp, "/device", PATH_MAX);

	if (realpath(temp, link) == NULL)
		return 0;

	return strncmp(link, device_path, strlen(link)) == 0;
}

/**
 */
static char *_slot_find(const char *enclo_path, const char *device_path)
{
	void *dir;
	char *temp, *result = NULL;

	dir = scan_dir(enclo_path);
	if (dir) {
		temp = list_first_that(dir, _slot_match, device_path);
		if (temp)
			result = strdup(temp);
		list_fini(dir);
	}
	return result;
}

int scsi_get_enclosure(struct block_device *device)
{
	uint64_t addr;
	int fd = -1;
	struct enclosure_device *encl;
	int ret;

	if (!device || !device->sysfs_path)
		return 0;

	addr = get_drive_sas_addr(device->sysfs_path);
	if (addr == 0)
		return 0;

	encl = sysfs_get_enclosure_devices();
	while (encl) {
		if (_slot_match(encl->sysfs_path, device->cntrl_path)) {
			device->enclosure = encl;
			break;
		}
		encl = list_next(encl);
	}

	if (!device->enclosure)
		return 0;

	fd = enclosure_open(device->enclosure);
	if (fd == -1)
		return 0;

	ret = is_addr_in_encl(fd, addr, &device->encl_index);

	close(fd);
	return ret;
}

/**
 */
int scsi_ses_write(struct block_device *device, enum ibpi_pattern ibpi)
{
	int fd = -1;

	if (!device || !device->sysfs_path || !device->enclosure ||
	    device->encl_index == -1)
		__set_errno_and_return(EINVAL);

	/* write only if state has changed */
	if (ibpi == device->ibpi_prev)
		return 1;

	if ((ibpi < IBPI_PATTERN_NORMAL) || (ibpi > SES_REQ_FAULT))
		__set_errno_and_return(ERANGE);

	fd = enclosure_open(device->enclosure);
	if (fd != -1) {
		ses_send_to_idx(fd, device->encl_index, ibpi);
		close(fd);
	} else {
		log_warning
		    ("Unable to send %s message to %s. Device is missing?",
		     ibpi_str[ibpi], strstr(device->sysfs_path, "host"));
	}
	return 0;
}

/**
 * @brief Gets a path to slot of sas controller.
 *
 * This function returns a sysfs path to component of enclosure the device
 * belongs to.
 *
 * @param[in]      path           Canonical sysfs path to block device.
 *
 * @return A sysfs path to controller device associated with the given
 *         block device if successful, otherwise NULL pointer.
 */
static char *sas_get_slot_path(const char *path, const char *ctrl_path)
{
	char *host;
	char host_path[PATH_MAX] = { 0 };
	size_t ctrl_path_len = strlen(ctrl_path);

	if (strncmp(path, ctrl_path, ctrl_path_len) != 0)
		return NULL;
	host = get_path_hostN(path);
	if (host) {
		snprintf(host_path, sizeof(host_path), "%s/%s/bsg/sas_%s",
			 ctrl_path, host, host);
		free(host);
	}
	return str_dup(host_path);
}

/**
 */
static char *_get_enc_slot_path(const char *path)
{
	struct enclosure_device *device;
	char *result = NULL;

	device = sysfs_get_enclosure_devices();
	while (device) {
		result = _slot_find(device->sysfs_path, path);
		if (result != NULL)
			break;
		device = list_next(device);
	}
	return result;
}

/**
 */
char *scsi_get_slot_path(const char *path, const char *ctrl_path)
{
	char *result = NULL;

	result = _get_enc_slot_path(path);
	if (!result)
		result = sas_get_slot_path(path, ctrl_path);
	return result;
}
