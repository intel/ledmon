/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2013 Intel Corporation.
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
 * Verbose level for messages out from application.
 */
enum verbose_level {
	VERB_QUIET = 0,
	VERB_ERROR,
	VERB_WARN,
	VERB_INFO,
	VERB_DEBUG,
	VERB_ALL
};

/**
 * Global variable indicates current level of verbosity.
 */
extern enum verbose_level verbose;

/**
 * This structure describes a device identifier. It consists of major and minor
 * attributes of device.
 */
struct device_id {
	int major, minor;
};

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
 * @param[in]      path           Path to directory to read.
 *
 * @return List containing content of the given directory. Each element on the
 *         list is canonical path.
 */
void *scan_dir(const char *path);

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
int buf_write(const char *path, const char *buf);

/**
 * @brief Reads the content of a text file.
 *
 * The function reads the content of a text file and stores it to memory buffer
 * allocated. The function determines a size of the file and allocates required
 * amount of memory. User is required to free allocated memory as soon as
 * application does not require the content. Use free() function to give memory
 * back to the system pool. The function replaces last end-of-line character with
 * '\0' character.
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
 * @brief Logs an error message.
 *
 * The function logs a message at error level of verbosity.
 *
 * @param[in]      buf            Buffer containing format of a message.
 * @param[in]      ...            Additional arguments according to format of
 *                                a message.
 *
 * @return The function does not return a value.
 */
void log_error(const char *buf, ...);

/**
 * @brief Logs a debug message.
 *
 * The function logs a message at debug level of verbosity.
 *
 * @param[in]      buf            Buffer containing format of a message.
 * @param[in]      ...            Additional arguments according to format of
 *                                a message.
 *
 * @return The function does not return a value.
 */
void log_debug(const char *buf, ...);

/**
 * @brief Logs a warning message.
 *
 * The function logs a message at warning level of verbosity.
 *
 * @param[in]      buf            Buffer containing format of a message.
 * @param[in]      ...            Additional arguments according to format of
 *                                a message.
 *
 * @return The function does not return a value.
 */
void log_warning(const char *buf, ...);

/**
 * @brief Logs a information message.
 *
 * The function logs a message at info level of verbosity.
 *
 * @param[in]      buf            Buffer containing format of a message.
 * @param[in]      ...            Additional arguments according to format of
 *                                a message.
 *
 * @return The function does not return a value.
 */
void log_info(const char *buf, ...);

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
 * @brief Concatenates text buffers.
 *
 * This function appends source buffer to destination buffer. It is similar to
 * strncat() standard C function except it always returns null-terminated buffer.
 * Second difference is that function calculates itself the amount of free space
 * in destination buffer. If source does not fit in dest then as many bytes are
 * copied as can be fit in destination buffer minus 1 for null-character. Otherwise
 * the source is copied to destination including null-character.
 *
 * @param[in,out]  dest            Pointer to destination buffer.
 * @param[in]      src             Pointer to source buffer.
 * @param[in]      size            Capacity of destination buffer.
 *
 * @return Pointer to destination buffer even if function failed.
 */
char *str_cat(char *dest, const char *src, size_t size);

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

#endif				/* _UTILS_H_INCLUDED_ */
