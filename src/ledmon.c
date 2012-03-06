/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Monitor Service
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
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/select.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "status.h"
#include "ibpi.h"
#include "pidfile.h"
#include "list.h"
#include "utils.h"
#include "sysfs.h"
#include "block.h"
#include "slave.h"
#include "raid.h"
#include "cntrl.h"
#include "version.h"
#include "scsi.h"
#include "ahci.h"

/**
 * Default suspend time between sysfs scan operations, given in seconds.
 */
#define DEFAULT_SLEEP_INTERVAL    10

/**
 * Minimum suspend time between sysfs scan operations, given in seconds.
 */
#define MINIMUM_SLEEP_INTERVAL    5

/**
 * @brief List of active block devices.
 *
 * This list holds all block devices attached to supported storage controllers.
 * Only devices which have enclosure management feature enabled are on the
 * list, other devices are ignored (except protocol is forced).
 */
static void *block_list = NULL;

/**
 * @brief Daemon process termination flag.
 *
 * This flag indicates that daemon process should terminate. User must send
 * SIGTERM to daemon in order to terminate the process gently.
 */
static sig_atomic_t terminate = 0;

/**
 * @brief The interval between sysfs scans.
 *
 * This static variable holds the value of time interval used by the daemon to
 * set sleep between two scans of sysfs. The time interval is given in seconds.
 * The value is set to DEFAULT_SLEEP_INTERVAL by default and it can be change
 * with appropriate command line option. The minimum time interval is 5 seconds.
 */
static int sleep_interval = DEFAULT_SLEEP_INTERVAL;

/**
 * @brief Name of IBPI patterns.
 *
 * This is internal array with names of IBPI patterns. Logging routines use this
 * entries to translate enumeration type into the string.
 */
const char *ibpi_str[] = {
  [IBPI_PATTERN_UNKNOWN]        = "None",
  [IBPI_PATTERN_NORMAL]         = "Off",
  [IBPI_PATTERN_ONESHOT_NORMAL] = "Oneshot Off",
  [IBPI_PATTERN_DEGRADED]       = "In a Critical Array",
  [IBPI_PATTERN_REBUILD]        = "Rebuild",
  [IBPI_PATTERN_REBUILD_P]      = "Rebuild Preferred",
  [IBPI_PATTERN_FAILED_ARRAY]   = "In a Failed Array",
  [IBPI_PATTERN_HOTSPARE]       = "Hotspare",
  [IBPI_PATTERN_PFA]            = "Predicted Failure Analysis",
  [IBPI_PATTERN_FAILED_DRIVE]   = "Failure",
  [IBPI_PATTERN_LOCATE]         = "Locate",
  [IBPI_PATTERN_LOCATE_OFF]     = "Locate Off"
};

/**
 * Internal variable of monitor service. It is the pattern used to print out
 * information about the version of monitor service.
 */
static char *ledmon_version = "Intel(R) Enclosure LED Monitor Service %d.%d\n" \
  "Copyright (C) 2009 Intel Corporation. All rights reserved.\n";

/**
 * Internal variable of monitor service. It is used to help parse command line
 * short options.
 */
static char *shortopt = "t:c:hvl:";

/**
 * Internal enumeration type. It is used to help parse command line arguments.
 */
enum longopt {
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
  OPT_WARNING
};

/**
 * Internal array with option tokens. It is used to help parse command line
 * long options.
 */
static struct option longopt[] = {
  [OPT_ALL]      = {"all", no_argument, NULL, '\0'},
  [OPT_CONFIG]   = {"config", required_argument, NULL, 'c'},
  [OPT_DEBUG]    = {"debug", no_argument, NULL, '\0'},
  [OPT_ERROR]    = {"error", no_argument, NULL, '\0'},
  [OPT_HELP]     = {"help", no_argument, NULL, 'h'},
  [OPT_INFO]     = {"info", no_argument, NULL, '\0'},
  [OPT_INTERVAL] = {"interval", required_argument, NULL, 't'},
  [OPT_LOG]      = {"log", required_argument, NULL, 'l'},
  [OPT_QUIET]    = {"quiet", no_argument, NULL, '\0'},
  [OPT_VERSION]  = {"version", no_argument, NULL, 'v'},
  [OPT_WARNING]  = {"warning", no_argument, NULL, '\0'},
  {NULL, no_argument, NULL, '\0'}
};

