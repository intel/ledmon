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

#ifndef _UTILS_H_INCLUDED_
#define _UTILS_H_INCLUDED_

#include <getopt.h>
#include <sys/types.h>
#include <stdarg.h>
#include <common/config_file.h>
#include "stdlib.h"
#include "stdint.h"
#include "list.h"
#include "status.h"
#include "syslog.h"
#include <led/libled.h>

struct map {
	char *name;
	int value;
};

/**
 * @brief Get string from map.
 *
 * @param[in] 	scode		status code.
 * @param[in] 	map		map.
 * @return exit status if defined, NULL otherwise
 */
char *str_map(int scode, struct map *map);

/**
 * Value is intentionally unused.
 * Macro to prevent compiler from raising unused result warning.
 */
#define UNUSED(x) do { if (x); } while (0)

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
 * @brief Macro for compile time array size constants
 */
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

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
 * @brief The struct is used for mapping IPBI pattern to the controller internal value.
 */
struct ibpi2value {
	unsigned int ibpi;
	unsigned int value;
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
 * @brief Get the text to dest similar to get_text, but caller supplies buffer.
 *
 * @see get_text
 *
 * @param[in]     path        Directory path to file.
 * @param[in]     name        Name of file.
 * @param[out]    dest        Where to place the contents of file read.
 * @param[in]     dest_len    Size of destination buffer.
 *
 * @return NULL if error(s) encountered else dest.
 */
char *get_text_to_dest(const char *path, const char *name, char *dest, size_t dest_len);

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
 * @brief Check if path contains subpath, starting from beginning.
 *
 * @param[in]      path           Path to check.
 * @param[in]      subpath        Expected subpath.
 * @param[in]      subpath_strlen Subpath string length.
 *
 * @return True if subpath is included on path, False otherwise.
 */
bool is_subpath(const char * const path, const char * const subpath, size_t subpath_strlen);

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
 * @brief some constants for different value types
 *
 */
#define BUF_SZ_SM   64
#define BUF_SZ_NUM  32

/**
 * @brief Same as buf_read, except file contents are placed in dest.
 *
 * @param[in]     path        Directory path to file
 * @param[out]    dest        Where to place the contents of file read
 * @param[in]     dest_len    Size of destination buffer
 * @return NULL if error(s) encountered else dest
 */
char *buf_read_to_dest(const char *path, char *dest, size_t dest_size);

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
int log_open(struct ledmon_conf *conf);

/**
 * @brief Close a local log file.
 *
 * The function closes a local log file. If the file has not been opened the
 * function does nothing.
 *
 * @return The function does not return a value.
 */
void log_close(struct ledmon_conf *conf);

/**
 * @brief Common logging function for both lib_log & _log functions.
 *
 * @param[in]	log_fd		Open file descriptor
 * @param[in]	config_level	Logging level specified in configuration
 * @param[in]	loglevel	What logging level this message is logged at
 * @param[in]	buf		printf formatting string
 * @param[in]	list		variable argument list
 */
void _common_log(int log_fd, enum led_log_level_enum config_level,
		enum led_log_level_enum, const char *buf, va_list list);

/**
 * @brief Logs an message with given loglevel.
 *
 * The function logs a message at given level of verbosity.
 *
 * @param[in]      conf           Ledmon configuration.
 * @param[in]      loglevel       Level of verbosity for a message.
 * @param[in]      buf            Buffer containing format of a message.
 * @param[in]      ...            Additional arguments according to format of
 *                                a message.
 *
 * @return The function does not return a value.
 */
void _log(struct ledmon_conf *conf, enum led_log_level_enum loglevel, const char *buf, ...)
		__attribute__ ((format (printf, 3, 4)));

#define log_error(buf, ...)	_log(&conf, LED_LOG_LEVEL_ERROR, buf, ##__VA_ARGS__)
#define log_debug(buf, ...)	_log(&conf, LED_LOG_LEVEL_DEBUG, buf, ##__VA_ARGS__)
#define log_info(buf, ...)	_log(&conf, LED_LOG_LEVEL_INFO, buf, ##__VA_ARGS__)
#define log_warning(buf, ...)	_log(&conf, LED_LOG_LEVEL_WARNING, buf, ##__VA_ARGS__)
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
 * @brief Converts string to long integer.
 *
 * This function works similar to strtol.
 * Writes to destination only if successfully read the number.
 * Destination can be NULL if endptr is set.
 *
 * @param[in]      dest            Pointer to destination.
 * @param[in]      strptr          Pointer to source string.
 * @param[in]      endptr          Pointer to the next character in string after parsed value.
 * @param[in]      base            Base of the numerical value in string.
 *
 * @return 0 if operation was successful, otherwise 1.
 */
int str_tol(signed long *dest, const char *strptr, char **endptr, int base);

/**
 * @brief Converts string to unsigned long integer.
 *
 * This function works similar to strtoul. Skips + and - characters.
 * Writes to destination only if successfully read the number.
 * Destination can be NULL if endptr is set.
 *
 * @param[in]      dest            Pointer to destination.
 * @param[in]      strptr          Pointer to source string.
 * @param[in]      endptr          Pointer to the next character in string after parsed value.
 * @param[in]      base            Base of the numerical value in string.
 *
 * @return 0 if operation was successful, otherwise 1.
 */
int str_toul(unsigned long *dest, const char *strptr, char **endptr, int base);

/**
 * @brief Converts string to integer.
 *
 * This function works similar to strtol, but for int.
 * Writes to destination only if successfully read the number.
 * Destination can be NULL if endptr is set.
 *
 * @param[in]      dest            Pointer to destination.
 * @param[in]      strptr          Pointer to source string.
 * @param[in]      endptr          Pointer to the next character in string after parsed value.
 * @param[in]      base            Base of the numerical value in string.
 *
 * @return 0 if operation was successful, otherwise 1.
 */
int str_toi(signed int *dest, const char *strptr, char **endptr, int base);

/**
 * @brief Converts string to unsigned integer.
 *
 * This function works similar to strtoul, but for unsigned int.
 * Skips + and - characters.
 * Writes to destination only if successfully read the number.
 * Destination can be NULL if endptr is set.
 *
 * @param[in]      dest            Pointer to destination.
 * @param[in]      strptr          Pointer to source string.
 * @param[in]      endptr          Pointer to the next character in string after parsed value.
 * @param[in]      base            Base of the numerical value in string.
 *
 * @return 0 if operation was successful, otherwise 1.
 */
int str_toui(unsigned int *dest, const char *strptr, char **endptr, int base);

/**
 * @brief Extracts the 'hostX' part from path.
 */
char *get_path_hostN(const char *path);

/**
 */
int get_log_fd(struct ledmon_conf *conf);

/**
 */
void print_opt(const char *long_opt, const char *short_opt, const char *desc);

/**
 */
status_t set_log_path(struct ledmon_conf *conf, const char *path);

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
	OPT_LIST_SLOTS,
	OPT_GET_SLOT,
	OPT_SET_SLOT,
	OPT_CNTRL_TYPE,
	OPT_DEVICE,
	OPT_SLOT,
	OPT_STATE,
	OPT_PRINT_PARAM,
	OPT_IBPI,
	OPT_TEST,
	OPT_NULL_ELEMENT
};

