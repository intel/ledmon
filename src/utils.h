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

#ifndef _UTILS_H_INCLUDED_
#define _UTILS_H_INCLUDED_

#include <getopt.h>
#include "config_file.h"
#include "stdlib.h"
#include "stdint.h"
#include "list.h"
#include "status.h"
#include "syslog.h"

/**
 * Maximum number of bytes in temporary buffer. It is used for local variables.
 */
#define BUFFER_MAX          128

/**
 * Maximum number of bytes in write buffer. It is used for local variables when
 * function needs to write a sysfs attribute.
 */
#define WRITE_BUFFER_SIZE   1024

/**
 * This structure describes a device identifier. It consists of major and minor
 * attributes of device.
 */
struct device_id {
	int major, minor;
};

struct log_level_info {
	char prefix[10];
	int priority;
};

/**
 */
#define PREFIX_DEBUG          "  DEBUG: "
#define PREFIX_WARNING        "WARNING: "
#define PREFIX_INFO           "   INFO: "
#define PREFIX_ERROR          "  ERROR: "

/**
 * This array describes a log levels priorities with message prefix .
 */
extern struct log_level_info log_level_infos[];

/**
 * This global variable holds the name of binary file an application has been
 * executed from.
 */
extern char *progname;

/**
 * @brief Reads integer value from a text file.
 *
 * This function assumes that the only text in a file is requested number to
 * read. In case of an error while reading from file the function will return
 * a value stored in defval argument.
 *
 * @param[in]      path           location of a file to be read.
 * @param[in]      defval         default value to return in case the file
 *                                does not exist.
 * @param[in]      name           name of a file to be read.
 *
 * @return Value read from a file if successful, otherwise a value stored in
 *         defval argument.
 */
int get_int(const char *path, int defval, const char *name);

/**
 * @brief Reads 64-bit unsigned integer from a text file.
 *
 * This function assumes that the only text in a file is requested number to
 * read. In case of an error while reading from file the function will return
 * a value stored in defval argument.
 *
 * @param[in]      path           Path where a file is located.
 * @param[in]      defval         Default value to be returned in case of error.
 * @param[in]      name           Name of a file to be read.
 *
 * @return Value read from a file if successful, otherwise a value stored in
 *         defval argument.
 */
uint64_t get_uint64(const char *path, uint64_t defval, const char *name);

/**
 * @brief Reads a content of a text file.
 *
 * This function reads a text file and return pointer to memory where the text
 * has been stored. The memory allocated by the function must be release as soon
 * as application does not require the content. Use free() function to give
 * allocated memory back to the system.
 *
 * @param[in]      path           Path where a file is located.
 * @param[in]      name           Name of a file to be read.
 *
 * @return Pointer to memory buffer if successful, otherwise NULL pointer.
 */
char *get_text(const char *path, const char *name);

/**
 * @brief Reads boolean value from a text file.
 *
 * This function assumes that the only text in a file is the requested value to
 * read. The recognized boolean values are in the format 'Y' for True and 'N'
 * for False. In case of an error while reading from file the function will
 * return a value stored in defval argument.
 *
 * @param[in]      path           Path where a file is located.
 * @param[in]      defval         Default value to be returned in case of error.
 * @param[in]      name           Name of a file to be read.
 *
 * @return Value read from a file if successful, otherwise a value stored in
 *         defval argument. 1 is returned for True, 0 for False.
 */
int get_bool(const char *path, int defval, const char *name);

/**
 * @brief Writes a text to file.
 *
 * This function writes a text to a file and return the number of bytes written.
 * If the file does not exist or the value is incorrect the function returns -1
 * and errno has additional error information.
 *
 * @param[in]      path           Location of file to write.
 * @param[in]      name           Name of file to write.
 * @param[in]      value          Text to write to a file.
 *
 * @return The number of bytes written or -1 if an error occurred.
 */
int put_text(const char *path, const char *name, const char *value);

/**
 * @brief Writes an integer value to a text file.
 *
 * This function writes an integer value to a text file. If the file does not
 * exist or the value is out of range the function returns -1 and errno variable
 * has additional error information.
 *
 * @param[in]      path           Location of file to write.
 * @param[in]      name           Name of file to write.
 * @param[in]      value          Integer value to write to a file.
 *
 * @return The number of bytes written or -1 if an error occurred.
 */
int put_int(const char *path, const char *name, int value);

/**
 * @brief Scans directory for files.
 *
 * This function reads a directory specified in path argument and puts found
 * file on a list. The function puts a canonical paths on the list, however it
 * does not resolve any symbolic link.
 *
 * This function allocates memory for the list elements. The caller should free
 * it using list_erase().
 *
 * @param[in]      path           Path to directory to read.
 * @param[in]      result         Pointer to list where the directory contents
 *                                will be put.
 *
 * @return 0 on success, -1 on error.
 */
int scan_dir(const char *path, struct list *result);

