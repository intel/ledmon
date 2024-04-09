/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2022-2024 Intel Corporation.
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

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <regex.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <sys/sysmacros.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "config.h"
#include "list.h"
#include "status.h"
#include "utils.h"

#include <lib/libled_internal.h>

/**
 */
#define TIMESTAMP_PATTERN    "%b %d %T "

/**
 * Name of the executable. It is the last section of invocation path.
 */
char *progname = NULL;

struct log_level_info log_level_infos[] = {
		[LED_LOG_LEVEL_DEBUG] = {PREFIX_DEBUG, LOG_DEBUG},
		[LED_LOG_LEVEL_WARNING] = {PREFIX_WARNING, LOG_WARNING},
		[LED_LOG_LEVEL_INFO] = {PREFIX_INFO, LOG_INFO},
		[LED_LOG_LEVEL_ERROR] = {PREFIX_ERROR, LOG_ERR}
};

/*
 * Function returns a content of a text file. See utils.h for details.
 */
char *get_text(const char *path, const char *name)
{
	char temp[PATH_MAX];
	int ret;

	ret = snprintf(temp, sizeof(temp), "%s/%s", path, name);
	if (ret < 0 || ret >= PATH_MAX)
		return NULL;
	return buf_read(temp);
}

char *get_text_to_dest(const char *path, const char *name, char *dest, size_t dest_len)
{
	char temp[PATH_MAX];
	int ret;

	ret = snprintf(temp, sizeof(temp), "%s/%s", path, name);
	if (ret < 0 || ret >= PATH_MAX)
		return NULL;
	return buf_read_to_dest(temp, dest, dest_len);
}

/*
 * Function returns integer value (1 or 0) based on a boolean value ('Y' or 'N')
 * read from a text file. See utils.h for details.
 */
int get_bool(const char *path, int defval, const char *name)
{
	char buf[BUF_SZ_SM];
	char *p = get_text_to_dest(path, name, buf, sizeof(buf));
	if (p) {
		if (*p == 'Y')
			defval = 1;
		else if (*p == 'N')
			defval = 0;
	}
	return defval;
}

uint64_t get_uint64(const char *path, uint64_t defval, const char *name)
{
	char buf[BUF_SZ_NUM];
	char *p = get_text_to_dest(path, name, buf, sizeof(buf));

	if (!p)
		return defval;

	str_toul(&defval, p, NULL, 16);
	return defval;
}

int get_int(const char *path, int defval, const char *name)
{
	char buf[BUF_SZ_NUM];
	char *p = get_text_to_dest(path, name, buf, sizeof(buf));

	if (!p)
		return defval;

	str_toi(&defval, p, NULL, 10);
	return defval;
}

bool is_subpath(const char * const path, const char * const subpath, size_t subpath_strlen)
{
	assert(path && subpath);

	if (strncmp(path, subpath, subpath_strlen) == 0)
		return true;
	return false;
}

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

		if (!list_append(result, str)) {
			free(str);
			ret = -1;
			list_erase(result);
			break;
		}
	}
	closedir(dir);

	return ret;
}

/**
 */
static int _is_virtual(dev_t dev_type)
{
	switch (major(dev_type)) {
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

	if (buf == NULL || strnlen(buf, WRITE_BUFFER_SIZE) == 0)
		__set_errno_and_return(ENODATA);

	fd = open(path, O_WRONLY);
	if (fd >= 0) {
		size = write(fd, buf, strnlen(buf, WRITE_BUFFER_SIZE));
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

	buf = calloc(1, st.st_size + 1);	/* Add a byte for string termination */
	if (!buf)
		return NULL;

	fd = open(path, O_RDONLY);
	if (fd >= 0) {
		size = read(fd, buf, st.st_size);
		close(fd);
		if (size > 0) {
			buf[size] = '\0';
			t = strchrnul(buf, '\n');
			*t = '\0';
		} else {
			buf[0] = '\0'; /* Set empty string if no data was read */
		}
	}

	return buf;
}

char *buf_read_to_dest(const char *path, char *dest, size_t dest_size)
{
	int fd, size;
	char *t = NULL;

	if (!dest || dest_size < 1)
		return NULL;

	memset(dest, 0, dest_size);

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return NULL;

	size = read(fd, dest, dest_size - 1);
	close(fd);
	if (size > 0) {
		dest[size] = '\0';
		t = strchrnul(dest, '\n');
		*t = '\0';
		return dest;
	}
	return NULL;
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
				str_toi(&did->major, p, NULL, 10);
				str_toi(&did->minor, t, NULL, 10);
			}
			free(p);
		}
	}
}

