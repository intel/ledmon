/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2021 Intel Corporation.
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

#include <config_ac.h>
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
#include "config_file.h"
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
#include "vmdssd.h"

/**
 * @brief List of active block devices.
 *
 * This list holds all block devices attached to supported storage controllers.
 * Only devices which have enclosure management feature enabled are on the
 * list, other devices are ignored (except protocol is forced).
 */
static struct list ledmon_block_list;

/**
 * @brief Daemon process termination flag.
 *
 * This flag indicates that daemon process should terminate. User must send
 * SIGTERM to daemon in order to terminate the process gently.
 */
static sig_atomic_t terminate;

/**
 * @brief Path to ledmon configuration file.
 *
 * This string contains path of the ledmon configuration file. The value is
 * set to LEDMON_DEF_CONF_FILE by default and it can be changed by command line
 * option.
 */
static char *ledmon_conf_path;

/**
 * @brief Boolean flag whether to run foreground or not.
 *
 * This flag is turned on with --foreground option. Primary use of this option
 * is to use it in systemd service file.
 */
static int foreground;

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
static char *ledmon_version = "Intel(R) Enclosure LED Monitor Service %s %s\n"
			      "Copyright (C) 2009-2021 Intel Corporation.\n";

/**
 * Internal variable of monitor service. It is used to help parse command line
 * short options.
 */
static char *shortopt;

struct option *longopt;

static int possible_params[] = {
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
	OPT_FOREGROUND,
};

static int possible_params_size = sizeof(possible_params)
		/ sizeof(possible_params[0]);

/**
 * @brief Monitor service finalize function.
 *
 * This is internal function of monitor service. It is used to finalize daemon
 * process i.e. free allocated memory, unlock and remove pidfile and close log
 * file and syslog. The function is registered as on_exit() handler.
 *
 * @param[in]     status          The function ignores this parameter.
 * @param[in]     program_name    The name of the binary file. This argument
 *                                is passed via on_exit() function.
 *
 * @return The function does not return a value.
 */
