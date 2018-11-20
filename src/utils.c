/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2018 Intel Corporation.
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

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <regex.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "config.h"
#include "list.h"
#include "status.h"
#include "utils.h"

/**
 */
#define PREFIX_DEBUG          "  DEBUG: "
#define PREFIX_WARNING        "WARNING: "
#define PREFIX_INFO           "   INFO: "
#define PREFIX_ERROR          "  ERROR: "

/**
 */
#define TIMESTAMP_PATTERN    "%b %d %T "

/**
 * Name of the executable. It is the last section of invocation path.
 */
char *progname = NULL;

/**
 */
static FILE *s_log = NULL;

struct log_level_info {
	char prefix[10];
	int priority;
};

struct log_level_info log_level_infos[] = {
		[LOG_LEVEL_DEBUG] = {PREFIX_DEBUG, LOG_DEBUG},
		[LOG_LEVEL_WARNING] = {PREFIX_WARNING, LOG_WARNING},
		[LOG_LEVEL_INFO] = {PREFIX_INFO, LOG_INFO},
		[LOG_LEVEL_ERROR] = {PREFIX_ERROR, LOG_ERR}

};

/*
 * Function returns a content of a text file. See utils.h for details.
 */
char *get_text(const char *path, const char *name)
{
	char temp[PATH_MAX];

	str_cpy(temp, path, PATH_MAX);
	str_cat(temp, PATH_DELIM_STR, PATH_MAX);
	str_cat(temp, name, PATH_MAX);

	return buf_read(temp);
}

/*
 * Function returns integer value (1 or 0) based on a boolean value ('Y' or 'N')
 * read from a text file. See utils.h for details.
 */
int get_bool(const char *path, int defval, const char *name)
{
	char *p = get_text(path, name);
	if (p) {
		if (*p == 'Y')
			defval = 1;
		else if (*p == 'N')
			defval = 0;
		free(p);
	}
	return defval;
}

/*
 * Function returns 64-bit unsigned integer value read from a text file. See
 * utils.h for details.
 */
uint64_t get_uint64(const char *path, uint64_t defval, const char *name)
{
	char *p = get_text(path, name);
	if (p) {
		sscanf(p, "%" SCNx64, &defval);
		free(p);
	}
	return defval;
}

/*
 * Function returns integer value read from a text file.
 * See utils.h for details.
 */
int get_int(const char *path, int defval, const char *name)
{
	char *p = get_text(path, name);
	if (p) {
		defval = atoi(p);
		free(p);
	}
	return defval;
}

/**
 */
int scan_dir(const char *path, struct list *result)
{
	struct dirent *dirent;
	int ret = 0;
	DIR *dir = opendir(path);
	if (!dir)
		return -1;

	list_init(result, NULL);

	while ((dirent = readdir(dir)) != NULL) {
		char *str;
		size_t len;

		if ((strcmp(dirent->d_name, ".") == 0) ||
		    (strcmp(dirent->d_name, "..")) == 0)
			continue;

		len = strlen(path) + strlen(dirent->d_name) + 2;
		str = malloc(len);
		if (!str) {
			ret = -1;
			list_erase(result);
			break;
		}

		snprintf(str, len, "%s/%s", path, dirent->d_name);

		list_append(result, str);
	}
	closedir(dir);

	return ret;
}

/**
 */
static int _is_virtual(int dev_type)
{
	switch (dev_type) {
	case 0:		/* sysfs  */
	case 3:		/* procfs */
		return 1;
	}
	return 0;
}

/**
 */
ssize_t buf_write(const char *path, const char *buf)
{
	int fd;
	ssize_t size = -1;

	if (path == NULL)
		__set_errno_and_return(EINVAL);
	if ((buf == NULL) || (strlen(buf) == 0))
		__set_errno_and_return(ENODATA);
	fd = open(path, O_WRONLY);
	if (fd >= 0) {
		size = write(fd, buf, strlen(buf));
		close(fd);
	}
	return size;
}

/**
 */
char *buf_read(const char *path)
{
	struct stat st;
	int fd, size;
	char *buf, *t;

	if (stat(path, &st) < 0)
		return NULL;
	if (st.st_size == 0) {
		if (!_is_virtual(st.st_dev))
			return NULL;
		st.st_size = st.st_blksize;
	}
	if (_is_virtual(st.st_dev))
		st.st_size = st.st_blksize;
	t = buf = malloc(st.st_size);
	if (buf) {
		fd = open(path, O_RDONLY);
		if (fd >= 0) {
			size = read(fd, buf, st.st_size);
			close(fd);
			if (size > 0)
				t = strchrnul(buf, '\n');
		}
		*t = '\0';
	}
	return buf;
}

/**
 */
