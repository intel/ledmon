/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009 Karel Zak <kzak@redhat.com>
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
 * Adapted from util-linux - blkid which was released with:
 * This file may be redistributed under the terms of the
 * GNU Lesser General Public License.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>

#include "config_file.h"
#include "list.h"
#include "utils.h"

static int parse_bool(char *s)
{
	if (*s && (!strcasecmp(s, "enabled") ||
		   !strcasecmp(s, "true") ||
		   !strcasecmp(s, "yes") ||
		   !strcasecmp(s, "1")))
		return 1;
	else if (*s && (!strcasecmp(s, "disabled") ||
		       !strcasecmp(s, "false") ||
		       !strcasecmp(s, "no") ||
		       !strcasecmp(s, "0")))
		return 0;

	fprintf(stderr, "Unknown bool value: %s\n", s);
	return -1;
}

void parse_list(void **list, char *s)
{
	if (*list)
		list_fini(*list);

	if (list_init(list) != STATUS_SUCCESS)
		return;

	while (s && *s) {
		char *sep;

		sep = strchr(s, ',');
		if (sep)
			*sep = '\0';

		list_put(*list, strdup(s), strlen(s));

		if (sep)
			s = sep + 1;
		else
			break;
	}
}

static int parse_next(FILE *fd, struct ledmon_conf *conf)
{
	char buf[BUFSIZ];
	char *s;

	/* read the next non-blank non-comment line */
	do {
		if (fgets(buf, sizeof(buf), fd) == NULL)
			return feof(fd) ? 0 : -1;
		s = strchr(buf, '\n');
		if (!s) {
			/* Missing final newline?  Otherwise extremely */
			/* long line - assume file was corrupted */
			if (feof(fd))
				s = strchr(buf, '\0');
			else {
				fprintf(stderr,
					"config file: missing newline at line '%s'.",
					buf);
				return -1;
			}
		}
		*s = '\0';
		if (--s >= buf && *s == '\r')
			*s = '\0';

		s = buf;
		while (*s == ' ' || *s == '\t')		/* skip space */
			s++;

	} while (*s == '\0' || *s == '#');

	if (!strncmp(s, "INTERVAL=", 9)) {
		s += 9;
		if (*s) {
			sscanf(s, "%d", &conf->scan_interval);
			if (conf->scan_interval < LEDMON_MIN_SLEEP_INTERVAL)
				conf->scan_interval = LEDMON_MIN_SLEEP_INTERVAL;
		}
	} else if (!strncmp(s, "LOG_LEVEL=", 10)) {
		s += 10;
		if (*s)
			sscanf(s, "%d", (int *)&conf->log_level);
	} else if (!strncmp(s, "LOG_PATH=", 9)) {
		s += 9;
		if (*s) {
			free(conf->log_path);
			conf->log_path = strdup(s);
		}
	} else if (!strncmp(s, "BLINK_ON_MIGR=", 14)) {
		s += 14;
		conf->blink_on_migration = parse_bool(s);
		if (conf->blink_on_migration < 0)
			return -1;
	} else if (!strncmp(s, "BLINK_ON_INIT=", 14)) {
		s += 14;
		conf->blink_on_init = parse_bool(s);
		if (conf->blink_on_init < 0)
			return -1;
	} else if (!strncmp(s, "REBUILD_BLINK_ON_ALL=", 21)) {
		s += 21;
		conf->rebuild_blink_on_all = parse_bool(s);
		if (conf->rebuild_blink_on_all < 0)
			return -1;
	} else if (!strncmp(s, "RAID_MEMBERS_ONLY=", 18)) {
		s += 18;
		conf->raid_memebers_only = parse_bool(s);
		if (conf->raid_memebers_only < 0)
			return -1;
	} else if (!strncmp(s, "WHITELIST=", 10)) {
		s += 10;
		if (*s) {
			parse_list(&conf->cntrls_whitelist, s);
			if (!conf->cntrls_whitelist)
				return -1;
		}
	} else if (!strncmp(s, "BLACKLIST=", 10)) {
		s += 10;
		if (*s) {
			parse_list(&conf->cntrls_blacklist, s);
			if (!conf->cntrls_blacklist)
				return -1;
		}
	} else {
		fprintf(stderr, "config file: unknown option '%s'.\n", s);
		return -1;
	}
	return 0;
}

void ledmon_free_config(struct ledmon_conf *conf)
{
	if (!conf)
		return;

	list_fini(conf->cntrls_blacklist);
	list_fini(conf->cntrls_whitelist);

	if (conf->log_path)
		free(conf->log_path);

	free(conf);
	conf = NULL;
}

/* return real config data or built-in default */
struct ledmon_conf *ledmon_read_config(const char *filename)
{
	struct ledmon_conf *conf;
	FILE *f;

	if (!filename)
		filename = LEDMON_DEF_CONF_FILE;

	conf = calloc(1, sizeof(*conf));
	if (!conf)
		return NULL;

	/* initialize with default values */
	conf->blink_on_init = 1;
	conf->blink_on_migration = 1;
	conf->rebuild_blink_on_all = 1;
	conf->raid_memebers_only = 0;
	conf->log_level = LOG_LEVEL_WARNING;
	conf->log_path = strdup(LEDMON_DEF_LOG_FILE);
	conf->scan_interval = LEDMON_DEF_SLEEP_INTERVAL;

	printf("reading config file: %s.\n", filename);

	f = fopen(filename, "re");
	if (!f) {
		fprintf(stderr, "%s: does not exist, using built-in default\n",
		    filename);
		return conf;
	}
	while (!feof(f)) {
		if (parse_next(f, conf)) {
			fprintf(stderr, "%s: parse error\n", filename);
			ledmon_free_config(conf);
			fclose(f);
			return NULL;
		}
	}

	fclose(f);
	return conf;
}

#ifdef _TEST_CONFIG
/*
 * usage: ledmon_conf_test [<filename>]
 */

void print(const char *s)
{
	printf("%s, ", s);
}

int main(int argc, char *argv[])
{
	struct ledmon_conf *conf;
	char *filename = NULL;

	if (argc == 2)
		filename = argv[1];

	conf = ledmon_read_config(filename);
	if (!conf)
		return EXIT_FAILURE;

	printf("INTERVAL: %d\n", conf->scan_interval);
	printf("LOG_LEVEL: %d\n", conf->log_level);
	printf("LOG_PATH: %s\n", conf->log_path);
	printf("BLINK_ON_MIGR: %d\n", conf->blink_on_migration);
	printf("BLINK_ON_INIT: %d\n", conf->blink_on_init);
	printf("REBUILD_BLINK_ON_ALL: %d\n", conf->rebuild_blink_on_all);
	printf("RAID_MEMBERS_ONLY: %d\n", conf->raid_memebers_only);

	if (!conf->cntrls_whitelist)
		printf("WHITELIST: NONE\n");
	else {
		printf("WHITELIST: ");
		list_for_each(conf->cntrls_whitelist, print);
		printf("\n");
	}

	if (!conf->cntrls_blacklist)
		printf("BLACKLIST: NONE\n");
	else {
		printf("BLACKLIST: ");
		list_for_each(conf->cntrls_blacklist, print);
		printf("\n");
	}

	ledmon_free_config(conf);
	return EXIT_SUCCESS;
}
#endif