/**
 */
static void _log_timestamp(int log_fd)
{
	time_t timestamp;
	struct tm *t;
	char buf[30];

	timestamp = time(NULL);
	t = localtime(&timestamp);

	if (t) {
		strftime(buf, sizeof(buf), TIMESTAMP_PATTERN, t);
		dprintf(log_fd, "%s", buf);
	}
}

/**
 */
int log_open(struct ledmon_conf *conf)
{
	if (conf->s_log)
		log_close(conf);

	conf->s_log = fopen(conf->log_path, "a");
	if (conf->s_log == NULL)
		return -1;
	return 0;
}

/**
 */
void log_close(struct ledmon_conf *conf)
{
	if (conf->s_log) {
		fflush(conf->s_log);
		fclose(conf->s_log);
		conf->s_log = NULL;
	}
}


void _common_log(int log_fd, enum led_log_level_enum config_level, enum led_log_level_enum loglevel,
		const char *buf, va_list list)
{
	if (config_level >= loglevel && log_fd >= 0) {
		char msg[4096];
		struct log_level_info *lli = &log_level_infos[loglevel];

		vsnprintf(msg, sizeof(msg), buf, list);
		_log_timestamp(log_fd);
		dprintf(log_fd, "%s", lli->prefix);
		dprintf(log_fd, "%s\n", msg);
		fsync(log_fd);
		syslog(lli->priority, "%s", msg);
	}
}

/**
 */
