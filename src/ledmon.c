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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "ahci.h"
#include "block.h"
#include "cntrl.h"
#include "config.h"
#include "ibpi.h"
#include "list.h"
#include "pidfile.h"
#include "raid.h"
#include "scsi.h"
#include "slave.h"
#include "smp.h"
#include "status.h"
#include "sysfs.h"
#include "udev.h"
#include "utils.h"
#include "version.h"
#include "vmdssd.h"

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
static void *ledmon_block_list = NULL;

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
	[IBPI_PATTERN_LOCATE_OFF]     = "Locate Off",
	[IBPI_PATTERN_ADDED]          = "Added",
	[IBPI_PATTERN_REMOVED]        = "Removed"
};

/**
 * Internal variable of monitor service. It is the pattern used to print out
 * information about the version of monitor service.
 */
static char *ledmon_version = "Intel(R) Enclosure LED Monitor Service %d.%d\n"
			      "Copyright (C) 2009-2017 Intel Corporation.\n";

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
static void _ledmon_fini(int __attribute__ ((unused)) status, void *progname)
{
	sysfs_fini();
	if (ledmon_block_list) {
		list_for_each(ledmon_block_list, block_device_fini);
		list_fini(ledmon_block_list);
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
	if (*((int *)ignore) != 0)
		log_info("exit status is %s.", strstatus(status));
	else if (status != STATUS_SUCCESS)
		log_error("parent exit status is %s.", strstatus(status));
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
	printf("\nThis is free software; see the source for copying conditions."
	       " There is NO warranty;\nnot even for MERCHANTABILITY or FITNESS"
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
	printf("Mandatory arguments for long options are mandatory for short "
	       "options, too.\n\n");
	printf("--interval=VALUE\t\t  Set time interval to VALUE seconds.\n"
	       "\t\t\t\t  The smallest interval is 5 seconds.\n");
	printf("--config=PATH,  -c PATH\t\t  Use alternate configuration file "
	       "(not yet\n\t\t\t\t  implemented).\n");
	printf
	    ("--log=PATH\t\t\t  Use local log file instead\n\t\t\t\t  /var/log/"
	     "ledmon.log global file.\n");
	printf("--help\t\t\t\t  Displays this help text.\n");
	printf
	    ("--version\t\t\t  Displays version and license information.\n\n");
	printf("Refer to ledmon(8) man page for more detailed description.\n");
	printf(
	"Bugs should be reported at: https://github.com/intel/ledmon/issues\n");
}

/**
 * @brief Sets the path to configuration file.
 *
 * This is internal function of monitor service. This function sets the path and
 * name of configuration file. The function is checking whether the given path
 * is valid or it is invalid and should be ignored.
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
		if ((errno != ENOENT) && (errno != ENOTDIR))
			return STATUS_INVALID_PATH;
	}
	if (log_open(temp) < 0)
		return STATUS_FILE_OPEN_ERROR;
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
		if (opt == -1)
			break;
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
		if (status != STATUS_SUCCESS)
			return status;
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
	sigemptyset(&act.sa_mask);
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
	int fd, udev_fd, max_fd;
	fd_set rdfds, exfds;
	struct timespec timeout;
	sigset_t sigset;

	sigprocmask(SIG_UNBLOCK, NULL, &sigset);
	sigdelset(&sigset, SIGTERM);
	timeout.tv_nsec = 0;
	timeout.tv_sec = seconds;

	FD_ZERO(&rdfds);
	FD_ZERO(&exfds);

	fd = open("/proc/mdstat", O_RDONLY);
	if (fd >= 0)
		FD_SET(fd, &exfds);

	udev_fd = get_udev_monitor();
	if (udev_fd >= 0)
		FD_SET(udev_fd, &rdfds);

	max_fd = MAX(fd, udev_fd);
	while (pselect(max_fd + 1, &rdfds, NULL, &exfds, &timeout, &sigset) > 0) {
		/* ignore 'change' udev event */
		if (!FD_ISSET(udev_fd, &rdfds) ||
		    handle_udev_event(ledmon_block_list) <= 0)
			break;
	}

	if (fd >= 0)
		close(fd);
}

