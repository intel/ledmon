/*
 * Generic IPMI Interface
 * Copyright (C) 2023, Dell Inc.
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

#include "libled_private.h"

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "utils.h"
#include "ipmi.h"

static int ipmi_open(void)
{
	int fd;

	fd = open("/dev/ipmi0", O_RDWR);
	if (fd >= 0)
		return fd;
	fd = open("/dev/ipmidev/0", O_RDWR);
	if (fd >= 0)
		return fd;
	fd = open("/dev/ipmidev0", O_RDWR);
	if (fd >= 0)
		return fd;
	fd = open("/dev/bmc", O_RDWR);
	if (fd >= 0)
		return fd;
	return -1;
}

int ipmicmd(struct led_ctx *ctx, int sa, int lun, int netfn, int cmd, int datalen, void *data,
	    int resplen, int *rlen, void *resp)
{
	struct ipmi_system_interface_addr saddr;
	struct ipmi_ipmb_addr iaddr;
	struct ipmi_addr raddr;
	struct ipmi_req req;
	struct ipmi_recv rcv;
	fd_set rfd;
	int fd, rc;
	uint8_t tresp[resplen + 1];

	fd = ipmi_open();
	if (fd < 0)
		return -1;

	memset(&saddr, 0, sizeof(saddr));
	memset(&iaddr, 0, sizeof(iaddr));
	memset(&raddr, 0, sizeof(raddr));
	memset(&req, 0, sizeof(req));
	memset(&rcv, 0, sizeof(rcv));
	memset(tresp, 0, sizeof(uint8_t) + (resplen + 1));

	if (sa == BMC_SA) {
		saddr.addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE;
		saddr.channel = IPMI_BMC_CHANNEL;
		saddr.lun = 0;
		req.addr = (void *)&saddr;
		req.addr_len = sizeof(saddr);
	} else {
		iaddr.addr_type = IPMI_IPMB_ADDR_TYPE;
		iaddr.channel = 0;
		iaddr.slave_addr = sa;
		iaddr.lun = lun;
		req.addr = (void *)&iaddr;
		req.addr_len = sizeof(iaddr);
	}

	/* Issue command */
	req.msgid = ++ctx->ipmi_msgid;
	req.msg.netfn = netfn;
	req.msg.cmd = cmd;
	req.msg.data_len = datalen;
	req.msg.data = data;
	rc = ioctl(fd, IPMICTL_SEND_COMMAND, (void *)&req);
	if (rc != 0) {
		perror("send");
		goto end;
	}

	/* Wait for Response */
	FD_ZERO(&rfd);
	FD_SET(fd, &rfd);
	rc = select(fd + 1, &rfd, NULL, NULL, NULL);
	if (rc < 0) {
		perror("select");
		goto end;
	}

	/* Get response */
	rcv.msg.data = tresp;
	rcv.msg.data_len = resplen + 1;
	rcv.addr = (void *)&raddr;
	rcv.addr_len = sizeof(raddr);
	rc = ioctl(fd, IPMICTL_RECEIVE_MSG_TRUNC, (void *)&rcv);
	if (rc != 0 && errno == EMSGSIZE)
		lib_log(ctx, LED_LOG_LEVEL_INFO, "too short..\n");
	if (rc != 0 && errno != EMSGSIZE) {
		lib_log(ctx, LED_LOG_LEVEL_INFO, "%s\n", strerror(errno));
		goto end;
	}
	if (rcv.msg.data[0])
		lib_log(ctx, LED_LOG_LEVEL_DEBUG, "IPMI Error: %.2x\n", rcv.msg.data[0]);
	rc = 0;
	*rlen = rcv.msg.data_len - 1;
	memcpy(resp, rcv.msg.data + 1, *rlen);
end:
	close(fd);
	return rc;
}