void _log(struct ledmon_conf *conf, enum led_log_level_enum loglevel, const char *buf,  ...)
{
	va_list vl;
	if (conf->s_log == NULL)
		log_open(conf);

	va_start(vl, buf);
	_common_log(get_log_fd(conf), conf->log_level, loglevel, buf, vl);
	va_end(vl);
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
	char *t = strrchr(invocation_name, PATH_DELIM);
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

static int _str_to_num(const char *strptr, char **endptr, int base, unsigned long *n, int is_signed)
{
	char *c;
	int sign_occurred = 0;

	assert(n);
	if (!strptr)
		return 1;
	errno = 0;
	if (is_signed)
		*n = (unsigned long)strtol(strptr, &c, base);
	else {
		while ((*strptr == '-' || *strptr == '+' || isspace(*strptr)) &&
				*strptr != '\0' && !sign_occurred) {
			if (*strptr == '-' || *strptr == '+')
				sign_occurred = 1;
			strptr++;
		}
		*n = strtoul(strptr, &c, base);
	}
	if (errno != 0 || strptr == c)
		return 1;
	if (endptr)
		*endptr = c;
	return 0;
}

static int _str_to_num_signed(const char *strptr, char **endptr, int base, unsigned long *n)
{
	return _str_to_num(strptr, endptr, base, n, 1);
}

static int _str_to_num_unsigned(const char *strptr, char **endptr, int base, unsigned long *n)
{
	return _str_to_num(strptr, endptr, base, n, 0);
}

static inline int value_in_range_signed(long value, long min, long max)
{
	return (value >= min && value <= max);
}

static inline int value_in_range_unsigned(unsigned long value, unsigned long min, unsigned long max)
{
	return (value >= min && value <= max);
}

#define _DECL_STR_TO_NUM(name, sign, type, min, max) \
int name(sign type * dest, const char *strptr, char **endptr, int base) \
{ \
	unsigned long n; \
	if ((!dest && !endptr) || _str_to_num_##sign(strptr, endptr, base, &n)) \
		return 1; \
	if (!value_in_range_##sign(n, min, max)) \
		return 1; \
	if (dest)\
		*dest = (sign type)n; \
	return 0; \
}

_DECL_STR_TO_NUM(str_tol, signed, long, LONG_MIN, LONG_MAX)
_DECL_STR_TO_NUM(str_toul, unsigned, long, 0, ULONG_MAX)
_DECL_STR_TO_NUM(str_toi, signed, int, INT_MIN, INT_MAX)
_DECL_STR_TO_NUM(str_toui, unsigned, int, 0, UINT_MAX)

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

int get_log_fd(struct ledmon_conf *conf)
{
	if (conf->s_log)
		return fileno(conf->s_log);
	return -1;
}

void print_opt(const char *long_opt, const char *short_opt, const char *desc)
{
	printf("%-70s%-40s%s\n", long_opt, short_opt, desc);
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
 *         STATUS_OUT_OF_MEMORY
 */
status_t set_log_path(struct ledmon_conf *conf, const char *path)
{
	char temp[PATH_MAX];
	char log_file[PATH_MAX];
	char *resolved, *logdir, *logfile, *cpath;

	/*
	 * Extract directory from path
	 */
	cpath = strdup(path);
	if (!cpath)
		return STATUS_OUT_OF_MEMORY;
	logdir = dirname(cpath);

	/*
	 * Check if directory exists
	 */
	resolved = realpath(logdir, temp);
	if (resolved == NULL) {
		_log(conf, LED_LOG_LEVEL_ERROR, "%s: %s\n", strerror(errno), logdir);
		free(cpath);
		return STATUS_INVALID_PATH;
	}

	free(cpath);
	cpath = strdup(path);
	if (!cpath)
		return STATUS_OUT_OF_MEMORY;

	logfile = basename(cpath);

	snprintf(log_file, sizeof(log_file), "%s/%s",
		 resolved, logfile);
	free(cpath);

	if (conf->log_path)
		free(conf->log_path);
	conf->log_path = strdup(log_file);
	if (!conf->log_path)
		return STATUS_OUT_OF_MEMORY;

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
	[OPT_FOREGROUND]   = {"foreground", no_argument, NULL, '\0'},
	[OPT_LIST_SLOTS]   = {"list-slots", no_argument, NULL, 'P'},
	[OPT_GET_SLOT]     = {"get-slot", no_argument, NULL, 'G'},
	[OPT_SET_SLOT]     = {"set-slot", no_argument, NULL, 'S'},
	[OPT_CNTRL_TYPE]   = {"controller-type", required_argument, NULL, 'n'},
	[OPT_DEVICE]       = {"device", required_argument, NULL, 'd'},
	[OPT_SLOT]         = {"slot", required_argument, NULL, 'p'},
	[OPT_STATE]        = {"state", required_argument, NULL, 's'},
	[OPT_PRINT_PARAM]  = {"print", required_argument, NULL, 'r'},
	[OPT_TEST]         = {"test", no_argument, NULL, 'T'},
	[OPT_IBPI]         = {"ibpi", no_argument, NULL, 'I' },
	[OPT_NULL_ELEMENT] = {NULL, no_argument, NULL, '\0'}
};

void setup_options(struct option **_longopt, char **_shortopt, int *options, int
		options_nr)
{
	struct option *longopt;
	char *shortopt;
	int i, j = 0;
	struct option *opt;

	longopt = malloc(sizeof(struct option) * (options_nr + 1));
	shortopt = calloc(options_nr * 2 + 1, sizeof(char));
	if (!longopt || !shortopt) {
		fprintf(stderr, "Out of memory\n");
		exit(STATUS_OUT_OF_MEMORY);
	}
	for (i = 0; i < options_nr; i++) {
		opt = &longopt_all[options[i]];
		longopt[i] = *opt;
		if (opt->val != '\0') {
			shortopt[j++] = (char) opt->val;
			if (opt->has_arg)
				shortopt[j++] = ':';
		}
	}
	longopt[i] = longopt_all[OPT_NULL_ELEMENT];
	shortopt[j] = '\0';

	*_longopt = longopt;
	*_shortopt = shortopt;
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
status_t set_verbose_level(struct ledmon_conf *conf, int log_level)
{
	int new_verbose = -1;

	switch (log_level) {
	case OPT_ALL:
		new_verbose = LED_LOG_LEVEL_ALL;
		break;
	case OPT_DEBUG:
		new_verbose = LED_LOG_LEVEL_DEBUG;
		break;
	case OPT_ERROR:
		new_verbose = LED_LOG_LEVEL_ERROR;
		break;
	case OPT_INFO:
		new_verbose = LED_LOG_LEVEL_INFO;
		break;
	case OPT_QUIET:
		new_verbose = LED_LOG_LEVEL_QUIET;
		break;
	case OPT_WARNING:
		new_verbose = LED_LOG_LEVEL_WARNING;
		break;
	}
	if (new_verbose != -1) {
		conf->log_level = new_verbose;
		return STATUS_SUCCESS;
	}
	return STATUS_CMDLINE_ERROR;
}

/**
 * @brief Mapping IBPI states to strings. There are 2 strings:
 *        - logs, uppercase for readability, should be set for all patterns.
 *        - user input, user friendly names, could not be set.
 */
struct ibpi2names {
	enum led_ibpi_pattern ibpi;
	char *log_name;
	char *input_name;
};

const struct ibpi2names ipbi_names[] = {
	{LED_IBPI_PATTERN_REBUILD,		"REBUILD",		"rebuild"},
	{LED_IBPI_PATTERN_LOCATE,		"LOCATE",		"locate"},
	{LED_IBPI_PATTERN_LOCATE_OFF,		"LOCATE_OFF",		"locate_off"},
	{LED_IBPI_PATTERN_LOCATE_AND_FAIL,	"LOCATE_AND_FAIL",	"locate_and_failure"},
	{LED_SES_REQ_ABORT,			"SES_ABORT",		"ses_abort"},
	{LED_SES_REQ_REBUILD,			"SES_REBUILD",		"ses_rebuild"},
	{LED_SES_REQ_IFA,			"SES_IFA",		"ses_ifa"},
	{LED_SES_REQ_ICA,			"SES_ICA",		"ses_ica"},
	{LED_SES_REQ_CONS_CHECK,		"SES_CONS_CHECK",	"ses_cons_check"},
	{LED_SES_REQ_HOTSPARE,			"SES_HOTSPARE",		"ses_hotspare"},
	{LED_SES_REQ_RSVD_DEV,			"SES_RSVD_DEV",		"ses_rsvd_dev"},
	{LED_SES_REQ_OK,			"SES_OK",		"ses_ok"},
	{LED_SES_REQ_IDENT,			"SES_IDENT",		"ses_ident"},
	{LED_SES_REQ_RM,			"SES_RM",		"ses_rm"},
	{LED_SES_REQ_INS,			"SES_INSERT",		"ses_insert"},
	{LED_SES_REQ_MISSING,			"SES_MISSING",		"ses_missing"},
	{LED_SES_REQ_DNR,			"SES_DNR",		"ses_dnr"},
	{LED_SES_REQ_ACTIVE,			"SES_ACTIVE",		"ses_active"},
	{LED_SES_REQ_EN_BB,			"SES_ENABLE_BB",	"ses_enable_bb"},
	{LED_SES_REQ_EN_BA,			"SES_ENABLE_BA",	"ses_enable_ba"},
	{LED_SES_REQ_DEV_OFF,			"SES_DEVOFF",		"ses_devoff"},
	{LED_SES_REQ_FAULT,			"SES_FAULT",		"ses_fault"},
	{LED_SES_REQ_PRDFAIL,			"SES_PRDFAIL",		"ses_prdfail"},

	/* Special internal patterns, can be only printed */
	{LED_IBPI_PATTERN_UNKNOWN,		"UNKNOWN",		NULL},
	{LED_IBPI_PATTERN_ADDED,		"ADDED",		NULL},
	{LED_IBPI_PATTERN_REMOVED,		"REMOVED",		NULL},
	{LED_IBPI_PATTERN_ONESHOT_NORMAL,	"ONESHOT_NORMAL",	NULL},

	/* Here comes IBPI states with multiple input names defined. */
	{LED_IBPI_PATTERN_NORMAL,	"NORMAL",	"normal"},
	{LED_IBPI_PATTERN_NORMAL,	"NORMAL",	"off"},

	{LED_IBPI_PATTERN_DEGRADED,	"ICA",		"ica"},
	{LED_IBPI_PATTERN_DEGRADED,	"ICA",		"degraded"},

	{LED_IBPI_PATTERN_FAILED_ARRAY,	"IFA",		"ifa"},
	{LED_IBPI_PATTERN_FAILED_ARRAY,	"IFA",		"failed_array"},

	{LED_IBPI_PATTERN_HOTSPARE,	"HOTSPARE",	"hotspare"},
	{LED_IBPI_PATTERN_PFA,		"PFA",		"pfa"},

	{LED_IBPI_PATTERN_FAILED_DRIVE,	"FAILURE",	"failure"},
	{LED_IBPI_PATTERN_FAILED_DRIVE,	"FAILURE",	"disk_failed"},

	{LED_IBPI_PATTERN_COUNT,	"UNKNOWN",	NULL},
};

/**
 * @brief Return log friendly IBPI pattern name.
 *
 * Every led_ibpi_pattern must be defined. Do not put pointer to free.
 */
const char *ibpi2str(enum led_ibpi_pattern ibpi)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ipbi_names); i++)
		if (ipbi_names[i].ibpi == ibpi)
			return ipbi_names[i].log_name;

	/* That should not happen */
	assert(false);
}

enum led_ibpi_pattern string2ibpi(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ipbi_names); i++) {
		char *input_name = ipbi_names[i].input_name;

		if (!input_name)
			continue;

		if (strncmp(input_name, name, strlen(input_name)) == 0)
			return ipbi_names[i].ibpi;
	}

	return LED_IBPI_PATTERN_UNKNOWN;
}

