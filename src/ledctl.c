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

#include <config.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <syslog.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "status.h"
#include "list.h"
#include "cntrl.h"
#include "sysfs.h"
#include "ibpi.h"
#include "block.h"
#include "utils.h"
#include "version.h"
#include "scsi.h"
#include "ahci.h"

/**
 * @brief An IBPI state structure.
 *
 * This structure connects an IBPI pattern and block devices. It is used by
 * _determine() function to figure the correct pattern out.
 */
struct ibpi_state {
	enum ibpi_pattern ibpi;
	void *block_list;
};

/**
 * @brief List of IBPI patterns.
 *
 * This variable holds a pointer to a list of IBPI patterns the user requested
 * to be visualized. Each element on the list is struct ibpi_state type. There's
 * only one instance of each IBPI pattern on the list (no duplicates).
 */
static void *ibpi_list = NULL;

/**
 * @brief IBPI pattern names.
 *
 * This is internal array holding names of IBPI pattern. Logging routines use
 * this entries to translate enumeration type values into the string.
 */
const char *ibpi_str[] = {
	[IBPI_PATTERN_UNKNOWN]        = "",
	[IBPI_PATTERN_NORMAL]         = "NORMAL",
	[IBPI_PATTERN_ONESHOT_NORMAL] = "",
	[IBPI_PATTERN_DEGRADED]       = "ICA",
	[IBPI_PATTERN_REBUILD]        = "REBUILD",
	[IBPI_PATTERN_REBUILD_P]      = "REBUILD",
	[IBPI_PATTERN_FAILED_ARRAY]   = "IFA",
	[IBPI_PATTERN_HOTSPARE]       = "HOTSPARE",
	[IBPI_PATTERN_PFA]            = "PFA",
	[IBPI_PATTERN_FAILED_DRIVE]   = "FAILURE",
	[IBPI_PATTERN_LOCATE]         = "LOCATE",
	[IBPI_PATTERN_LOCATE_OFF]     = "LOCATE_OFF"
};

/**
 * Internal variable of ledctl utility. It is the pattern used to print out
 * information about the version of ledctl utility.
 */
static char *ledctl_version = "Intel(R) Enclosure LED Control Application %d.%d\n"
			      "Copyright (C) 2009-2013 Intel Corporation.\n";

/**
 * Internal variable of monitor service. It is used to help parse command line
 * short options.
 */
static char *shortopt = "c:hvl:";

/**
 * Internal enumeration type. It is used to help parse command line arguments.
 */
enum longopt {
	OPT_CONFIG,
	OPT_HELP,
	OPT_LOG,
	OPT_VERSION
};

/**
 * Internal array with option tokens. It is used to help parse command line
 * long options.
 */
static struct option longopt[] = {
	[OPT_CONFIG]  = {"config", required_argument, NULL, 'c'},
	[OPT_HELP]    = {"help", no_argument, NULL, 'h'},
	[OPT_LOG]     = {"log", required_argument, NULL, 'l'},
	[OPT_VERSION] = {"version", no_argument, NULL, 'v'},
			{NULL, no_argument, NULL, '\0'}
};

/**
 * @brief Finalizes LED control utility.
 *
 * This is internal function of ledctl utility. The function cleans up a memory
 * allocated for the application and closes all opened handles. This function is
 * design to be registered as on_exit() handler function.
 *
 * @param[in]      status         exit status of the ledctl application.
 * @param[in]      ignored        function ignores this argument.
 *
 * @return The function does not return a value.
 */
static void _ledctl_fini(int __attribute__ ((unused)) status,
			 void *__attribute__ ((unused)) ignore)
{
	sysfs_fini();
	if (ibpi_list)
		list_fini(ibpi_list);
	log_close();
}

/**
 * @brief Displays the credits.
 *
 * This is internal function of ledctl utility. It prints out the name and
 * version of the program. It displays the copyright notice and information
 * about the author and license, too.
 *
 * @return The function does not return a value.
 */