void get_id(const char *path, struct device_id *did)
{
	char *t, *p;

	if (did && path) {
		did->major = did->minor = -1;
		p = buf_read(path);
		if (p) {
			t = strchr(p, ':');
			if (t) {
				*(t++) = '\0';
				did->major = atoi(p);
				did->minor = atoi(t);
			}
			free(p);
		}
	}
}

/**
 */
static void _log_timestamp(void)
{
	time_t timestamp;
	struct tm *t;
	char buf[30];

	timestamp = time(NULL);
	t = localtime(&timestamp);

	if (t) {
		strftime(buf, sizeof(buf), TIMESTAMP_PATTERN, t);
		fprintf(s_log, "%s", buf);
	}
}

/**
 */
int log_open(const char *path)
{
	if (s_log)
		log_close();

	s_log = fopen(path, "a");
	if (s_log == NULL)
		return -1;
	return 0;
}

/**
 */
void log_close(void)
{
	if (s_log) {
		fflush(s_log);
		fclose(s_log);
		s_log = NULL;
	}
	closelog();
}

/**
 */
void _log(enum log_level_enum loglevel, const char *buf,  ...)
{
	va_list vl;
	struct log_level_info *lli = &log_level_infos[loglevel];

	if (s_log == NULL)
		log_open(conf.log_path);

	if (s_log && (conf.log_level >= loglevel)) {
		_log_timestamp();
		fprintf(s_log, lli->prefix);
		va_start(vl, buf);
		vfprintf(s_log, buf, vl);
		va_end(vl);
		fprintf(s_log, END_LINE_STR);
		fflush(s_log);
		va_start(vl, buf);
		vsyslog(lli->priority, buf, vl);
		va_end(vl);
	}
}

/**
 * @brief Sets program's short name.
 *
 * This is internal function of monitor service. It is used to extract the name
 * of executable file from command line argument.
 *
 * @param[in]     invocation_name - the pointer to command line argument
 *                                  with the invocation name.
 *
 * @return The function does not return a value.
 */
void set_invocation_name(char *invocation_name)
{
#ifdef program_invocation_short_name
	(void)invocation_name;
	progname = program_invocation_short_name;
#else
	char *t = rindex(invocation_name, PATH_DELIM);
	if (t)
		progname = t + 1;
	else
		progname = invocation_name;
#endif				/* program_invocation_short_name */
}

/**
 */
char *str_cpy(char *dest, const char *src, size_t size)
{
	strncpy(dest, src, size - 1);
	dest[size - 1] = '\0';
	return dest;
}

/**
 */
char *str_dup(const char *src)
{
	if (src && (strlen(src) > 0))
		return strdup(src);
	return NULL;
}

/**
 */
char *str_cat(char *dest, const char *src, size_t size)
{
	int t = strlen(dest);
	strncat(dest, src, size - t);
	if (t + strlen(src) >= size)
		dest[size - 1] = '\0';
	return dest;
}

char *get_path_hostN(const char *path)
{
	char *c = NULL, *s = NULL, *p = strdup(path);
	if (!p)
		return NULL;
	c = strstr(p, "host");
	if (!c)
		goto end;
	s = strchr(c, '/');
	if (!s)
		goto end;
	*s = 0;
	s = strdup(c);
 end:
	free(p);
	return s;
}

char *get_path_component_rev(const char *path, int index)
{
	int i;
	char *c = NULL, *p = strdup(path);
	char *result = NULL;
	for (i = 0; i <= index; i++) {
		if (c)
			*c = '\0';
		c = strrchr(p, '/');
	}
	if (c)
		result = strdup(c + 1);
	free(p);
	return result;
}

char *truncate_path_component_rev(const char *path, int index)
{
	int i;
	char *c = NULL, *p = str_dup(path);
	if (!p)
		return NULL;

	for (i = 0; i <= index; i++) {
		if (c)
			*c = '\0';
		c = strrchr(p, '/');
	}
	c = strdup(p);
	free(p);
	return c;
}

int match_string(const char *string, const char *pattern)
{
	int status;
	regex_t regex;

	if (!string || !pattern)
		return 0;

	if (strcmp(string, pattern) == 0)
		return 1;

	status = regcomp(&regex, pattern, REG_EXTENDED);
	if (status != 0) {
		log_debug("regecomp failed, ret=%d", status);
		return 0;
	}

	status = regexec(&regex, string, 0, NULL, 0);
	if (status != 0)
		return 0;

	return 1;
}

int get_log_fd(void)
{
	return fileno(s_log);
}

void print_opt(const char *long_opt, const char *short_opt, const char *desc)
{
	printf("%-20s%-10s%s\n", long_opt, short_opt, desc);
}

/**
 * @brief Sets the path to local log file.
 *
 * This function sets the path and
 * file name of log file. The function checks if the specified path is valid. In
 * case of incorrect path the function does nothing.
 *
 * @param[in]      path           new location and name of log file.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 *         The following status code are returned:
 *
 *         STATUS_INVALID_PATH    the given path is invalid.
 *         STATUS_FILE_OPEN_ERROR unable to open a log file i.e. because of
 *                                insufficient privileges.
 */