/**
 * @brief Writes a text to file.
 *
 * This function writes content of text buffer to file. If the file does not
 * exist or a content is invalid the function returns -1 and errno variable has
 * additional error information.
 *
 * @param[in]      path           Location and name of file to write to.
 * @param[in]      buf            Pointer to text buffer to write to a file.
 *
 * @return Number of bytes written if successful, otherwise -1 for an error.
 */
ssize_t buf_write(const char *path, const char *buf);

/**
 * @brief Reads the content of a text file.
 *
 * The function reads the content of a text file and stores it to memory buffer
 * allocated. The function determines a size of the file and allocates required
 * amount of memory. User is required to free allocated memory as soon as
 * application does not require the content. Use free() function to give memory
 * back to the system pool. The function replaces last end-of-line character
 * with '\0' character.
 *
 * @param[in]      path           Path and name of file to read.
 *
 * @return Pointer to memory block if successful, otherwise NULL pointer.
 */
char *buf_read(const char *path);

/**
 * @brief Gets major and minor of device.
 *
 * The function reads from text buffer the major and minor of block device.
 *
 * @param[in]      buf            Pointer to text buffer to interpret.
 * @param[out]     did            Placeholder where major and minor will be
 *                                stored.
 *
 * @return The function does not return a value.
 */
void get_id(const char *buf, struct device_id *did);

/**
 * @brief Open a local log file.
 *
 * The function opens a file to write log messages. If the given file does not
 * exist the new one will be created. If the file already exist the file will be
 * opened in append mode and the pointer will be set at the end of a file.
 *
 * @param[in]      path           Location and name of a log file.
 *
 * @return The function returns 0 if successful, otherwise -1 and errno variable
 *         has additional error information.
 */
int log_open(const char *path);

/**
 * @brief Close a local log file.
 *
 * The function closes a local log file. If the file has not been opened the
 * function does nothing.
 *
 * @return The function does not return a value.
 */
void log_close(void);

/**
 * @brief Logs an message with given loglevel.
 *
 * The function logs a message at given level of verbosity.
 *
 * @param[in]      loglevel       Level of verbosity for a message.
 * @param[in]      buf            Buffer containing format of a message.
 * @param[in]      ...            Additional arguments according to format of
 *                                a message.
 *
 * @return The function does not return a value.
 */
void _log(enum log_level_enum loglevel, const char *buf, ...);

#define log_error(buf, ...)	_log(LOG_LEVEL_ERROR, buf, ##__VA_ARGS__)
#define log_debug(buf, ...)	_log(LOG_LEVEL_DEBUG, buf, ##__VA_ARGS__)
#define log_info(buf, ...)	_log(LOG_LEVEL_INFO, buf, ##__VA_ARGS__)
#define log_warning(buf, ...)	_log(LOG_LEVEL_WARNING, buf, ##__VA_ARGS__)
/**
 */
void set_invocation_name(char *invocation_name);

/**
 * @brief Copies a text buffer.
 *
 * This function copies source text buffer to destination buffer. The function
 * always return a null-terminated buffer even if src does not fit in dest.
 *
 * @param[out]     dest           Pointer to destination buffer.
 * @param[in]      src            Pointer to source buffer.
 * @param[in]      size           Capacity of destination buffer in bytes.
 *
 * @return Pointer to destination buffer even if function failed.
 */
char *str_cpy(char *dest, const char *src, size_t size);

/**
 * @brief Duplicates a text buffer.
 *
 * This function duplicates a text buffer. It allocates a new memory block and
 * copies the content of source buffer to new location. If pointer to source
 * buffer is NULL the function will return NULL, too. The caller is required to
 * free allocated memory as soon as content is not needed.
 *
 * @param[in]      src            Source buffer to duplicate the content.
 *
 * @return Pointer to allocated memory block if successful, otherwise NULL.
 */
char *str_dup(const char *src);

/**
 */
char *truncate_path_component_rev(const char *path, int index);

/**
 */
char *get_path_component_rev(const char *path, int index);

/**
 * @brief Extracts the 'hostX' part from path.
 */
char *get_path_hostN(const char *path);

int match_string(const char *string, const char *pattern);

/**
 */
int get_log_fd(void);

/**
 */
void print_opt(const char *long_opt, const char *short_opt, const char *desc);

/**
 */
status_t set_log_path(const char *path);

/**
 * Internal enumeration type. It is used to help parse command line arguments.
 */
enum opt {
	OPT_ALL,
	OPT_CONFIG,
	OPT_DEBUG,
	OPT_ERROR,
	OPT_HELP,
	OPT_INFO,
	OPT_INTERVAL,
	OPT_LOG,
	OPT_QUIET,
	OPT_VERSION,
	OPT_WARNING,
	OPT_LOG_LEVEL,
	OPT_LIST_CTRL,
	OPT_LISTED_ONLY,
	OPT_FOREGROUND,
	OPT_NULL_ELEMENT
};

extern struct option longopt_all[];
void setup_options(struct option **longopt, char **shortopt, int *options,
			int options_nr);
int get_option_id(const char *optarg);
status_t set_verbose_level(int log_level);
#endif				/* _UTILS_H_INCLUDED_ */