/**
 * @brief Monitor service finalize function.
 *
 * This is internal function of monitor service. It is used to finalize daemon
 * process i.e. free allocated memory, unlock and remove pidfile and close log
 * file and syslog. The function is registered as on_exit() handler.
 *
 * @param[in]     status          The function ignores this parameter.
 * @param[in]     progname        The name of the binary file. This argument
 *                                is passed via on_exit() function.
 *
 * @return The function does not return a value.
 */
static void _ledmon_fini(int __attribute__((unused)) status, void *progname)
{
  sysfs_fini();
  if (block_list) {
    list_for_each(block_list, block_device_fini);
    list_fini(block_list);
  }
  log_close();
  pidfile_remove(progname);
}

/**
 * @brief Puts exit status to a log file.
 *
 * This is internal function of monitor service. It is used to report an exit
 * status of the monitor service. The message is logged in to syslog and to log
 * file. The function is registered as on_exit() hander.
 *
 * @param[in]     status            Status given in the last call to exit()
 *                                  function.
 * @param[in]     ignore            Pointer to placeholder where ignore flag is
 *                                  stored. If flag is set 0 then parent process
 *                                  is exiting, otherwise a child is exiting.
 *                                  This argument must not be NULL pointer.
 *
 * @return The function does not return a value.
 */
static void _ledmon_status(int status, void *ignore)
{
  if (*((int *)ignore) != 0) {
    log_info("exit status is %s.", strstatus(status));
  } else if (status != STATUS_SUCCESS) {
    log_error("parent exit status is %s.", strstatus(status));
  }
}

/**
 * @brief Displays the credits.
 *
 * This is internal function of monitor service. It prints out the name and
 * version of the program. It displays the copyright notice and information
 * about the author and license, too.
 *
 * @return The function does not return a value.
 */
static void _ledmon_version(void)
{
  printf(ledmon_version, VERSION_MAJOR, VERSION_MINOR);
  printf("\nThis is free software; see the source for copying conditions." \
         " There is NO warranty;\nnot even for MERCHANTABILITY or FITNESS" \
         " FOR A PARTICULAR PURPOSE.\n\n");
}

/**
 * @brief Displays the help.
 *
 * This is internal function of monitor service. The function prints the name
 * and version of the program out. It displays the usage and available options
 * and its arguments (if any). Each option is described. This is an extract
 * from user manual page.
 *
 * @return The function does not return a value.
 */
static void _ledmon_help(void)
{
  printf(ledmon_version, VERSION_MAJOR, VERSION_MINOR);
  printf("\nUsage: %s [OPTIONS]\n\n", progname);
  printf("Mandatory arguments for long options are mandatory for short " \
    "options, too.\n\n");
  printf("--interval=VALUE\t\t  Set time interval to VALUE seconds.\n" \
         "\t\t\t\t  The smallest interval is 5 seconds.\n");
  printf("--config=PATH,  -c PATH\t\t  Use alternate configuration file " \
         "(not yet\n\t\t\t\t  implemented).\n");
  printf("--log=PATH\t\t\t  Use local log file instead\n\t\t\t\t  /var/log/" \
         "ledmon.log global file.\n");
  printf("--help\t\t\t\t  Displays this help text.\n");
  printf("--version\t\t\t  Displays version and license information.\n\n");
  printf("Refer to ledmon(8) man page for more detailed description.\n");
  printf("Report bugs to: artur.wojcik@intel.com\n\n");
}