status_t set_log_path(const char *path)
{
	char temp[PATH_MAX];
	char *resolved, *logdir, *cpath;

	/*
	 * Extract directory from path
	 */
	cpath = strdup(path);
	logdir = dirname(cpath);

	/*
	 * Check if directory exists
	 */
	resolved = realpath(logdir, temp);
	if (resolved == NULL) {
		printf("%s: %s\n", strerror(errno), logdir);
		free(cpath);
		return STATUS_INVALID_PATH;
	}

	free(cpath);
	/*
	 * Resolve path to log and open it
	 */
	resolved = realpath(path, temp);
	if (log_open(resolved) < 0)
		return STATUS_FILE_OPEN_ERROR;

	if (conf.log_path)
		free(conf.log_path);
	conf.log_path = strdup(resolved);

	return STATUS_SUCCESS;
}

/**
 * Internal array with option tokens. It is used to help parse command line
 * long options.
 */
struct option longopt_all[] = {
	[OPT_ALL]          = {"all", no_argument, NULL, '\0'},
	[OPT_CONFIG]       = {"config", required_argument, NULL, 'c'},
	[OPT_DEBUG]        = {"debug", no_argument, NULL, '\0'},
	[OPT_ERROR]        = {"error", no_argument, NULL, '\0'},
	[OPT_HELP]         = {"help", no_argument, NULL, 'h'},
	[OPT_INFO]         = {"info", no_argument, NULL, '\0'},
	[OPT_INTERVAL]     = {"interval", required_argument, NULL, 't'},
	[OPT_LOG]          = {"log", required_argument, NULL, 'l'},
	[OPT_QUIET]        = {"quiet", no_argument, NULL, '\0'},
	[OPT_VERSION]      = {"version", no_argument, NULL, 'v'},
	[OPT_WARNING]      = {"warning", no_argument, NULL, '\0'},
	[OPT_LOG_LEVEL]    = {"log-level", required_argument, NULL, '\0'},
	[OPT_LIST_CTRL]    = {"list-controllers", no_argument, NULL, 'L'},
	[OPT_LISTED_ONLY]  = {"listed-only", no_argument, NULL, 'x'},
	[OPT_NULL_ELEMENT] = {NULL, no_argument, NULL, '\0'}
};

void setup_options(struct option **longopt, char **shortopt, int *options, int
		options_nr)
{
	int i, j = 0;
	struct option *opt;
	*longopt = malloc(sizeof(struct option) * (options_nr + 1));
	*shortopt = calloc(options_nr * 2 + 1, sizeof(char));
	for (i = 0; i < options_nr; i++) {
		opt = &longopt_all[options[i]];
		(*longopt)[i] = *opt;
		if (opt->val != '\0') {
			(*shortopt)[j++] = (char) opt->val;
			if (opt->has_arg)
				(*shortopt)[j++] = ':';
		}
	}
	longopt[i] = &longopt_all[OPT_NULL_ELEMENT];
	shortopt[j] = '\0';
}

/**
 * @brief Gets id for given CLI option which corresponds to value from longopt
 * table.
 *
 * This is internal function of monitor service. The function maps given string
 * to the value from longopt enum and returns id of matched element. Generic
 * parameters allow to use this function for any CLI options-table which bases
 * on option struct.
 *
 * @param[in]     optarg          String containing value given by user in CLI.
 *
 * @return integer id if successful, otherwise a -1.
 */
int get_option_id(const char *optarg)
{
	int i = 0;

	while (longopt_all[i].name != NULL) {
		if (strcmp(longopt_all[i].name, optarg) == 0)
			return i;
		i++;
	}
	return -1;
}


/**
 * @brief Sets verbose variable to given level.
 *
 * This is internal function of monitor service. The function maps given level
 * to the value from verbose_level enum and sets verbose value to ledmon
 * configuration.
 *
 * @param[in]      log_level     required new log_level.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t set_verbose_level(int log_level)
{
	int new_verbose = -1;

	switch (log_level) {
	case OPT_ALL:
		new_verbose = LOG_LEVEL_ALL;
		break;
	case OPT_DEBUG:
		new_verbose = LOG_LEVEL_DEBUG;
		break;
	case OPT_ERROR:
		new_verbose = LOG_LEVEL_ERROR;
		break;
	case OPT_INFO:
		new_verbose = LOG_LEVEL_INFO;
		break;
	case OPT_QUIET:
		new_verbose = LOG_LEVEL_QUIET;
		break;
	case OPT_WARNING:
		new_verbose = LOG_LEVEL_WARNING;
		break;
	}
	if (new_verbose != -1) {
		conf.log_level = new_verbose;
		return STATUS_SUCCESS;
	}
	return STATUS_CMDLINE_ERROR;
}
