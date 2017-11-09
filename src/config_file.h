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

#ifndef SRC_CONFIG_FILE_H_
#define SRC_CONFIG_FILE_H_

#define LEDMON_DEF_CONF_FILE "/etc/ledmon.conf"
#define LEDMON_DEF_LOG_FILE "/var/log/ledmon.log"
#define LEDMON_DEF_SLEEP_INTERVAL 10
#define LEDMON_MIN_SLEEP_INTERVAL 5

/**
 * @brief Pointer to global ledmon configuration.
 *
 * This is a pointer to structure that contains settings of ledmon behavior
 * read from configuration file.
 */
extern struct ledmon_conf *conf;

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
	int raid_memebers_only;

	/* whitelist and blacklist of controllers for blinking */
	void *cntrls_whitelist;
	void *cntrls_blacklist;
};

struct ledmon_conf *ledmon_read_config(const char *filename);

#endif /* SRC_CONFIG_FILE_H_ */