/**
 * @brief Sets the path to configuration file.
 *
 * This is internal function of monitor service. This function sets the path and
 * name of configuration file. The function is checking whether the given path is
 * valid or it is invalid and should be ignored.
 *
 * @param[in]      path           the new location and name of config file.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
static status_t _set_config_path(const char *path)
{
  (void)path;
  return STATUS_SUCCESS;
}

/**
 * @brief Sets the path to local log file.
 *
 * This is internal function of monitor service. This function sets the path and
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
static status_t _set_log_path(const char *path)
{
  char temp[PATH_MAX];

  if (realpath(path, temp) == NULL) {
    if ((errno != ENOENT) && (errno != ENOTDIR)) {
      return STATUS_INVALID_PATH;
    }
  }
  if (log_open(temp) < 0) {
    return STATUS_FILE_OPEN_ERROR;
  }
  return STATUS_SUCCESS;
}

/**
 * @brief Sets the value of sleep interval.
 *
 * This function is used by command line handler to set new value of time
 * interval, @see time_interval for details.
 *
 * @param[in]     optarg          String containing the new value of time
 *                                interval, given in command line option.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
static status_t _set_sleep_interval(const char *optarg)
{
  sleep_interval = atoi(optarg);
  if (sleep_interval < MINIMUM_SLEEP_INTERVAL) {
    log_warning("sleep interval too small... using default.");
    sleep_interval = DEFAULT_SLEEP_INTERVAL;
  }
  return STATUS_SUCCESS;
}

/**
 * @brief Command line interface handler function.
 *
 * This is internal function of monitor service. This function interprets the
 * options and commands given to the program from command line interface.
 *
 * @param[in]     argc            - number of arguments.
 * @param[in]     argv            - array of command line arguments.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
static status_t _cmdline_parse(int argc, char *argv[])
{
  int opt, opt_index = -1;
  status_t status = STATUS_SUCCESS;

  do {
    opt = getopt_long(argc, argv, shortopt, longopt, &opt_index);
    if (opt == -1) {
      break;
    }
    switch (opt) {
    case 0:
      switch (opt_index) {
      case OPT_ALL:
        verbose = VERB_ALL;
        break;
      case OPT_DEBUG:
        verbose = VERB_DEBUG;
        break;
      case OPT_ERROR:
        verbose = VERB_ERROR;
        break;
      case OPT_INFO:
        verbose = VERB_INFO;
        break;
      case OPT_QUIET:
        verbose = VERB_QUIET;
        break;
      case OPT_WARNING:
        verbose = VERB_WARN;
        break;
      }
      break;
    case 'l':
      status = _set_log_path(optarg);
      break;
    case 't':
      status = _set_sleep_interval(optarg);
      break;
    case 'c':
      status = _set_config_path(optarg);
      break;
    case 'v':
      _ledmon_version();
      exit(EXIT_SUCCESS);
    case 'h':
      _ledmon_help();
      exit(EXIT_SUCCESS);
    case ':':
    case '?':
    default:
      log_debug("[opt='%c', opt_index=%d]", opt, opt_index);
      break;
    }
    opt_index = -1;
    if (status != STATUS_SUCCESS) {
      return status;
    }
  } while (1);

  return STATUS_SUCCESS;
}

/**
 * @brief SIGTERM handler function.
 *
 * This is internal function of monitor service.
 *
 * @param[in]    signum          - the number of signal received.
 *
 * @return The function does not return a value.
 */
static void _ledmon_sig_term(int signum)
{
  if (signum == SIGTERM) {
    log_info("SIGTERM caught - terminating daemon process.");
    terminate = 1;
  }
}

/**
 * @brief Configures signal handlers.
 *
 * This is internal function of monitor services. It sets to ignore SIGALRM,
 * SIGHUP and SIGPIPE signals. The function installs a handler for SIGTERM
 * signal. User must send SIGTERM to daemon process in order to shutdown the
 * daemon gently.
 *
 * @return The function does not return a value.
 */
static void _ledmon_setup_signals(void)
{
  struct sigaction act;
  sigset_t sigset;

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGALRM);
  sigaddset(&sigset, SIGHUP);
  sigaddset(&sigset, SIGTERM);
  sigaddset(&sigset, SIGPIPE);
  sigaddset(&sigset, SIGUSR1);
  sigprocmask(SIG_BLOCK, &sigset, NULL);

  act.sa_handler = SIG_IGN;
  act.sa_flags = 0;
  sigaction(SIGALRM, &act, NULL);
  sigaction(SIGHUP, &act, NULL);
  sigaction(SIGPIPE, &act, NULL);
  act.sa_handler = _ledmon_sig_term;
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGUSR1, &act, NULL);

  sigprocmask(SIG_UNBLOCK, &sigset, NULL);
}