static void _ledctl_version(void)
{
	printf(ledctl_version, VERSION_MAJOR, VERSION_MINOR);
	printf("\nThis is free software; see the source for copying conditions." \
	       " There is NO warranty;\nnot even for MERCHANTABILITY or FITNESS" \
	       " FOR A PARTICULAR PURPOSE.\n\n");
}

/**
 * @brief Displays the help.
 *
 * This is internal function of ledctl utility. The function prints the name
 * and version of the program out. It displays the usage and available options
 * and its arguments (if any). Each option is described. This is an extract
 * from user manual page.
 *
 * @return The function does not return a value.
 */
static void _ledctl_help(void)
{
	printf(ledctl_version, VERSION_MAJOR, VERSION_MINOR);
	printf("\nUsage: %s [OPTIONS] pattern=list_of_devices ...\n\n",
	       progname);
	printf("Mandatory arguments for long options are mandatory for" \
	       " short options, too.\n\n");
	printf
	    ("--log=PATH,\t-l PATH\t\t  Use local log file instead\n" \
	     "\t\t\t\t  /var/log/ledctl.log global file.\n");
	printf("--config=PATH,\t-c PATH\t\t  Use alternate configuration" \
	       " file (not yet\n\t\t\t\t  implemented).\n");
	printf("--help,\t\t-h\t\t  Displays this help text.\n");
	printf
	    ("--version,\t-v\t\t  Displays version and license information.\n\n");
	printf("Patterns:\n"
	       "\tCommon patterns are:\n"
	       "\t\tlocate, locate_off, normal, off, degraded, rebuild,\n" ""
	       "\t\trebuild_p, failed_array, hotspare, pfa, failure,\n"
	       "\t\tdisk_failed\n" "\tSES-2 only patterns:\n"
	       "\t\tses_abort, ses_rebuild, ses_ifa, ses_ica, ses_cons_check,\n"
	       "\t\tses_hotspare, ses_rsvd_dev, ses_ok, ses_ident, ses_rm,\n"
	       "\t\tses_insert, ses_missing, ses_dnr, ses_active,\n"
	       "\t\tses_enbale_bb, ses_enable_ba, ses_devoff, ses_fault\n"
	       "\tAutomatic translation form IBPI into SES-2:\n"
	       "\t\tlocate=ses_ident, locate_off=~ses_ident,\n"
	       "\t\tnormal=ses_ok, off=ses_ok, degraded=ses_ica,\n"
	       "\t\trebuild=ses_rebuild rebuild_p=ses_rebuild,\n"
	       "\t\tfailed_array=ses_ifa, hotspare=ses_hotspare\n"
	       "\t\tpfa=ses_rsvd_dev, failure=ses_fault,\n"
	       "\t\tdisk_failed=ses_fault\n");
	printf("Refer to ledctl(8) man page for more detailed description.\n");
	printf("Bugs should be reported at: " \
	       "http://sourceforge.net/p/ledmon/bugs \n");
}

/**
 * @brief Puts new IBPI state on the list.
 *
 * This is internal function of ledctl utility. The function creates a new entry
 * of the list with IBPI patterns. Each IBPI state has a list of block devices
 * attached. The function initializes this list and sets empty.
 *
 * @param[in]      ibpi           an IBPI pattern to add.
 *
 * @return Pointer to the created element if successful, otherwise function
 *         returns NULL. The NULL pointer means either initialization
 *         of list for block devices failed or putting new element
 *         no IBPI state list failed.
 */
static void *_ibpi_state_init(enum ibpi_pattern ibpi)
{
	struct ibpi_state state;

	if (list_init(&state.block_list) != STATUS_SUCCESS)
		return NULL;
	state.ibpi = ibpi;
	return list_put(ibpi_list, &state, sizeof(struct ibpi_state));
}

