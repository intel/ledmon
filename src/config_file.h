/*
 * Intel(R) Enclosure LED Utilities
 *
 * Copyright (C) 2017-2019 Intel Corporation.
 * Copyright (C) 2009 Karel Zak <kzak@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Contains code from util-linux/libblkid/src/config.c
 * originally released under LGPL.
*/

#ifndef SRC_CONFIG_FILE_H_
#define SRC_CONFIG_FILE_H_

#include "list.h"

#define LEDMON_SHARE_MEM_FILE "/ledmon.conf"
#define LEDMON_DEF_CONF_FILE "/etc/ledmon.conf"
#define LEDMON_DEF_LOG_FILE "/var/log/ledmon.log"
#define LEDCTL_DEF_LOG_FILE "/var/log/ledctl.log"
#define LEDMON_DEF_SLEEP_INTERVAL 10
#define LEDMON_MIN_SLEEP_INTERVAL 5

enum log_level_enum {
	LOG_LEVEL_UNDEF = 0,
	LOG_LEVEL_QUIET,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_INFO,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_ALL,
};

struct ledmon_conf {
	/* internal ledmon functions */
	char *log_path;
	enum log_level_enum log_level;
	int scan_interval;

	/* customizable leds behaviour */
	int blink_on_migration;
	int blink_on_init;
	int rebuild_blink_on_all;
	int raid_members_only;
	int ignore_raid_status;

	/* whitelist and blacklist of controllers for blinking */
	struct list cntrls_whitelist;
	struct list cntrls_blacklist;
};

extern struct ledmon_conf conf;

int ledmon_read_config(const char *filename);
int ledmon_write_shared_conf(void);
int ledmon_remove_shared_conf(void);

#endif /* SRC_CONFIG_FILE_H_ */
