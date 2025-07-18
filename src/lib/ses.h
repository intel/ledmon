// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 Intel Corporation.

#ifndef _SES_H_INCLUDED_
#define _SES_H_INCLUDED_

/* System headers */
#include <asm/types.h>
#include <stdint.h>

/* Public headers */
#include <led/libled.h>

/* Local headers */
#include "status.h"

/* Size of buffer for SES-2 Messages. */
#define SES_ALLOC_BUFF 4096

typedef enum __attribute__((packed)) {
	SES_UNSPECIFIED		= 0x00,
	SES_DEVICE_SLOT		= 0x01,
	SES_ARRAY_DEVICE_SLOT	= 0x17,
} element_type;

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
	struct ses_page page1;
	struct ses_page page2;
	struct ses_page page10;
	const struct type_descriptor_header *page1_types;
	int page1_types_len;
	int changes;
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

struct ses_slot {
	int index;
	uint64_t sas_addr;
	enum led_ibpi_pattern ibpi_status;
};

int ses_load_pages(int fd, struct ses_pages *sp, struct led_ctx *ctx);
status_t ses_write_msg(enum led_ibpi_pattern ibpi, struct ses_pages *sp, int idx);
int ses_send_diag(int fd, struct ses_pages *sp);
int ses_get_slots(struct ses_pages *sp, struct ses_slot **out_slots, int *out_slots_count);
uint64_t ses_get_primary_logical_id(struct ses_pages *sp);

#endif /* _SES_H_INCLUDED_ */