extern struct option longopt_all[];
void setup_options(struct option **longopt, char **shortopt, int *options,
			int options_nr);
int get_option_id(const char *optarg);
status_t set_verbose_level(struct ledmon_conf *conf, int log_level);

const char *ibpi2str(enum led_ibpi_pattern ibpi);
enum led_ibpi_pattern string2ibpi(const char *name);

/**
 * @brief Returns ibpi2value entry if IBPI matches
 *
 * @param[in]       ibpi                  IBPI pattern.
 * @param[in]       ibpi2val_arr          IBPI pattern to value array.
 * @param[in]       ibpi2value_arr_cnt    Array entries count.
 *
 * @return Corresponding ibpi2value entry last or entry with LED_IBPI_PATTERN_UNKNOWN
 */
const struct ibpi2value *get_by_ibpi(const enum led_ibpi_pattern ibpi,
				     const struct ibpi2value *ibpi2val_arr,
				     int ibpi2value_arr_cnt);

/**
 * @brief Returns ibpi2value entry if value matches
 *
 * @param[in]       value                 value to compare.
 * @param[in]       ibpi2val_arr          IBPI pattern to value array.
 * @param[in]       ibpi2value_arr_cnt    Array entries count.
 *
 * @return Corresponding ibpi2value entry last or entry with LED_IBPI_PATTERN_UNKNOWN
 */
const struct ibpi2value *get_by_value(const enum led_ibpi_pattern ibpi,
				      const struct ibpi2value *ibpi2val_arr,
				      int ibpi2value_arr_cnt);

/**
 * @brief Returns ibpi2value entry if any bit matches
 *
 * @param[in]       value                 value to compare.
 * @param[in]       ibpi2val_arr          IBPI pattern to value array.
 * @param[in]       ibpi2value_arr_cnt    Array entries count.
 *
 * @return Corresponding ibpi2value entry last or entry with LED_IBPI_PATTERN_UNKNOWN
 */
const struct ibpi2value *get_by_bits(const enum led_ibpi_pattern ibpi,
				     const struct ibpi2value *ibpi2val_arr,
				     int ibpi2value_arr_cnt);

#endif				/* _UTILS_H_INCLUDED_ */