/**
 * @brief Puts the calling process into sleep.
 *
 * This is internal function of monitor service. The function puts the calling
 * process into a sleep for the given amount of time (expressed in seconds). The
 * function will give control back to the process as soon as time elapses or
 * SIGTERM occurs.
 *
 * @param[in]    seconds         - the time interval given in seconds.
 *
 * @return The function does not return a value.
 */
static void _ledmon_wait(int seconds)
{
  fd_set rd;
  int fd;
  struct timespec timeout;
  sigset_t sigset;

  sigprocmask(SIG_UNBLOCK, NULL, &sigset);
  sigdelset(&sigset, SIGTERM);
  timeout.tv_nsec = 0;
  timeout.tv_sec = seconds;
  FD_ZERO(&rd);
  fd = open("/proc/mdstat", O_RDONLY);
  if (fd) {
    FD_SET(fd, &rd);
  }
  pselect(fd + 1, NULL, NULL, &rd, &timeout, &sigset);
  if (fd >= 0) {
    close(fd);
  }
}

/**
 * @brief Checks the presence of block device.
 *
 * This is internal function of monitor service. The function is checking
 * whether block device is already on the list or it is missing from the list.
 * The function is design to be used as 'test' parameter for list_find_first()
 * function.
 *
 * @param[in]    blk1            - an element from a list to compare to.
 * @param[in]    blk2            - a block device being searched.
 *
 * @return 0 if the block devices do not match, otherwise function returns 1.
 */
static int _compare(struct block_device *blk1, struct block_device *blk2)
{
  return (strcmp(blk1->sysfs_path, blk2->sysfs_path) == 0);
}

/**
 * @brief Adds the block device to list.
 *
 * This is internal function of monitor service. The function adds a block
 * device to the block_list list or if the device is already on the list it
 * updates the IBPI state of the given device. The function updates timestamp
 * value which indicates the time of last structure modification.  The function
 * is design to be used as 'action' parameter of list_for_each() function.
 * Each change of state is logged to the file and to the syslog.
 *
 * @param[in]    block           Pointer to block device structure.
 *
 * @return The function does not return a value.
 */
static void _add_block(struct block_device *block)
{
  struct block_device *temp;

  temp = list_first_that(block_list, _compare, block);
  if (temp) {
    enum ibpi_pattern ibpi = temp->ibpi;
    temp->timestamp = block->timestamp;
    if (temp->ibpi == IBPI_PATTERN_ONESHOT_NORMAL) {
      temp->ibpi = IBPI_PATTERN_UNKNOWN;
    } else if (temp->ibpi != IBPI_PATTERN_FAILED_DRIVE) {
      if (block->ibpi == IBPI_PATTERN_UNKNOWN) {
        if ((temp->ibpi != IBPI_PATTERN_UNKNOWN) && (temp->ibpi != IBPI_PATTERN_NORMAL)) {
          temp->ibpi = IBPI_PATTERN_ONESHOT_NORMAL;
        } else {
          temp->ibpi = IBPI_PATTERN_UNKNOWN;
        }
      } else {
        temp->ibpi = block->ibpi;
      }
    } else {
      temp->ibpi = IBPI_PATTERN_ONESHOT_NORMAL;
    }
    if (ibpi != temp->ibpi) {
      log_info("CHANGE %s: from '%s' to '%s'.", block->sysfs_path,
          ibpi_str[ibpi], ibpi_str[temp->ibpi]);
    }
    temp->cntrl = block->cntrl;
  } else {
    temp = block_device_duplicate(block);
    if (temp != NULL) {
      log_info("NEW %s: state '%s'.", temp->sysfs_path, ibpi_str[temp->ibpi]);
      list_put(block_list, temp, sizeof(struct block_device));
      free(temp);
    }
  }
}

/**
 * @brief Sends LED control message.
 *
 * This is internal function of monitor service. The function sends a LED
 * command to storage controller or enclosure device. The function checks
 * the time of last modification of block device structure. If the timestamp
 * is different then the current global timestamp this means the device is
 * missing due to hot-remove or hardware failure so it must be reported on
 * LEDs appropriately. Note that controller device attached to this block
 * device points to invalid pointer so it must be 'refreshed'.
 *
 * @param[in]    block            Pointer to block device structure.
 *
 * @return The function does not return a value.
 */