static void _ledmon_fini(int __attribute__ ((unused)) status, void *program_name)
{
	sysfs_reset();
	list_erase(&ledmon_block_list);
	log_close();
	pidfile_remove(program_name);
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
 * @param[in]     arg               Argument passed to on_exit().
 *
 * @return The function does not return a value.
 */
static void _ledmon_status(int status, void *arg)
{
	int log_level;
	char message[4096];
	int ignore = *((int *)arg);

	if (ignore)
		return;

	if (status == STATUS_SUCCESS)
		log_level = LOG_LEVEL_INFO;
	else
		log_level = LOG_LEVEL_ERROR;

	snprintf(message, sizeof(message), "exit status is %s.",
		 strstatus(status));

	if (get_log_fd() >= 0)
		_log(log_level, message);
	else
		syslog(log_level_infos[log_level].priority, "%s", message);
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
	printf(ledmon_version, PACKAGE_VERSION, BUILD_LABEL);
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
	printf(ledmon_version, PACKAGE_VERSION, BUILD_LABEL);
	printf("\nUsage: %s [OPTIONS]\n\n", progname);
	printf("Mandatory arguments for long options are mandatory for short "
	       "options, too.\n\n");
	print_opt("--interval=VALUE", "-t VALUE",
			  "Set time interval to VALUE seconds.");
	print_opt("", "", "The smallest interval is 5 seconds.");
	print_opt("--config=PATH", "-c PATH",
			  "Use alternate configuration file.");
	print_opt("--log=PATH", "-l PATH",
			  "Use local log file instead /var/log/ledmon.log");
	print_opt("--log-level=VALUE", "-l VALUE",
			  "Allows user to set ledmon verbose level in logs.");
	print_opt("--foreground", "",
			  "Do not run as daemon.");
	print_opt("--help", "-h", "Displays this help text.");
	print_opt("--version", "-v",
			  "Displays version and license information.");
	printf("\nRefer to ledmon(8) man page for more detailed description.\n");
	printf("Bugs should be reported at: https://github.com/intel/ledmon/issues\n");
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
static status_t _set_config_path(char **conf_path, const char *path)
{
	if (!path)
		path = LEDMON_DEF_CONF_FILE;

	if (*conf_path)
		free(*conf_path);
	*conf_path = str_dup(path);

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
	errno = 0;
	conf.scan_interval = strtol(optarg, NULL, 10);
	if (errno != 0) {
		log_error("Cannot parse sleep interval");
		return STATUS_CMDLINE_ERROR;
	}
	if (conf.scan_interval < LEDMON_MIN_SLEEP_INTERVAL) {
		log_warning("sleep interval too small... using default.");
		conf.scan_interval = LEDMON_DEF_SLEEP_INTERVAL;
	}
	return STATUS_SUCCESS;
}


/**
 * @brief Reads config file path and checks if command line input contains
 * options which don't require to run ledmon as daemon.
 *
 * This is internal function of monitor service. This function looks for
 * config file path in command line options given to the program from
 * command line interface. It also handles options to print help and version.
 *
 * @param[in]     argc            - number of arguments.
 * @param[in]     argv            - array of command line arguments.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
static status_t _cmdline_parse_non_daemonise(int argc, char *argv[])
{
	int opt_index = -1;
	int opt = -1;
	status_t status = STATUS_SUCCESS;

	do {
		opt = getopt_long(argc, argv, shortopt, longopt, &opt_index);
		switch (opt) {
		case 'c':
			status = _set_config_path(&ledmon_conf_path, optarg);
			break;
		case 'h':
			_ledmon_help();
			exit(EXIT_SUCCESS);
		case 'v':
			_ledmon_version();
			exit(EXIT_SUCCESS);
		case ':':
		case '?':
			return STATUS_CMDLINE_ERROR;
		}
	} while (opt >= 0);

	return status;
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

	optind = 1;
	do {
		opt = getopt_long(argc, argv, shortopt, longopt, &opt_index);
		if (opt == -1)
			break;
		if (opt == 'c')
			continue;
		switch (opt) {
		int log_level;
		case 0:
			switch (get_option_id(longopt[opt_index].name)) {
			case OPT_LOG_LEVEL:
				log_level = get_option_id(optarg);
				if (log_level != -1)
					status = set_verbose_level(log_level);
				else
					status = STATUS_CMDLINE_ERROR;
				break;
			case OPT_FOREGROUND:
				foreground = 1;
				break;
			default:
				status = set_verbose_level(
						possible_params[opt_index]);
			}
			break;
		case 'l':
			status = set_log_path(optarg);
			break;
		case 't':
			status = _set_sleep_interval(optarg);
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
	int fd, udev_fd, max_fd, res;
	fd_set rdfds, exfds;
	struct timespec timeout;
	sigset_t sigset;

	sigprocmask(SIG_UNBLOCK, NULL, &sigset);
	sigdelset(&sigset, SIGTERM);
	timeout.tv_nsec = 0;
	timeout.tv_sec = seconds;

	fd = open("/proc/mdstat", O_RDONLY);
	udev_fd = get_udev_monitor();
	max_fd = MAX(fd, udev_fd) + 1;
	do {
		FD_ZERO(&rdfds);
		FD_ZERO(&exfds);

		if (fd > 0)
			FD_SET(fd, &exfds);
		if (udev_fd > 0)
			FD_SET(udev_fd, &rdfds);

		res = pselect(max_fd, &rdfds, NULL, &exfds, &timeout, &sigset);
		if (terminate || !FD_ISSET(udev_fd, &rdfds) ||
		    handle_udev_event(&ledmon_block_list) <= 0)
			break;
	} while (res > 0);

	if (fd >= 0)
		close(fd);
}

/**
 * @brief Determine failed state by comparing saved block device with new
 * scanned.
 *
 * This is internal function of monitor service. Due race conditions related
 * with removing files from /sys/block/md* when raid is stopped or disk is
 * failed, this function analyse state of every block device between scans.
 *
 * @param[in]    block           Pointer to new (scanned) block device
 *				 structure.
 * @param[in]    temp            Pointer to previously saved state of block
 *				 device structure.
 *
 * @return The function does not return a value.
 */
static void _handle_fail_state(struct block_device *block,
			       struct block_device *temp)
{
	struct raid_device *temp_raid_device = NULL;

	if (!temp->raid_dev)
		/*
		 * Device is a RAID member now, so keep information about
		 * related with device RAID.
		 */
		temp->raid_dev = raid_device_duplicate(block->raid_dev);

	if (!temp->raid_dev)
		return;

	temp_raid_device = find_raid_device(sysfs_get_volumes(),
					    temp->raid_dev->sysfs_path);

	if (!block->raid_dev) {
		if (temp->raid_dev->type == DEVICE_TYPE_VOLUME &&
		    temp_raid_device) {
			/*
			 * Device is outside of the volume, but related raid
			 * still exist, so disk has been removed from volume -
			 * blink fail LED. It is case when drive is removed
			 * by mdadm -If.
			 */
			temp->ibpi = IBPI_PATTERN_FAILED_DRIVE;
			/*
			 * Changing failed state to hotspare will be prevent by
			 * code from _add_block function. If disk come back to
			 * container failed state should be removed. By setting
			 * type to CONTAINER ledmon can react in this case.
			 */
			temp->raid_dev->type = DEVICE_TYPE_CONTAINER;
		} else {
			/*
			 * Device was RAID member and was failed (was outside
			 * of array and container). Now again is in container.
			 * Release object to perform hotspare state.
			 * Or:
			 * Device is outside of the volume, but related raid is
			 * removed (or stopped) so device is no longer a RAID
			 * member.
			 */
			raid_device_fini(temp->raid_dev);
			temp->raid_dev = NULL;
		}
	} else if (block->raid_dev) {
		if (temp->raid_dev->type == DEVICE_TYPE_VOLUME &&
		    block->raid_dev->type == DEVICE_TYPE_CONTAINER) {
			/*
			 * Drive is removed from volume, but still exist
			 * in container.
			 */
			enum raid_level new_level;

			if (!temp_raid_device)
				new_level = RAID_LEVEL_UNKNOWN;
			else
				new_level = temp_raid_device->level;

			if ((temp->raid_dev->level == RAID_LEVEL_10 ||
				 temp->raid_dev->level == RAID_LEVEL_1) &&
				 new_level == RAID_LEVEL_0) {
				/*
				 * Device is removed from volume due to
				 * migration to raid. State of this disk is
				 * hotspare now.
				 */
				temp->ibpi = IBPI_PATTERN_HOTSPARE;
			} else {
				/*
				 * Trasitions other than raid 0 migration.
				 * Like reshape, volume stopping etc.
				 */
				if (temp_raid_device) {
					/*
					 * Drive is removed from volume,
					 * but still exist in container. This
					 * situation can be caused by bad
					 * blocks or calling mdadm
					 * --set-faulty.
					 */
					temp->ibpi = IBPI_PATTERN_FAILED_DRIVE;
				}
			}
		} else if (temp->raid_dev->type == DEVICE_TYPE_CONTAINER &&
			   block->raid_dev->type == DEVICE_TYPE_VOLUME) {
			/*
			 * Disk was in container and is added to volume.
			 * Release object for recreating.
			 */
			raid_device_fini(temp->raid_dev);
			temp->raid_dev =
				raid_device_duplicate(block->raid_dev);
		}
	}
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
	struct block_device *temp = NULL;

	list_for_each(&ledmon_block_list, temp) {
		if (block_compare(temp, block))
			break;
		temp = NULL;
	}
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
		} else if (!(temp->ibpi == IBPI_PATTERN_FAILED_DRIVE &&
			block->ibpi == IBPI_PATTERN_HOTSPARE) ||
			(temp->ibpi == IBPI_PATTERN_FAILED_DRIVE &&
			block->ibpi == IBPI_PATTERN_NONE)) {
			temp->ibpi = block->ibpi;
		}

		_handle_fail_state(block, temp);

		if (ibpi != temp->ibpi && ibpi <= IBPI_PATTERN_REMOVED) {
			log_info("CHANGE %s: from '%s' to '%s'.",
				 temp->sysfs_path, ibpi2str(ibpi),
				 ibpi2str(temp->ibpi));
		}
		/* Check if name of the device changed.*/
		if (strcmp(temp->sysfs_path, block->sysfs_path)) {
			log_info("NAME CHANGED %s to %s",
				 temp->sysfs_path, block->sysfs_path);
			free(temp->sysfs_path);
			temp->sysfs_path = str_dup(block->sysfs_path);
		}
	} else {
		/* Device not found, it's a new one! */
		temp = block_device_duplicate(block);
		if (temp != NULL) {
			log_info("NEW %s: state '%s'.", temp->sysfs_path,
				 ibpi2str(temp->ibpi));
			list_append(&ledmon_block_list, temp);
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
	if (!block->cntrl) {
		log_debug("Missing cntrl for dev: %s. Not sending anything.",
			  strstr(block->sysfs_path, "host"));
		return;
	}
	if (block->timestamp != timestamp ||
	    block->ibpi == IBPI_PATTERN_REMOVED) {
		if (block->ibpi != IBPI_PATTERN_FAILED_DRIVE) {
			log_info("CHANGE %s: from '%s' to '%s'.",
				 block->sysfs_path, ibpi2str(block->ibpi),
				 ibpi2str(IBPI_PATTERN_FAILED_DRIVE));
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
	if (!block->cntrl) {
		/* It could be removed VMD drive */
		log_debug("Failed to get controller for dev: %s, ctrl path: %s",
			  block->sysfs_path, block->cntrl_path);
		return;
	}
	if (block->cntrl->cntrl_type == CNTRL_TYPE_SCSI) {
		block->host = block_get_host(block->cntrl, block->host_id);
		if (block->host) {
			if (dev_directly_attached(block->sysfs_path))
				cntrl_init_smp(NULL, block->cntrl);
			else
				scsi_get_enclosure(block);
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
	block->enclosure = NULL;
	block->encl_index = -1;
}

static void _check_block_dev(struct block_device *block, int *restart)
{
	if (!block->cntrl) {
		(*restart)++;
	}
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
	struct block_device *device;

	/* Revalidate each device in the list. Bring back controller and host */
	list_for_each(&ledmon_block_list, device)
		_revalidate_dev(device);
	/* Scan all devices and compare them against saved list */
	list_for_each(sysfs_get_block_devices(), device)
		_add_block(device);
	/* Send message to all devices in the list if needed. */
	list_for_each(&ledmon_block_list, device)
		_send_msg(device);
	/* Flush unsent messages from internal buffers. */
	list_for_each(&ledmon_block_list, device)
		_flush_msg(device);
	/* Check if there is any orphaned device. */
	list_for_each(&ledmon_block_list, device)
		_check_block_dev(device, &restart);

	if (restart) {
		/* there is at least one detached element in the list. */
		list_erase(&ledmon_block_list);
	}
}

static status_t _init_ledmon_conf(void)
{
	memset(&conf, 0, sizeof(struct ledmon_conf));

	/* initialize with default values */
	conf.blink_on_init = 1;
	conf.blink_on_migration = 1;
	conf.rebuild_blink_on_all = 0;
	conf.raid_members_only = 0;
	conf.log_level = LOG_LEVEL_WARNING;
	conf.scan_interval = LEDMON_DEF_SLEEP_INTERVAL;
	list_init(&conf.cntrls_whitelist, NULL);
	list_init(&conf.cntrls_blacklist, NULL);
	return set_log_path(LEDMON_DEF_LOG_FILE);
}

static void _close_parent_fds(void)
{
	struct list dir;

	if (scan_dir("/proc/self/fd", &dir) == 0) {
		char *elem;

		list_for_each(&dir, elem) {
			int fd = (int)strtol(basename(elem), NULL, 10);

			if (fd != get_log_fd())
				close(fd);
		}
		list_erase(&dir);
	}
}

/**
 */
int main(int argc, char *argv[])
{
	status_t status = STATUS_SUCCESS;
	int ignore = 0;

	setup_options(&longopt, &shortopt, possible_params,
			possible_params_size);
	set_invocation_name(argv[0]);
	openlog(progname, LOG_PID | LOG_PERROR, LOG_DAEMON);

	if (on_exit(_ledmon_status, &ignore))
		return STATUS_ONEXIT_ERROR;

	if (_cmdline_parse_non_daemonise(argc, argv) != STATUS_SUCCESS)
		return STATUS_CMDLINE_ERROR;

	if (geteuid() != 0) {
		fprintf(stderr, "Only root can run this application.\n");
		return STATUS_NOT_A_PRIVILEGED_USER;
	}

	status = _init_ledmon_conf();
	if (status != STATUS_SUCCESS)
		return status;

	status = ledmon_read_config(ledmon_conf_path);
	if (status != STATUS_SUCCESS)
		return status;

	if (_cmdline_parse(argc, argv) != STATUS_SUCCESS)
		return STATUS_CMDLINE_ERROR;

	ledmon_write_shared_conf();

	if (log_open(conf.log_path) != STATUS_SUCCESS)
		return STATUS_LOG_FILE_ERROR;

	free(shortopt);
	free(longopt);
	if (pidfile_check(progname, NULL) == 0) {
		log_warning("daemon is running...");
		return STATUS_LEDMON_RUNNING;
	}
	if (!foreground) {
		pid_t pid = fork();

		if (pid < 0) {
			log_debug("main(): fork() failed (errno=%d).", errno);
			exit(EXIT_FAILURE);
		}
		if (pid > 0) {
			ignore = 1; /* parent: don't print exit status */
			exit(EXIT_SUCCESS);
		}

		pid_t sid = setsid();

		if (sid < 0) {
			log_debug("main(): setsid() failed (errno=%d).", errno);
			exit(EXIT_FAILURE);
		}

		_close_parent_fds();

		int t = open("/dev/null", O_RDWR);
		UNUSED(dup(t));
		UNUSED(dup(t));
	}

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
	list_init(&ledmon_block_list, (item_free_t)block_device_fini);
	sysfs_init();
	log_info("monitor service has been started...");
	while (terminate == 0) {
		struct block_device *device;

		timestamp = time(NULL);
		sysfs_scan();
		_ledmon_execute();
		_ledmon_wait(conf.scan_interval);
		/* Invalidate each device in the list. Clear controller and host. */
		list_for_each(&ledmon_block_list, device)
			_invalidate_dev(device);
		sysfs_reset();
	}
	ledmon_remove_shared_conf();
	stop_udev_monitor();
	exit(EXIT_SUCCESS);
}
