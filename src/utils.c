/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009,2011 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <config.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "status.h"
#include "list.h"
#include "utils.h"

/**
 */
#define DEFAULT_LOG_NAME      "/var/log/%s.log"

/**
 */
#define PREFIX_DEBUG          "  DEBUG: "
#define PREFIX_WARNING        "WARNING: "
#define PREFIX_INFO           "   INFO: "
#define PREFIX_ERROR          "  ERROR: "

/**
 */
#define TIMESTAMP_PATTERN    "0x%08x:0x%08x "

/**
 */
enum verbose_level verbose = VERB_WARN;

/**
 * Name of the executable. It is the last section of invocation path.
 */
char *progname = NULL;

/**
 */
static FILE *s_log = NULL;

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
 * Function returns 64-bit unsigned integer value read from a text file. See
 * utils.h for details.
 */
uint64_t get_uint64(const char *path, uint64_t defval, const char *name)
{
  char *p = get_text(path, name);
  if (p) {
    sscanf(p, "0x%Lx", (long long unsigned int *)&defval);
    free(p);
  }
  return defval;
}

/*
 * Function returns integer value read from a text file. See utils.h for details.
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
void * scan_dir(const char *path)
{
  struct dirent *dirent;
  void *result;
  char temp[PATH_MAX];

  if (list_init(&result) != STATUS_SUCCESS) {
    return NULL;
  }
  DIR *dir = opendir(path);
  if (dir) {
    while ((dirent = readdir(dir)) != NULL) {
      if ((strcmp(dirent->d_name, ".") == 0) || (strcmp(dirent->d_name, "..")) == 0) {
        continue;
      }
      str_cpy(temp, path, PATH_MAX);
      str_cat(temp, PATH_DELIM_STR, PATH_MAX);
      str_cat(temp, dirent->d_name, PATH_MAX);
      list_put(result, temp, strlen(temp) + 1);
    }
    closedir(dir);
  }
  return result;
}

/**
 */
static int _is_virtual(int dev_type)
{
  switch (dev_type) {
  case 0: /* sysfs  */
  case 3: /* procfs */
    return 1;
  }
  return 0;
}

/**
 */