static void _send_msg(struct block_device *block)
{
  if (block->timestamp != timestamp) {
    block->cntrl = block_get_controller(sysfs_get_cntrl_devices(),
                                        block->cntrl_path);
    if (block->ibpi != IBPI_PATTERN_FAILED_DRIVE) {
      log_info("CHANGE %s: from '%s' to '%s'.", block->sysfs_path,
          ibpi_str[block->ibpi], ibpi_str[IBPI_PATTERN_FAILED_DRIVE]);
      block->ibpi = IBPI_PATTERN_FAILED_DRIVE;
    }
  }
  block->send_fn(block, block->ibpi);
}

/**
 * @brief Sets a list of block devices and sends LED control messages.
 *
 * This is internal function of monitor service. Based on current layout of
 * sysfs tree the function extracts block devices and for each block device it
 * send LED control message to storage controller or enclosure. The message is
 * determine by appropriate field in block device's structure. See _add_block()
 * and _send_msg() functions description for more details.
 *
 * @return The function does not return a value.
 */
static void _ledmon_execute(void)
{
  sysfs_block_device_for_each(_add_block);
  list_for_each(block_list, _send_msg);
}

/**
 */
int main(int argc, char *argv[])
{
  status_t status = STATUS_SUCCESS;

  set_invocation_name(argv[0]);
  openlog(progname, LOG_PID | LOG_PERROR, LOG_DAEMON);

  if (getuid() != 0) {
    log_error("Only root can run this application.");
    return STATUS_NOT_A_PRIVILEGED_USER;
  }

  if (on_exit(_ledmon_status, &terminate)) {
    return STATUS_ONEXIT_ERROR;
  }
  if (_cmdline_parse(argc, argv) != STATUS_SUCCESS) {
      return STATUS_CMDLINE_ERROR;
  }

  if (pidfile_check(progname, NULL) == 0) {
    log_warning("daemon is running...");
    return STATUS_LEDMON_RUNNING;
  }

  pid_t pid = fork();
  if (pid < 0) {
    log_debug("main(): fork() failed (errno=%d).", errno);
    exit(EXIT_FAILURE);
  }
  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  pid_t sid = setsid();
  if (sid < 0) {
    log_debug("main(): setsid() failed (errno=%d).", errno);
    exit(EXIT_FAILURE);
  }
  for (int i = getdtablesize(); i >= 0; --i) {
    close(i);
  }
  int t = open("/dev/null", O_RDWR);
  dup(t);
  dup(t);
  umask(027);

  if (chdir("/") < 0) {
    log_debug("main(): chdir() failed (errno=%d).", errno);
    exit(EXIT_FAILURE);
  }
  if (pidfile_create(progname)) {
    log_debug("main(): pidfile_creat() failed.");
    exit(EXIT_FAILURE);
  }
  _ledmon_setup_signals();

  if (on_exit(_ledmon_fini, progname)) {
    exit(STATUS_ONEXIT_ERROR);
  }
  if ((status = list_init(&block_list)) != STATUS_SUCCESS) {
    log_debug("main(): list_init() failed (status=%s).", strstatus(status));
    exit(EXIT_FAILURE);
  }
  if ((status = sysfs_init()) != STATUS_SUCCESS) {
    log_debug("main(): sysfs_init() failed (status=%s).", strstatus(status));
    exit(EXIT_FAILURE);
  }
  log_info("monitor service has been started...");
  while (terminate == 0) {
    timestamp = time(NULL);
    if ((status = sysfs_scan()) != STATUS_SUCCESS) {
      log_debug("main(): sysfs_scan() failed (status=%s).", strstatus(status));
    } else {
      _ledmon_execute();
      if ((status = sysfs_reset()) != STATUS_SUCCESS) {
        log_debug("main(): sysfs_reset() failed (status=%s).", strstatus(status));
      }
    }
    _ledmon_wait(sleep_interval);
  }
  exit(EXIT_SUCCESS);
}