/**
 * @brief Adds the block device to list.
 *
 * This is internal function of monitor service. The function adds a block
 * device to the ledmon_block_list list or if the device is already on the list
 * it updates the IBPI state of the given device. The function updates timestamp
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

	temp = list_first_that(ledmon_block_list, block_compare, block);
	if (temp) {
		enum ibpi_pattern ibpi = temp->ibpi;
		temp->timestamp = block->timestamp;
		if (temp->ibpi == IBPI_PATTERN_ADDED) {
			temp->ibpi = IBPI_PATTERN_ONESHOT_NORMAL;
		} else if (temp->ibpi == IBPI_PATTERN_ONESHOT_NORMAL) {
			temp->ibpi = IBPI_PATTERN_UNKNOWN;
		} else if (temp->ibpi != IBPI_PATTERN_FAILED_DRIVE) {
			if (block->ibpi == IBPI_PATTERN_UNKNOWN) {
				if ((temp->ibpi != IBPI_PATTERN_UNKNOWN) &&
				    (temp->ibpi != IBPI_PATTERN_NORMAL)) {
					temp->ibpi =
					    IBPI_PATTERN_ONESHOT_NORMAL;
				} else {
					temp->ibpi = IBPI_PATTERN_UNKNOWN;
				}
			} else {
				temp->ibpi = block->ibpi;
			}
		} else {
			temp->ibpi = block->ibpi;
		}

		if (ibpi != temp->ibpi) {
			log_info("CHANGE %s: from '%s' to '%s'.",
				 temp->sysfs_path, ibpi_str[ibpi],
				 ibpi_str[temp->ibpi]);
		}
		/* Check if name of the device changed.*/
		if (strcmp(temp->sysfs_path, block->sysfs_path)) {
			log_info("NAME CHANGED %s to %s",
				 temp->sysfs_path, block->sysfs_path);
			free(temp->sysfs_path);
			temp->sysfs_path = strdup(block->sysfs_path);
		}
	} else {
		/* Device not found, it's a new one! */
		temp = block_device_duplicate(block);
		if (temp != NULL) {
			log_info("NEW %s: state '%s'.", temp->sysfs_path,
				 ibpi_str[temp->ibpi]);
			list_put(ledmon_block_list, temp,
				 sizeof(struct block_device));
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
 * LEDs appropriately. Note that controller device and host attached to this
 * block device points to invalid pointer so it must be 'refreshed'.
 *
 * @param[in]    block            Pointer to block device structure.
 *
 * @return The function does not return a value.
 */
static void _send_msg(struct block_device *block)
{
	if (!block->cntrl && !block->pci_slot) {
		log_debug("Missing cntrl for dev: %s. Not sending anything.",
			  strstr(block->sysfs_path, "host"));
		return;
	}
	if (block->timestamp != timestamp ||
	    block->ibpi == IBPI_PATTERN_REMOVED) {
		if (block->ibpi != IBPI_PATTERN_FAILED_DRIVE) {
			log_info("CHANGE %s: from '%s' to '%s'.",
				 block->sysfs_path, ibpi_str[block->ibpi],
				 ibpi_str[IBPI_PATTERN_FAILED_DRIVE]);
			block->ibpi = IBPI_PATTERN_FAILED_DRIVE;
		} else {
			char *host = strstr(block->sysfs_path, "host");
			log_debug("DETACHED DEV '%s' in failed state",
				  host ? host : block->sysfs_path);
		}
	}
	block->send_fn(block, block->ibpi);
	block->ibpi_prev = block->ibpi;
}

static void _flush_msg(struct block_device *block)
{
	if (!block->cntrl)
		return;
	block->flush_fn(block);
}

static void _revalidate_dev(struct block_device *block)
{
	/* Bring back controller and host to the device. */
	block->cntrl = block_get_controller(sysfs_get_cntrl_devices(),
					    block->cntrl_path);
	block->pci_slot = vmdssd_find_pci_slot(block->sysfs_path);
	if (!block->cntrl) {
		/* It could be removed VMD drive */
		if (!block->pci_slot)
			log_debug("Failed to get controller for dev: %s, ctrl path: %s",
				  block->sysfs_path, block->cntrl_path);
		return;
	}
	if (block->cntrl->cntrl_type == CNTRL_TYPE_SCSI) {
		block->host = block_get_host(block->cntrl, block->host_id);
		if (block->host) {
			if (dev_directly_attached(block->sysfs_path))
				cntrl_init_smp(NULL, block->cntrl);
		} else {
			log_debug("Failed to get host for dev: %s, hostId: %d",
				  block->sysfs_path, block->host_id);
			/* If failed, invalidate cntrl */
			block->cntrl = NULL;
		}
	}
	return;
}

static void _invalidate_dev(struct block_device *block)
{
	/* Those fields are valid only per 'session' - through single scan. */
	block->cntrl = NULL;
	block->host = NULL;
	block->pci_slot = NULL;
}

static void _check_block_dev(struct block_device *block, int *restart)
{
	if (!block->cntrl) {
		if  (!block->pci_slot)
			(*restart)++;
		return;
	}
	/* Check SCSI device behind expander. */
	if (block->cntrl->cntrl_type == CNTRL_TYPE_SCSI) {
		if (dev_directly_attached(block->sysfs_path) == 0) {
			if (block->ibpi == IBPI_PATTERN_FAILED_DRIVE &&
			    (block->encl_index == -1
			     || block->encl_dev[0] == 0)) {
				(*restart)++;
				log_debug("%s(): invalidating device: %s. "
					"No link to enclosure", __func__,
					strstr(block->sysfs_path, "host"));
			}
		}
	}
	return;
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
	int restart = 0;	/* ledmon_block_list needs restart? */
	status_t status = STATUS_SUCCESS;

	/* Revalidate each device in the list. Bring back controller and host */
	list_for_each(ledmon_block_list, _revalidate_dev);
	/* Scan all devices and compare them against saved list */
	sysfs_block_device_for_each(_add_block);
	/* Send message to all devices in the list if needed. */
	list_for_each(ledmon_block_list, _send_msg);
	/* Flush unsent messages from internal buffers. */
	list_for_each(ledmon_block_list, _flush_msg);
	/* Check if there is any orphaned device. */
	list_for_each_parm(ledmon_block_list, _check_block_dev, &restart);

	if (restart) {
		/* there is at least one detached element in the list. */
		list_for_each(ledmon_block_list, block_device_fini);
		list_fini(ledmon_block_list);
		status = list_init(&ledmon_block_list);
		if (status != STATUS_SUCCESS) {
			log_debug("%s(): list_init() failed (status=%s).",
				  __func__, strstatus(status));
			exit(EXIT_FAILURE);
		}
	}
}

/**
 */
int main(int argc, char *argv[])
{
	status_t status = STATUS_SUCCESS;
	int i;

	set_invocation_name(argv[0]);
	openlog(progname, LOG_PID | LOG_PERROR, LOG_DAEMON);

	if (getuid() != 0) {
		log_error("Only root can run this application.");
		return STATUS_NOT_A_PRIVILEGED_USER;
	}

	if (on_exit(_ledmon_status, &terminate))
		return STATUS_ONEXIT_ERROR;
	if (_cmdline_parse(argc, argv) != STATUS_SUCCESS)
		return STATUS_CMDLINE_ERROR;

	if (pidfile_check(progname, NULL) == 0) {
		log_warning("daemon is running...");
		return STATUS_LEDMON_RUNNING;
	}

	pid_t pid = fork();
	if (pid < 0) {
		log_debug("main(): fork() failed (errno=%d).", errno);
		exit(EXIT_FAILURE);
	}
	if (pid > 0)
		exit(EXIT_SUCCESS);

	pid_t sid = setsid();
	if (sid < 0) {
		log_debug("main(): setsid() failed (errno=%d).", errno);
		exit(EXIT_FAILURE);
	}
	for (i = getdtablesize() - 1; i >= 0; --i)
		close(i);
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

	if (on_exit(_ledmon_fini, progname))
		exit(STATUS_ONEXIT_ERROR);
	status = list_init(&ledmon_block_list);
	if (status != STATUS_SUCCESS) {
		log_debug("main(): list_init() failed (status=%s).",
			  strstatus(status));
		exit(EXIT_FAILURE);
	}
	status = sysfs_init();
	if (status != STATUS_SUCCESS) {
		log_debug("main(): sysfs_init() failed (status=%s).",
			  strstatus(status));
		exit(EXIT_FAILURE);
	}
	log_info("monitor service has been started...");
	while (terminate == 0) {
		timestamp = time(NULL);
		status = sysfs_scan();
		if (status != STATUS_SUCCESS) {
			log_debug("main(): sysfs_scan() failed (status=%s).",
				  strstatus(status));
		} else {
			_ledmon_execute();
		}
		_ledmon_wait(sleep_interval);
		/* Invalidate each device in the list. Clear controller and host. */
		list_for_each(ledmon_block_list, _invalidate_dev);
		status = sysfs_reset();
		if (status != STATUS_SUCCESS) {
			log_debug("main(): sysfs_reset() failed "
				  "(status=%s).", strstatus(status));
		}
	}
	stop_udev_monitor();
	exit(EXIT_SUCCESS);
}
