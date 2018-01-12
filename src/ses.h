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

#ifndef _SES_H_INCLUDED_
#define _SES_H_INCLUDED_

#include <asm/types.h>

/* Size of buffer for SES-2 Messages. */
#define SES_ALLOC_BUFF 4096

#define ENCL_CFG_DIAG_STATUS		0x01
#define ENCL_CTRL_DIAG_STATUS		0x02
#define ENCL_CTRL_DIAG_CFG		0x02
#define ENCL_EL_DESCR_STATUS		0x07
#define ENCL_ADDITIONAL_EL_STATUS	0x0a
#define SCSI_PROTOCOL_SAS		6

typedef enum __attribute__((packed)) {
	SES_UNSPECIFIED		= 0x00,
	SES_DEVICE_SLOT		= 0x01,
	SES_ARRAY_DEVICE_SLOT	= 0x17,
} element_type;

static inline void _set_prdfail(unsigned char *u)
{
	u[0] |= (1 << 6);
}

static inline void _set_abrt(unsigned char *u)
{
	u[1] |= (1 << 0);
}

static inline void _set_rebuild(unsigned char *u)
{
	u[1] |= (1 << 1);
}

static inline void _set_ifa(unsigned char *u)
{
	u[1] |= (1 << 2);
}

static inline void _set_ica(unsigned char *u)
{
	u[1] |= (1 << 3);
}

static inline void _set_cons_check(unsigned char *u)
{
	u[1] |= (1 << 4);
}

static inline void _set_hspare(unsigned char *u)
{
	u[1] |= (1 << 5);
}

static inline void _set_rsvd_dev(unsigned char *u)
{
	u[1] |= (1 << 6);
}

static inline void _set_ok(unsigned char *u)
{
	u[1] |= (1 << 7);
}

static inline void _set_ident(unsigned char *u)
{
	u[2] |= (1 << 1);
}

static inline void _clr_ident(unsigned char *u)
{
	u[2] &= ~(1 << 1);
}

static inline void _set_rm(unsigned char *u)
{
	u[2] |= (1 << 2);
}

static inline void _set_ins(unsigned char *u)
{
	u[2] |= (1 << 3);
}

static inline void _set_miss(unsigned char *u)
{
	u[2] |= (1 << 4);
}

static inline void _set_dnr(unsigned char *u)
{
	u[2] |= (1 << 6);
}

static inline void _set_actv(unsigned char *u)
{
	u[2] |= (1 << 7);
}

static inline void _set_enbb(unsigned char *u)
{
	u[3] |= (1 << 2);
}

static inline void _set_enba(unsigned char *u)
{
	u[3] |= (1 << 3);
}

static inline void _set_off(unsigned char *u)
{
	u[3] |= (1 << 4);
}

static inline void _set_fault(unsigned char *u)
{
	u[3] |= (1 << 5);
}

struct ses_page {
	unsigned char buf[SES_ALLOC_BUFF];
	int len;
};

struct type_descriptor_header {
	element_type element_type;
	__u8 num_of_elements;
	__u8 subenclosure_id;
	__u8 type_desc_text_len;
};

struct ses_pages {
	struct ses_page *page1;
	struct ses_page *page2;
	struct ses_page *page10;
	struct type_descriptor_header *page1_types;
	int page1_types_len;
};

struct ses_slot_ctrl_elem {
	union {
		struct {
			__u8 common_control;
			__u8 array_slot_control;
			__u8 b2;
			__u8 b3;
		};
		__u8 b[4];
	};
};

#endif