/**
 * @brief Sets an IBPI state.
 *
 * This is internal function of ledctl utility. The function sets an IBPI state
 * for a block device. If existing pattern has higher priority then state
 * requested the function does nothing.
 *
 * @param[in]      block          pointer to block device structure.
 * @param[in]      ibpi           IBPI pattern/state to be set.
 *
 * @return The function does not return a value.
 */
static void _set_state(struct block_device **block, enum ibpi_pattern ibpi)
{
	if ((*block)->ibpi < ibpi)
		(*block)->ibpi = ibpi;
}

/**
 * @brief Sets a state of block device.
 *
 * This is internal function of ledctl utility. The function sets
 * an IBPI pattern for block devices. The function is design to be used
 * as action parameter of list_for_each() function.
 *
 * @param[in]      state          pointer to structure holding the IBPI pattern
 *                                identifier and list of block devices.
 *
 * @return The function does not return a value.
 */
static void _determine(struct ibpi_state *state)
{
	if (list_is_empty(state->block_list) == 0) {
		list_for_each_parm(state->block_list, _set_state, state->ibpi);
	} else {
		log_warning
		    ("IBPI %s: missing block device(s)... pattern ignored.",
		     ibpi_str[state->ibpi]);
	}
}

/**
 * @brief Determines a state of block devices.
 *
 * This is internal function of ledctl utility. The functions goes through list
 * of IBPI states and calls _determine() function for each element. If the list
 * is empty the function logs a warning message and does nothing.
 *
 * @param[in]      ibpi_list      pointer to list of IBPI states.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 *         The following status codes function returns:
 *
 *         STATUS_LIST_EMPTY      the specified list has no elements.
 *         STATUS_NULL_POINTER    ibpi_list is NULL.
 */
static status_t _ibpi_state_determine(void *ibpi_list)
{
	if (list_is_empty(ibpi_list) == 0)
		return list_for_each(ibpi_list, _determine);
	log_error("missing operand(s)... run %s --help for details.", progname);
	return STATUS_LIST_EMPTY;
}

/**
 * @brief Tests the IBPI state.
 *
 * This is internal function of ledctl utility. The function checks if the given
 * IBPI state is already on the list. The function is designed to be used as the
 * 'test' argument of list_first_that() function.
 *
 * @param[in]      state          pointer to IBPI state structure.
 * @param[in]      ibpi           IBPI state is being searched.
 *
 * @return 1 if the IBPI state matches, otherwise the function returns 0.
 */