int buf_write(const char *path, const char *buf)
{
  int fd, size = -1;

  if (path == NULL) {
    __set_errno_and_return(EINVAL);
  }
  if ((buf == NULL) || (strlen(buf) == 0)) {
    __set_errno_and_return(ENODATA);
  }
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

  if (stat(path, &st) < 0) {
    return NULL;
  }
  if (st.st_size == 0) {
    if (!_is_virtual(st.st_dev)) {
      return NULL;
    }
    st.st_size = st.st_blksize;
  }
  if (_is_virtual(st.st_dev)) {
    st.st_size = st.st_blksize;
  }
  t = buf = malloc(st.st_size);
  if (buf) {
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
      size = read(fd, buf, st.st_size);
      close(fd);
      if (size > 0) {
        t = strchrnul(buf, '\n');
      }
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
  struct timeval t;
  if (gettimeofday(&t, NULL) == 0) {
    fprintf(s_log, TIMESTAMP_PATTERN, (int)t.tv_sec, (int)t.tv_usec);
  }
}

/**
 */
static int _mkdir(const char *path)
{
  char temp[PATH_MAX];
  int status = -1;
  char *t = realpath(path, temp);
  while (t) {
    t = strchr(t + 1, PATH_DELIM);
    if (t) {
      *t = '\0';
    }
    status = mkdir(temp, 0640);
    if (t) {
      *t = PATH_DELIM;
    }
    if ((status < 0) && (errno != EEXIST)) {
      break;
    }
    status = 0;
  }
  return status;
}

/**
 */
int log_open(const char *path)
{
  if (s_log) {
    log_close();
  }
  char *t = rindex(path, PATH_DELIM);
  if (t) {
    *t = '\0';
  }
  int status = _mkdir(path);
  if (t) {
    *t = PATH_DELIM;
  }
  if (status == 0) {
    if ((s_log = fopen(path, "a")) == NULL) {
      return -1;
    }
  }
  return status;
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
static void _log_open_default(void)
{
  char temp[PATH_MAX];
  sprintf(temp, DEFAULT_LOG_NAME, progname);
  log_open(temp);
}

/**
 */
void log_debug(const char *buf, ...)
{
  va_list vl;

  if (s_log == NULL) {
    _log_open_default();
  }
  if (s_log && (verbose >= VERB_DEBUG)) {
    _log_timestamp();
    fprintf(s_log, PREFIX_DEBUG);
    va_start(vl, buf);
    vfprintf(s_log, buf, vl);
    va_end(vl);
    fprintf(s_log, "\n");
    fflush(s_log);
  }
  va_start(vl, buf);
  vsyslog(LOG_DEBUG, buf, vl);
  va_end(vl);
}

/**
 */
void log_error(const char *buf, ...)
{
  va_list vl;

  if (s_log == NULL) {
    _log_open_default();
  }
  if (s_log && (verbose >= VERB_ERROR)) {
    _log_timestamp();
    fprintf(s_log, PREFIX_ERROR);
    va_start(vl, buf);
    vfprintf(s_log, buf, vl);
    va_end(vl);
    fprintf(s_log, END_LINE_STR);
    fflush(s_log);
  }
  va_start(vl, buf);
  vsyslog(LOG_ERR, buf, vl);
  va_end(vl);
}

/**
 */
void log_warning(const char *buf, ...)
{
  va_list vl;

  if (s_log == NULL) {
    _log_open_default();
  }
  if (s_log && (verbose >= VERB_WARN)) {
    _log_timestamp();
    fprintf(s_log, PREFIX_WARNING);
    va_start(vl, buf);
    vfprintf(s_log, buf, vl);
    va_end(vl);
    fprintf(s_log, END_LINE_STR);
    fflush(s_log);
  }
  va_start(vl, buf);
  vsyslog(LOG_WARNING, buf, vl);
  va_end(vl);
}

/**
 */
void log_info(const char *buf, ...)
{
  va_list vl;

  if (s_log == NULL) {
    _log_open_default();
  }
  if (s_log && (verbose >= VERB_INFO)) {
    _log_timestamp();
    fprintf(s_log, PREFIX_INFO);
    va_start(vl, buf);
    vfprintf(s_log, buf, vl);
    va_end(vl);
    fprintf(s_log, END_LINE_STR);
    fflush(s_log);
  }
  va_start(vl, buf);
  vsyslog(LOG_INFO, buf, vl);
  va_end(vl);
}

/**
 * @brief Sets program's short name.
 *
 * This is internal function of monitor service. It is used to extract the name
 * of executable file from command line argument.
 *
 * @param[in]     invocation_name - the pointer to command line argument with the
 *                                  invocation name.
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
  if (t) {
    progname = t + 1;
  } else {
    progname = invocation_name;
  }
#endif /* program_invocation_short_name */
}

/**
 */
char *str_cpy(char *dest, const char *src, size_t size)
{
  strncpy(dest, src, size - 1);
  if (size < strlen(src)) {
    dest[size - 1] = '\0';
  }
  return dest;
}

/**
 */
char *str_dup(const char *src)
{
  if (src && (strlen(src) > 0)) {
    return strdup(src);
  }
  return NULL;
}

/**
 */
char *str_cat(char *dest, const char *src, size_t size)
{
  int t = strlen(dest);
  strncat(dest, src, size - t);
  if (t + strlen(src) >= size) {
    dest[size - 1] = '\0';
  }
  return dest;
}

char *get_path_component_rev(const char *path, int index)
{
  int i;
  char *c = NULL, *p = strdup(path);
  char *result = NULL;
  for (i = 0; i <= index; i++)
  {
      if (c) {
        *c = '\0';
      }
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

  for (i = 0; i <= index; i++)
  {
      if (c) {
        *c = '\0';
      }
      c = strrchr(p, '/');
  }
  c = strdup(p);
  free(p);
  return c;
}

