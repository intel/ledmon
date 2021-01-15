/*
 * Intel(R) Enclosure LED Utilities
 *
 * Copyright (C) 2017-2021 Intel Corporation.
 * Copyright (C) 2009 Karel Zak <kzak@redhat.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * Contains code from util-linux/libblkid/src/config.c
 * originally released under LGPL.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>

#include "config_file.h"
#include "utils.h"
#include "status.h"


/**
 * @brief Pointer to global ledmon configuration.
 *
 * This is a pointer to structure that contains settings of ledmon behavior
 * read from configuration file.
 */
struct ledmon_conf conf;

const char *log_level_map[] = {
	[LOG_LEVEL_QUIET]   = "QUIET",
	[LOG_LEVEL_ERROR]   = "ERROR",
	[LOG_LEVEL_WARNING] = "WARNING",
	[LOG_LEVEL_INFO]    = "INFO",
	[LOG_LEVEL_DEBUG]   = "DEBUG",
	[LOG_LEVEL_ALL]     = "ALL"
};

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

static void parse_list(struct list *list, char *s)
{
	list_erase(list);

	while (s && *s) {
		char *sep;

		sep = strchr(s, ',');
		if (sep)
			*sep = '\0';

		list_append(list, str_dup(s));

		if (sep)
			s = sep + 1;
		else
			break;
	}
}

int _map_log_level(char *conf_log_level)
{
	size_t i = 1;

	while (i < sizeof(log_level_map)/sizeof(char *)) {
		if (strcasecmp(log_level_map[i], conf_log_level) == 0)
			return i;
		i++;
	}
	return 0;
}

void _set_log_level(char *s)
{
	int log_level;

	log_level = _map_log_level(s);
	if (log_level)
		conf.log_level = log_level;
	else if (sscanf(s, "%d", &log_level) == 1 &&
			log_level >= LOG_LEVEL_QUIET &&
			log_level <= LOG_LEVEL_ALL)
		conf.log_level = log_level;
	else
		log_warning("Log level given in config file (%s) is incorrect! Using default log level: %s",
			s, log_level_map[conf.log_level]);
}

static int parse_next(FILE *fd)
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
				fprintf(stderr, "config file: missing newline at line '%s'.",
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
			if (sscanf(s, "%d", &conf.scan_interval) != 1 ||
			    conf.scan_interval < LEDMON_MIN_SLEEP_INTERVAL)
				conf.scan_interval = LEDMON_MIN_SLEEP_INTERVAL;
		}
	} else if (!strncmp(s, "LOG_LEVEL=", 10)) {
		s += 10;
		_set_log_level(s);
	} else if (!strncmp(s, "LOG_PATH=", 9)) {
		s += 9;
		if (*s)
			set_log_path(s);
	} else if (!strncmp(s, "BLINK_ON_MIGR=", 14)) {
		s += 14;
		conf.blink_on_migration = parse_bool(s);
		if (conf.blink_on_migration < 0)
			return -1;
	} else if (!strncmp(s, "BLINK_ON_INIT=", 14)) {
		s += 14;
		conf.blink_on_init = parse_bool(s);
		if (conf.blink_on_init < 0)
			return -1;
	} else if (!strncmp(s, "REBUILD_BLINK_ON_ALL=", 21)) {
		s += 21;
		conf.rebuild_blink_on_all = parse_bool(s);
		if (conf.rebuild_blink_on_all < 0)
			return -1;
	} else if (!strncmp(s, "RAID_MEMBERS_ONLY=", 18)) {
		s += 18;
		conf.raid_members_only = parse_bool(s);
		if (conf.raid_members_only < 0)
			return -1;
	} else if (!strncmp(s, "WHITELIST=", 10)) {
		s += 10;
		if (*s)
			parse_list(&conf.cntrls_whitelist, s);
	} else if (!strncmp(s, "BLACKLIST=", 10)) {
		s += 10;
		if (*s)
			parse_list(&conf.cntrls_blacklist, s);
	} else {
		fprintf(stderr, "config file: unknown option '%s'.\n", s);
		return -1;
	}
	return 0;
}