static int _ibpi_find(struct ibpi_state *state, enum ibpi_pattern ibpi)
{
	return (state->ibpi == ibpi);
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
 * This is internal function of ledctl utility. This function sets the path and
 * file name of log file. The function checks if the specified path is valid. If
 * the path is invalid then the function does nothing.
 *
 * @param[in]      path           new location and name of log file.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
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
 * @brief Gets a pointer to IBPI state structure.
 *
 * This is internal function of ledctl utility. The function retrieves an entry
 * to an IBPI state structure from ibpi_list list. If such an entry does not
 * exist the memory is allocated and entry is added to the list.
 *
 * @param[in]      name       a name of IBPI pattern i.e. taken from command
 *                            line interface. It might be 'locate', 'normal',
 *                            'locate_off', 'off', 'ica', 'degraded', 'rebuild',
 *                            'rebuild_p', 'ifa', 'failed_array', 'hotspare',
 *                            'pfa', 'failure' or 'disk_failed' string.
 *
 * @return Pointer to IBPI state structure if successful, otherwise the function
 *         returns NULL. The NULL pointer means either the invalid status name
 *         has been given or there's not enough memory available in the system
 *         to allocate the structure.
 */
static struct ibpi_state *_ibpi_state_get(const char *name)
{
	struct ibpi_state *state = NULL;
	enum ibpi_pattern ibpi;

	if (strcmp(name, "locate") == 0) {
		ibpi = IBPI_PATTERN_LOCATE;
	} else if (strcmp(name, "locate_off") == 0) {
		ibpi = IBPI_PATTERN_LOCATE_OFF;
	} else if (strcmp(name, "normal") == 0) {
		ibpi = IBPI_PATTERN_NORMAL;
	} else if (strcmp(name, "off") == 0) {
		ibpi = IBPI_PATTERN_NORMAL;
	} else if ((strcmp(name, "ica") == 0) ||
		   (strcmp(name, "degraded") == 0)) {
		ibpi = IBPI_PATTERN_DEGRADED;
	} else if (strcmp(name, "rebuild") == 0) {
		ibpi = IBPI_PATTERN_REBUILD;
	} else if (strcmp(name, "rebuild_p") == 0) {
		ibpi = IBPI_PATTERN_REBUILD_P;
	} else if ((strcmp(name, "ifa") == 0) ||
		   (strcmp(name, "failed_array") == 0)) {
		ibpi = IBPI_PATTERN_FAILED_ARRAY;
	} else if (strcmp(name, "hotspare") == 0) {
		ibpi = IBPI_PATTERN_HOTSPARE;
	} else if (strcmp(name, "pfa") == 0) {
		ibpi = IBPI_PATTERN_PFA;
	} else if ((strcmp(name, "failure") == 0) ||
		   (strcmp(name, "disk_failed") == 0)) {
		ibpi = IBPI_PATTERN_FAILED_DRIVE;
	} else if (strcmp(name, "ses_abort") == 0) {
		ibpi = SES_REQ_ABORT;
	} else if (strcmp(name, "ses_rebuild") == 0) {
		ibpi = SES_REQ_REBUILD;
	} else if (strcmp(name, "ses_ifa") == 0) {
		ibpi = SES_REQ_IFA;
	} else if (strcmp(name, "ses_ica") == 0) {
		ibpi = SES_REQ_ICA;
	} else if (strcmp(name, "ses_cons_check") == 0) {
		ibpi = SES_REQ_CONS_CHECK;
	} else if (strcmp(name, "ses_hotspare") == 0) {
		ibpi = SES_REQ_HOSTSPARE;
	} else if (strcmp(name, "ses_rsvd_dev") == 0) {
		ibpi = SES_REQ_RSVD_DEV;
	} else if (strcmp(name, "ses_ok") == 0) {
		ibpi = SES_REQ_OK;
	} else if (strcmp(name, "ses_ident") == 0) {
		ibpi = SES_REQ_IDENT;
	} else if (strcmp(name, "ses_rm") == 0) {
		ibpi = SES_REQ_RM;
	} else if (strcmp(name, "ses_insert") == 0) {
		ibpi = SES_REQ_INS;
	} else if (strcmp(name, "ses_missing") == 0) {
		ibpi = SES_REQ_MISSING;
	} else if (strcmp(name, "ses_dnr") == 0) {
		ibpi = SES_REQ_DNR;
	} else if (strcmp(name, "ses_active") == 0) {
		ibpi = SES_REQ_ACTIVE;
	} else if (strcmp(name, "ses_enbale_bb") == 0) {
		ibpi = SES_REQ_EN_BB;
	} else if (strcmp(name, "ses_enable_ba") == 0) {
		ibpi = SES_REQ_EN_BA;
	} else if (strcmp(name, "ses_devoff") == 0) {
		ibpi = SES_REQ_DEV_OFF;
	} else if (strcmp(name, "ses_fault") == 0) {
		ibpi = SES_REQ_FAULT;
	} else {
		return NULL;
	}
	state = list_first_that(ibpi_list, _ibpi_find, ibpi);
	if (state == NULL)
		state = _ibpi_state_init(ibpi);
	return state;
}

/**
 * This is internal function of ledctl utility. The function checks if the
 * given block device is the one we looking for. This function is design
 * to be used as 'test' parameter for list_first_that() function.
 *
 * @param[in]      block          pointer to block device structure, the
 *                                element on block_list of sysfs structure.
 * @param[in]      path           path to block device in sysfs tree we are
 *                                looking for.
 *
 * @return 1 if the block device is found, otherwise the function returns 0.
 */
static int _block_device_search(struct block_device *block, const char *path)
{
	return (strcmp(block->sysfs_path, path) == 0);
}

/**
 * @brief Adds a block device to a block list.
 *
 * This is internal function of ledctl utility. Each IBPI state has list of
 * block devices attached to. The function puts a pointer to a block device
 * on that list. First the function determines the canonical version of the
 * given path and checks if it is correct. If the path to /dev directory is
 * given the function finds out the correct entry in sysfs tree.
 *
 * @param[in]      state          pointer to IBPI state structure the block
 *                                device will be added to.
 * @param[in]      block          pointer to block device structure.
 *
 * @return The function does not return a value.
 */
static status_t _ibpi_state_add_block(struct ibpi_state *state, char *name)
{
	struct stat st;
	char temp[PATH_MAX], path[PATH_MAX];
	void *blk1, *blk2;

	if ((realpath(name, temp) == NULL) && (errno != ENOTDIR))
		return STATUS_INVALID_PATH;
	if (strstr(temp, "/dev/") != NULL) {
		if (stat(temp, &st) < 0)
			return STATUS_STAT_ERROR;
		sprintf(temp, "/sys/dev/block/%d:%d", major(st.st_rdev),
			minor(st.st_rdev));
		if ((realpath(temp, path) == NULL) && (errno != ENOTDIR))
			return STATUS_INVALID_PATH;
	} else {
		str_cpy(path, temp, PATH_MAX);
	}
	blk1 = sysfs_block_device_first_that(_block_device_search, path);
	if (blk1 == NULL) {
		log_error("%s: device not supported", name);
		return STATUS_NOT_SUPPORTED;
	}
	blk2 = list_first_that(state->block_list, _block_device_search, path);
	if (blk2 == NULL) {
		if (list_put(state->block_list, &blk1, sizeof(void *)) == NULL)
			return STATUS_OUT_OF_MEMORY;
	} else {
		log_info("%s: %s: device already on the list.",
			 ibpi_str[state->ibpi], path);
	}
	return STATUS_SUCCESS;
}

/**
 * @brief Command line parser - operands.
 *
 * This is internal function of ledctl utility. The function parses operands of
 * ledctl application. The operands section contains the pattern name and a list
 * of block devices associated with each pattern. There are two different
 * formats for the operand. First format is pattern={ dev_list }, where elements
 * are space separated on the dev_list. Second format is pattern=dev1,dev2,...
 * where elements are comma separated on the list of devices.
 *
 * @param[in]      argc           number of elements in argv array.
 * @param[in]      argv           command line arguments.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
static status_t _cmdline_ibpi_parse(int argc, char *argv[])
{
	while (optind < argc) {
		struct ibpi_state *state = NULL;
		char *p = argv[optind++];
		char *t;
		t = strchrnul(p, '=');
		if (*t != '\0') {
			*(t++) = '\0';
			state = _ibpi_state_get(p);
			if (state != NULL) {
				if (*t == '{') {
					while ((t = argv[optind++]) != NULL) {
						if (*t == '}')
							break;
						_ibpi_state_add_block(state, t);
					}
				} else {
					while (*(p = t) != '\0') {
						t = strchrnul(p, ',');
						if (*t != '\0')
							*(t++) = '\0';
						_ibpi_state_add_block(state, p);
					}
				}
			}
		}
		if (state == NULL) {
			log_error("%s - unknown pattern name.", p);
			exit(STATUS_INVALID_STATE);
		}
	}
	return STATUS_SUCCESS;
}

/**
 * @brief Command line parser - options.
 *
 * This is internal function of ledctl utility. The function parses options of
 * ledctl application. Refer to ledctl help in order to get more information
 * about ledctl command line options.
 *
 * @param[in]      argc           number of elements in argv array.
 * @param[in]      argv           command line arguments.
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
		case 'v':
			_ledctl_version();
			exit(EXIT_SUCCESS);
		case 'h':
			_ledctl_help();
			exit(EXIT_SUCCESS);
		case 'c':
			status = _set_config_path(optarg);
			break;
		case 'l':
			status = _set_log_path(optarg);
			break;
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
 * @brief Sends a LED control message.
 *
 * This is internal function of ledctl utility. The function sends LED control
 * message to controller in order to control LED in enclosure. The pattern to
 * send is stored in block device structure.
 *
 * @param[in]      device         a pointer to block device structure.
 *
 * @return The function does not return a value.
 */
static void _send_cntrl_message(struct block_device *device)
{
	/* turn off all unset LEDs */
	if (device->ibpi == IBPI_PATTERN_UNKNOWN)
		device->ibpi = IBPI_PATTERN_NORMAL;
	device->send_fn(device, device->ibpi);
	device->flush_fn(device);
}

/**
 * @brief Determine and send IBPI pattern.
 *
 * This is internal function of ledctl utility. The function determines a state
 * of block device based on ibpi_list list. Then it sends a LED control message
 * to controller to visualize the pattern.
 *
 * @param[in]      sysfs          pointer to sysfs structure holding information
 *                                about the existing controllers, block devices,
 *                                and software RAID devices.
 * @param[in]      ibpi_list      TBD
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
static status_t _ledctl_execute(void *ibpi_list)
{
	if (_ibpi_state_determine(ibpi_list) != STATUS_SUCCESS)
		return STATUS_IBPI_DETERMINE_ERROR;
	return sysfs_block_device_for_each(_send_cntrl_message);
}

/**
 * @brief Application's entry point.
 *
 * This is the entry point of ledctl utility. This function does all the work.
 * It allocates and initializes all used structures. Registers on_exit()
 * handlers.
 * Then the function parses command line options and commands given and scans
 * sysfs tree for controllers, block devices and RAID devices. If no error is
 * the function send LED control messages according to IBPI pattern set.
 *
 * @param[in]      argc           number of elements in argv array, number of
 *                                command line arguments.
 * @param[in]      argv           array of command line arguments. The last
 *                                element on the list is NULL pointer.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
int main(int argc, char *argv[])
{
	status_t status;

	set_invocation_name(argv[0]);
	openlog(progname, LOG_PERROR, LOG_USER);

	if (getuid() != 0) {
		log_error("Only root can run this application.");
		return STATUS_NOT_A_PRIVILEGED_USER;
	}
	if (on_exit(_ledctl_fini, progname))
		exit(STATUS_ONEXIT_ERROR);
	if (_cmdline_parse(argc, argv))
		exit(STATUS_CMDLINE_ERROR);
	status = list_init(&ibpi_list);
	if (status != STATUS_SUCCESS) {
		log_debug("main(): list_init() failed (status=%s).",
			  strstatus(status));
		exit(STATUS_LIST_INIT_ERROR);
	}
	status = sysfs_init();
	if (status != STATUS_SUCCESS) {
		log_debug("main(): sysfs_init() failed (status=%s).",
			  strstatus(status));
		exit(STATUS_SYSFS_INIT_ERROR);
	}
	status = sysfs_scan();
	if (status != STATUS_SUCCESS) {
		log_debug("main(): sysfs_scan() failed (status=%s).",
			  strstatus(status));
		exit(STATUS_SYSFS_SCAN_ERROR);
	}
	status = _cmdline_ibpi_parse(argc, argv);
	if (status != STATUS_SUCCESS) {
		log_debug("main(): _ibpi_parse() failed (status=%s).",
			  strstatus(status));
		exit(STATUS_INVALID_STATE);
	}
	return _ledctl_execute(ibpi_list);
}