static const struct ibpi2value *get_ibpi2value(const enum led_ibpi_pattern val,
					       const struct ibpi2value *ibpi2val_arr,
					       const int ibpi2value_arr_cnt,
					       bool (*compar)(const unsigned int val,
							      const struct ibpi2value *ibpi2val))
{
	int cnt = 0;
	const struct ibpi2value *tmp = NULL;

	do {
		/* Prevent out of bound results */
		assert(cnt < ibpi2value_arr_cnt);

		tmp = ibpi2val_arr + cnt;
		if (compar(val, tmp))
			return tmp;
		cnt++;
	} while (tmp->ibpi != LED_IBPI_PATTERN_UNKNOWN);

	return tmp;
}

static bool compar_bits(const unsigned int val, const struct ibpi2value *ibpi2val)
{
	if (ibpi2val->value & val)
		return true;
	return false;
}

#define IBPI2VALUE_GET_FN(_name)							\
const struct ibpi2value *get_by_##_name(const enum led_ibpi_pattern ibpi,			\
					const struct ibpi2value *ibpi2val_arr,		\
					int ibpi2value_arr_cnt)				\
{											\
	return get_ibpi2value(ibpi, ibpi2val_arr, ibpi2value_arr_cnt, compar_##_name);	\
}

#define COMPARE(_name)									\
static bool compar_##_name(const enum led_ibpi_pattern val, const struct ibpi2value *ibpi2val)	\
{											\
	if (ibpi2val->_name == val)							\
		return true;								\
	return false;									\
}											\
IBPI2VALUE_GET_FN(_name)								\

IBPI2VALUE_GET_FN(bits);
COMPARE(ibpi);
COMPARE(value);
/**
 * @brief Get string from map.
 *
 * @param[in] 	scode		status code.
 * @param[in] 	map		map.
 * @return exit status if defined, NULL otherwise
 */
char *str_map(int scode, struct map *map)
{
	while(map->name) {
		if (scode == map->value)
			return map->name;
		map++;
	}
	return NULL;
}