void ledmon_free_config(void)
{
	list_erase(&conf.cntrls_blacklist);
	list_erase(&conf.cntrls_whitelist);

	if (conf.log_path)
		free(conf.log_path);
}

/* return real config data or built-in default */
int ledmon_read_config(const char *filename)
{
	FILE *f;

	if (!filename || (filename && access(filename, F_OK) < 0)) {
		if (filename)
			fprintf(stdout, "%s: does not exist, using global config file\n",
				filename);
		filename = LEDMON_DEF_CONF_FILE;
	}

	f = fopen(filename, "re");
	if (!f) {
		fprintf(stdout, "%s: does not exist, using built-in defaults\n",
			filename);
	} else {
		while (!feof(f)) {
			if (parse_next(f)) {
				fprintf(stderr, "%s: parse error\n", filename);
				ledmon_free_config();
				fclose(f);
				return STATUS_CONFIG_FILE_ERROR;
			}
		}
		fclose(f);
	}

	if (!list_is_empty(&conf.cntrls_whitelist) &&
	    !list_is_empty(&conf.cntrls_blacklist))
		fprintf(stdout, "Both whitelist and blacklist are specified - ignoring blacklist.");

	return STATUS_SUCCESS;
}

static char *conf_list_to_str(struct list *list)
{
	char buf[BUFSIZ];
	char *elem;

	memset(buf, 0, sizeof(buf));
	list_for_each(list, elem) {
		if (elem) {
			int curr = strlen(buf);

			snprintf(buf + curr, sizeof(buf) - curr, "%s,", elem);
		}
	}

	return str_dup(buf);
}

int ledmon_write_shared_conf(void)
{
	char buf[BUFSIZ];
	char *whitelist = NULL;
	char *blacklist = NULL;
	void *shared_mem_ptr;
	int fd = shm_open(LEDMON_SHARE_MEM_FILE, O_RDWR | O_CREAT, 0644);

	if (fd == -1)
		return STATUS_FILE_OPEN_ERROR;

	if (ftruncate(fd, sizeof(buf)) != 0) {
		close(fd);
		return STATUS_FILE_WRITE_ERROR;
	}

	shared_mem_ptr = mmap(NULL, sizeof(buf), PROT_WRITE, MAP_SHARED, fd, 0);
	if (shared_mem_ptr == MAP_FAILED) {
		close(fd);
		return STATUS_FILE_WRITE_ERROR;
	}

	snprintf(buf, sizeof(buf),
		 "BLINK_ON_INIT=%d\n", conf.blink_on_init);
	snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
		 "BLINK_ON_MIGR=%d\n", conf.blink_on_migration);
	snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
		 "LOG_LEVEL=%u\n", conf.log_level);
	snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
		 "LOG_PATH=%s\n", conf.log_path);
	snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
		 "RAID_MEMBERS_ONLY=%d\n", conf.raid_members_only);
	snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
		 "REBUILD_BLINK_ON_ALL=%d\n", conf.rebuild_blink_on_all);
	snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
		 "INTERVAL=%d\n", conf.scan_interval);
	whitelist = conf_list_to_str(&conf.cntrls_whitelist);
	if (whitelist) {
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			 "WHITELIST=%s\n", whitelist);
		free(whitelist);
	}
	blacklist = conf_list_to_str(&conf.cntrls_blacklist);
	if (blacklist) {
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			 "BLACKLIST=%s\n", blacklist);
		free(blacklist);
	}

	memcpy(shared_mem_ptr, buf, strlen(buf));
	munmap(shared_mem_ptr, strlen(buf));
	close(fd);

	return STATUS_SUCCESS;
}

int ledmon_remove_shared_conf(void)
{
	return shm_unlink(LEDMON_SHARE_MEM_FILE);
}

