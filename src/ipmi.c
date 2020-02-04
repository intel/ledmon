/*
 * Generic IPMI Interface
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

int ipmicmd(int sa, int lun, int netfn, int cmd, int datalen, void *data,
	    int resplen, int *rlen, void *resp)
{
	static int msgid;
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

	memset(&req, 0, sizeof(req));
	memset(&rcv, 0, sizeof(rcv));
	if (sa == BMC_SA) {
		memset(&saddr, 0, sizeof(saddr));
		saddr.addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE;
		saddr.channel = IPMI_BMC_CHANNEL;
		saddr.lun = 0;
		req.addr = (void *)&saddr;
		req.addr_len = sizeof(saddr);
	} else {
		memset(&iaddr, 0, sizeof(iaddr));
		iaddr.addr_type = IPMI_IPMB_ADDR_TYPE;
		iaddr.channel = 0;
		iaddr.slave_addr = sa;
		iaddr.lun = lun;
		req.addr = (void *)&iaddr;
		req.addr_len = sizeof(iaddr);
	}

	/* Issue command */
	req.msgid = ++msgid;
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
		log_info("too short..\n");
	if (rc != 0 && errno != EMSGSIZE) {
		log_info("%s\n", strerror(errno));
		goto end;
	}
	if (rcv.msg.data[0])
		log_info("IPMI Error: %.2x\n", rcv.msg.data[0]);
	rc = 0;
	*rlen = rcv.msg.data_len - 1;
	memcpy(resp, rcv.msg.data + 1, *rlen);
end:
	close(fd);
	return rc;
}
