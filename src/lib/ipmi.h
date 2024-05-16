// SPDX-License-Identifier: LGPL-2.1-or-later

/* Generic IPMI Interface */

#include "led/libled.h"

#define BMC_TA 0x20

int ipmicmd(struct led_ctx *ctx, int sa, int lun, int netfn, int cmd, int datalen, void *data,
	    int resplen, int *rlen, void *resp);
