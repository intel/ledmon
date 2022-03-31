/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2022 Intel Corporation.
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
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <sys/sysmacros.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "ahci.h"
#include "block.h"
#include "cntrl.h"
#include "config.h"
#include "config_file.h"
#include "slot.h"
#include "list.h"
#include "npem.h"
#include "pci_slot.h"
#include "scsi.h"
#include "status.h"
#include "sysfs.h"

/**
 * @brief An IBPI state structure.
 *
 * This structure connects an IBPI pattern and block devices. It is used by
 * _determine() function to figure the correct pattern out.
 */
struct ibpi_state {
	enum ibpi_pattern ibpi;
	struct list block_list;
};

/**
 * @brief List of IBPI patterns.
 *
 * This is a list of IBPI patterns the user requested to be visualized.
 * Each element on the list is struct ibpi_state type. There's only one
 * instance of each IBPI pattern on the list (no duplicates).
 */
static struct list ibpi_list;

/**
 * @brief Pointer to a get slot function.
 *
 * The pointer to a function which will print slot details.
 *
 * @param[in]         device        Name of the device.
 * @param[in]         slot          Unique identifier of the slot.
 * @param[in]         res           Pointer to slot response.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
typedef status_t (*get_slot_t) (char *device, char *slot, struct slot_response *res);

/**
 * @brief Pointer to a set slot function.
 *
 * The pointer to a function which will set slot details.
 *
 * @param[in]         device        Name of the device.
 * @param[in]         slot          Unique identifier of the slot.
 * @param[in]         state         IBPI state based on slot request.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
typedef status_t (*set_slot_t) (char *device, char *slot, enum ibpi_pattern state);

/**
 * @brief slot request parametres
 *
 * This structure contains all possible parameters for slot related commands.
 */
struct slot_request {
	/**
	 * Option given in the request.
	 */
	int chosen_opt;

	/**
	 * Name of the device.
	 */
	char device[PATH_MAX];

	/**
	 * Unique slot identifier.
	 */
	char slot[PATH_MAX];

	/**
	 * Type of the controller.
	 */
	char ctrl_type[BUFFER_MAX];

	/**
	 * IBPI state.
	 */
	enum ibpi_pattern state;

	/**
	 * Pointer to the get slot function.
	 */
	get_slot_t get_slot_fn;

	/**
	 * Pointer to the set slot function.
	 */
	set_slot_t set_slot_fn;
};

/**
 * @brief IBPI pattern names.
 *
 * This is internal array holding names of IBPI pattern. Logging routines use
 * this entries to translate enumeration type values into the string.
 */
const char *ibpi_str[] = {
	[IBPI_PATTERN_UNKNOWN]        = "UNKNOWN",
	[IBPI_PATTERN_NORMAL]         = "NORMAL",
	[IBPI_PATTERN_ONESHOT_NORMAL] = "",
	[IBPI_PATTERN_DEGRADED]       = "ICA",
	[IBPI_PATTERN_REBUILD]        = "REBUILD",
	[IBPI_PATTERN_FAILED_ARRAY]   = "IFA",
	[IBPI_PATTERN_HOTSPARE]       = "HOTSPARE",
	[IBPI_PATTERN_PFA]            = "PFA",
	[IBPI_PATTERN_FAILED_DRIVE]   = "FAILURE",
	[IBPI_PATTERN_LOCATE]         = "LOCATE",
	[IBPI_PATTERN_LOCATE_OFF]     = "LOCATE_OFF",
	[IBPI_PATTERN_ADDED]          = "ADDED",
	[IBPI_PATTERN_REMOVED]        = "REMOVED"
};

/**
 * Internal variable of ledctl utility. It is the pattern used to print out
 * information about the version of ledctl utility.
 */
static char *ledctl_version = "Intel(R) Enclosure LED Control Application %s %s\n"
			      "Copyright (C) 2009-2022 Intel Corporation.\n";

/**
 * Internal variable of monitor service. It is used to help parse command line
 * short options.
 */
static char *shortopt;

struct option *longopt;

static int possible_params[] = {
	OPT_HELP,
	OPT_LOG,
	OPT_VERSION,
	OPT_LIST_CTRL,
	OPT_LISTED_ONLY,
	OPT_LIST_SLOTS,
	OPT_GET_SLOT,
	OPT_SET_SLOT,
	OPT_CONTROLLER,
	OPT_DEVICE,
	OPT_SLOT,
	OPT_STATE,
	OPT_ALL,
	OPT_DEBUG,
	OPT_ERROR,
	OPT_INFO,
	OPT_QUIET,
	OPT_WARNING,
	OPT_LOG_LEVEL,
};

static const int possible_params_size = sizeof(possible_params)
		/ sizeof(possible_params[0]);

static int listed_only;

/**
 * @brief Determines a get slot function.
 *
 * This function determines get slot function based on
 * controller type.
 *
 * @param[in]       ctrl_type       Controller type.
 *
 * @return Pointer to get slot function if successful, otherwise
 *         the function returns NULL pointer.
 */
static get_slot_t _get_slot_ctrl_fn(const char *ctrl_type)
{
	if (strcasecmp(ctrl_type, "vmd") == 0) {
		return pci_get_slot;
	} else if (strcasecmp(ctrl_type, "npem") == 0) {
		return npem_get_slot;
	}
	log_debug("The controller type %s does not support slots managing.", ctrl_type);

	return NULL;
}

/**
 * @brief Determines a set slot function.
 *
 * This function determines set slot function based on
 * controller type.
 *
 * @param[in]       ctrl_type       Controller type.
 *
 * @return Pointer to set slot function if successful, otherwise
 *         the function returns NULL pointer.
 */
static set_slot_t _set_slot_ctrl_fn(const char *ctrl_type)
{
	if (strcasecmp(ctrl_type, "vmd") == 0)
		return pci_set_slot;
	log_error("The controller type %s does not support slots managing.", ctrl_type);

	return NULL;
}

static void ibpi_state_fini(struct ibpi_state *p)
{
	list_clear(&p->block_list);
	free(p);
}

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
static void _ledctl_fini(int status __attribute__ ((unused)),
			 void *ignore __attribute__ ((unused)))
{
	sysfs_reset();
	list_erase(&ibpi_list);
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
	printf(ledctl_version, PACKAGE_VERSION, BUILD_LABEL);
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
	printf(ledctl_version, PACKAGE_VERSION, BUILD_LABEL);
	printf("\nUsage: %s [OPTIONS] pattern=list_of_devices ...\n\n",
	       progname);
	printf("Mandatory arguments for long options are mandatory for short options, too.\n\n");
	print_opt("--listed-only", "-x",
		  "Ledctl will change state only for given devices.");
	print_opt("--list-controllers", "-L",
		  "Displays list of controllers detected by ledmon.");
	print_opt("--list-slots --controller CONTROLLER", "-P -c CONTROLLER",
		  "List slots under the controller, their led states, slot numbers and "
		  "devnodes connected.");
	print_opt("--get-slot --controller CONTROLLER --device DEVNODE / --slot SLOT",
		  "-G -c CONTROLLER -d DEVNODE / -p SLOT",
		  "Prints slot information, its led state, slot number and devnode.");
	print_opt("--set-slot --controller CONTROLLER --slot SLOT --state STATE",
		  "-S -c CONTROLLER -p SLOT -s STATE", "Sets given state for chosen slot "
		  "under the controller.");
	print_opt("--log=PATH", "-l PATH",
		  "Use local log file instead /var/log/ledctl.log.");
	print_opt("--help", "-h", "Displays this help text.");
	print_opt("--version", "-v",
		  "Displays version and license information.");
	print_opt("--log-level=VALUE", "-l VALUE",
		  "Allows user to set ledctl verbose level in logs.");
	printf("\nPatterns:\n"
	       "\tCommon patterns are:\n"
	       "\t\tlocate, locate_off, normal, off, degraded, rebuild,\n" ""
	       "\t\tfailed_array, hotspare, pfa, failure, disk_failed\n"
	       "\tSES-2 only patterns:\n"
	       "\t\tses_abort, ses_rebuild, ses_ifa, ses_ica, ses_cons_check,\n"
	       "\t\tses_hotspare, ses_rsvd_dev, ses_ok, ses_ident, ses_rm,\n"
	       "\t\tses_insert, ses_missing, ses_dnr, ses_active, ses_prdfail,\n"
	       "\t\tses_enable_bb, ses_enable_ba, ses_devoff, ses_fault\n"
	       "\tAutomatic translation form IBPI into SES-2:\n"
	       "\t\tlocate=ses_ident, locate_off=~ses_ident,\n"
	       "\t\tnormal=ses_ok, off=ses_ok, degraded=ses_ica,\n"
	       "\t\trebuild=ses_rebuild, failed_array=ses_ifa,\n"
	       "\t\thotspare=ses_hotspare, pfa=ses_prdfail, failure=ses_fault,\n"
	       "\t\tdisk_failed=ses_fault\n");
	printf("Refer to ledctl(8) man page for more detailed description.\n");
	printf("Bugs should be reported at: " \
		"https://github.com/intel/ledmon/issues\n");
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
 *         returns NULL. The NULL pointer means element allocation failed.
 */
static struct ibpi_state *_ibpi_state_init(enum ibpi_pattern ibpi)
{
	struct ibpi_state *state = malloc(sizeof(struct ibpi_state));

	if (!state)
		return NULL;

	list_init(&state->block_list, NULL);
	state->ibpi = ibpi;

	list_append(&ibpi_list, state);

	return state;
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
	if (list_is_empty(&state->block_list) == 0) {
		struct block_device *block;

		list_for_each(&state->block_list, block) {
			if (block->ibpi != state->ibpi)
				block->ibpi = state->ibpi;
		}
	} else {
		log_warning
		    ("IBPI %s: missing block device(s)... pattern ignored.",
		     ibpi2str(state->ibpi));
	}
}

/**
 * @brief Determines a state of block devices.
 *
 * This is internal function of ledctl utility. The functions goes through list
 * of IBPI states and calls _determine() function for each element. If the list
 * is empty the function logs a warning message and does nothing.
 *
 * @param[in]      ibpi_local_list  pointer to list of IBPI states.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 *         The following status codes function returns:
 *
 *         STATUS_LIST_EMPTY      the specified list has no elements.
 */
static status_t _ibpi_state_determine(struct list *ibpi_local_list)
{
	if (list_is_empty(ibpi_local_list) == 0) {
		struct ibpi_state *state;

		list_for_each(ibpi_local_list, state)
			_determine(state);
		return STATUS_SUCCESS;
	}
	log_error("missing operand(s)... run %s --help for details.", progname);
	return STATUS_LIST_EMPTY;
}

static struct ibpi_state *_ibpi_find(const struct list *ibpi_local_list,
				     enum ibpi_pattern ibpi)
{
	struct ibpi_state *state;

	list_for_each(ibpi_local_list, state) {
		if (state->ibpi == ibpi)
			return state;
	}
	return NULL;
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
 *                            'ifa', 'failed_array', 'hotspare',
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
	} else if (strcmp(name, "ses_enable_bb") == 0) {
		ibpi = SES_REQ_EN_BB;
	} else if (strcmp(name, "ses_enable_ba") == 0) {
		ibpi = SES_REQ_EN_BA;
	} else if (strcmp(name, "ses_devoff") == 0) {
		ibpi = SES_REQ_DEV_OFF;
	} else if (strcmp(name, "ses_fault") == 0) {
		ibpi = SES_REQ_FAULT;
	} else if (strcmp(name, "ses_prdfail") == 0) {
		ibpi = SES_REQ_PRDFAIL;
	} else {
		return NULL;
	}
	state = _ibpi_find(&ibpi_list, ibpi);
	if (state == NULL)
		state = _ibpi_state_init(ibpi);
	return state;
}

static struct block_device *_block_device_search(const struct list *block_list,
						 const char *path)
{
	struct block_device *block;

	list_for_each(block_list, block) {
		if (strcmp(block->sysfs_path, path) == 0)
			return block;
	}
	return NULL;
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
 * @param[in]      name           path to block device.
 *
 * @return The function does not return a value.
 */
static status_t _ibpi_state_add_block(struct ibpi_state *state, char *name)
{
	struct stat st;
	char temp[PATH_MAX], path[PATH_MAX];
	struct block_device *blk1, *blk2;

	if ((realpath(name, temp) == NULL) && (errno != ENOTDIR))
		return STATUS_INVALID_PATH;
	if (strstr(temp, "/dev/") != NULL) {
		if (stat(temp, &st) < 0)
			return STATUS_STAT_ERROR;
		snprintf(temp, PATH_MAX, "/sys/dev/block/%u:%u", major(st.st_rdev),
			minor(st.st_rdev));
		if ((realpath(temp, path) == NULL) && (errno != ENOTDIR))
			return STATUS_INVALID_PATH;
	} else {
		str_cpy(path, temp, PATH_MAX);
	}
	blk1 = _block_device_search(sysfs_get_block_devices(), path);
	if (blk1 == NULL) {
		log_error("%s: device not supported", name);
		return STATUS_NOT_SUPPORTED;
	}
	blk2 = _block_device_search(&state->block_list, path);
	if (blk2 == NULL)
		list_append(&state->block_list, blk1);
	else
		log_info("%s: %s: device already on the list.",
			 ibpi2str(state->ibpi), path);
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
	status_t t_status, ret_status = STATUS_SUCCESS;

	while (optind < argc) {
		struct ibpi_state *state = NULL;
		char *p = argv[optind++];
		char *t;
		t = strchrnul(p, '=');
		if (*t != '\0') {
			*(t++) = '\0';
			state = _ibpi_state_get(p);
			if (state == NULL) {
				log_error("%s - unknown pattern name.", p);
				return STATUS_INVALID_STATE;
			}
			if (*t == '{') {
				while ((t = argv[optind++]) != NULL) {
					if (*t == '}')
						break;
					t_status =
					_ibpi_state_add_block(state, t);
					if (t_status != STATUS_SUCCESS)
						ret_status = t_status;
				}
			} else {
				while (*(p = t) != '\0') {
					t = strchrnul(p, ',');
					if (*t != '\0')
						*(t++) = '\0';
					t_status =
					_ibpi_state_add_block(state, p);
					if (t_status != STATUS_SUCCESS)
						ret_status = t_status;
				}
			}
		}
	}
	if (_ibpi_state_determine(&ibpi_list) != STATUS_SUCCESS)
		ret_status = STATUS_IBPI_DETERMINE_ERROR;
	return ret_status;
}

/**
 * @brief Command line parser - checks if command line input contains
 * options which don't require to run ledctl as root.
 *
 * The function parses options of ledctl application.
 * It handles option to print version and help.
 *
 * @param[in]      argc           number of elements in argv array.
 * @param[in]      argv           command line arguments.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
static status_t _cmdline_parse_non_root(int argc, char *argv[])
{
	int opt_index, opt = -1;
	status_t status = STATUS_SUCCESS;

	do {
		opt = getopt_long(argc, argv, shortopt, longopt, &opt_index);
		switch (opt) {
		case 'v':
			_ledctl_version();
			exit(EXIT_SUCCESS);
		case 'h':
			_ledctl_help();
			exit(EXIT_SUCCESS);
		case ':':
		case '?':
			return STATUS_CMDLINE_ERROR;
		}
	} while (opt >= 0);

	return status;
}

/**
 * @brief Inits slot request structure with initial values.
 *
 * @param[in]       slot_req       structure with slot request
 *
 * @return This function does not return a value.
 */
static void slot_request_init(struct slot_request *slot_req)
{
	memset(slot_req, 0, sizeof(struct slot_request));

	slot_req->chosen_opt = OPT_NULL_ELEMENT;
	slot_req->state = IBPI_PATTERN_UNKNOWN;
}

/**
 * @brief Inits slot response structure with initial values.
 *
 * @param[in]       slot_res       structure with slot response
 *
 * @return This function does not return a value.
 */
static void slot_response_init(struct slot_response *slot_res)
{
	memset(slot_res, 0, sizeof(struct slot_response));

	slot_res->state = IBPI_PATTERN_UNKNOWN;
}

/**
 * @brief Verifies slot request parameters.
 *
 * @param[in]       slot_req       structure with slot request
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
static status_t slot_verify_request(struct slot_request *slot_req)
{
	if (!slot_req->ctrl_type || slot_req->ctrl_type[0] == '\0') {
		log_error("Invalid controller in the request.");
		return STATUS_INVALID_CONTROLLER;
	}
	if (slot_req->chosen_opt == OPT_SET_SLOT && slot_req->state == IBPI_PATTERN_UNKNOWN) {
		log_error("Invalid IBPI state in the request.");
		return STATUS_INVALID_STATE;
	}
	if ((slot_req->chosen_opt == OPT_GET_SLOT || slot_req->chosen_opt == OPT_LIST_SLOTS)
	     && !slot_req->get_slot_fn) {
		log_error("The controller type %s does not support slots managing.",
			  slot_req->ctrl_type);
		return STATUS_DATA_ERROR;
	}
	if (slot_req->chosen_opt == OPT_SET_SLOT && !slot_req->set_slot_fn) {
		log_debug("The controller type %s doesn't support set slot functionality.",
			  slot_req->ctrl_type);
		return STATUS_DATA_ERROR;
	}

	return STATUS_SUCCESS;
}

/**
 * @brief List slots connected to given controller
 *
 * This function scans all available slots connected to given controller
 * and prints their led states and names of the connected devices (if exist).
 *
 * @param[in]       slot_req       Structure with slot request.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
static status_t list_slots(struct slot_request *slot_req)
{
	struct pci_slot *slot;
	status_t status = STATUS_SUCCESS;

	if (strcasecmp(slot_req->ctrl_type, "vmd") == 0) {
		list_for_each(sysfs_get_pci_slots(), slot) {
			struct slot_response slot_res;

			slot_response_init(&slot_res);
			char *slot_num = pci_get_slot_number_from_path(slot->sysfs_path);

			if (slot_num) {
				status = slot_req->get_slot_fn(NULL, slot_num, &slot_res);
				if (status == STATUS_SUCCESS)
					print_slot_state(&slot_res);
				else
					return status;
			} else {
				return STATUS_NULL_POINTER;
			}
		}
		return status;
	}

	log_debug("The controller type %s does not support slots managing.", slot_req->ctrl_type);
	return STATUS_NOT_SUPPORTED;
}

/**
 * @brief Executes proper slot mode function.
 *
 * @param[in]       slot_req       Structure with slot request.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t slot_execute(struct slot_request *slot_req)
{
	struct slot_response slot_res;
	status_t status = STATUS_SUCCESS;

	slot_response_init(&slot_res);

	switch (slot_req->chosen_opt) {
	case OPT_LIST_SLOTS:
		return list_slots(slot_req);
	case OPT_SET_SLOT:
		status = slot_req->set_slot_fn(slot_req->device, slot_req->slot, slot_req->state);
		if (status != STATUS_SUCCESS)
			return status;
	case OPT_GET_SLOT:
		status = slot_req->get_slot_fn(slot_req->device, slot_req->slot, &slot_res);
		if (status == STATUS_SUCCESS)
			print_slot_state(&slot_res);
		return status;
	default:
		return STATUS_NOT_SUPPORTED;
	}
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
static status_t _cmdline_parse(int argc, char *argv[], struct slot_request *req)
{
	int opt, opt_index = -1;
	status_t status = STATUS_SUCCESS;

	optind = 1;

	do {
		opt = getopt_long(argc, argv, shortopt, longopt, &opt_index);
		if (opt == -1)
			break;
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
			default:
				status = set_verbose_level(
						possible_params[opt_index]);

			}
			break;
		case 'l':
			status = set_log_path(optarg);
			break;
		case 'x':
			status = STATUS_SUCCESS;
			listed_only = 1;
			break;
		case 'L':
		{
			struct cntrl_device *ctrl_dev;

			sysfs_init();
			sysfs_scan();
			list_for_each(sysfs_get_cntrl_devices(), ctrl_dev)
				print_cntrl(ctrl_dev);
			sysfs_reset();
			exit(EXIT_SUCCESS);
		}
		case 'G':
			req->chosen_opt = OPT_GET_SLOT;
			break;
		case 'P':
			req->chosen_opt = OPT_LIST_SLOTS;
			break;
		case 'S':
			req->chosen_opt = OPT_SET_SLOT;
			break;
		case 'c':
			strncpy(req->ctrl_type, optarg, BUFFER_MAX - 1);
			req->get_slot_fn = _get_slot_ctrl_fn(req->ctrl_type);
			req->set_slot_fn = _set_slot_ctrl_fn(req->ctrl_type);
			break;
		case 's':
		{
			struct ibpi_state *state = _ibpi_state_get(optarg);

			if (state)
				req->state = state->ibpi;
			free(state);
			break;
		}
		case 'd':
			strncpy(req->device, optarg, PATH_MAX - 1);
			break;
		case 'p':
			strncpy(req->slot, optarg, PATH_MAX - 1);
			break;
		case ':':
		case '?':
		default:
			log_debug("[opt='%c', opt_index=%d]", opt, opt_index);
			return STATUS_CMDLINE_ERROR;
		}
		opt_index = -1;
		if (status != STATUS_SUCCESS)
			return status;
	} while (1);

	return STATUS_SUCCESS;
}

/**
 * @brief Send IBPI pattern.
 *
 * This is internal function of ledctl utility. The function set a requested
 * ibpi_state for devices linked with this ibpi_state on ibpi_local_list.
 * For other devices IBPI_PATTERN_LOCATE_OFF might be set - depending on
 * listed_only parameter. Then it sends a LED control message to controller
 * to visualize the pattern.
 *
 * @param[in]      ibpi_local_list	    pointer to list of ipbi_state.
 *
 * @return STATUS_SUCCESS if successful, otherwise STATUS_IBPI_DETERMINE_ERROR
 */
static status_t _ledctl_execute(struct list *ibpi_local_list)
{
	struct ibpi_state *state;
	struct block_device *device;

	if (!listed_only) {
		list_for_each(sysfs_get_block_devices(), device)
			device->send_fn(device, IBPI_PATTERN_LOCATE_OFF);
	}

	list_for_each(ibpi_local_list, state)
		list_for_each(&state->block_list, device) {
			if (state->ibpi != device->ibpi) {
				log_debug("Mismatch detected for %s, ibpi state: %s, device state %s\n",
					  device->sysfs_path, state->ibpi,
					  device->ibpi);
				return STATUS_IBPI_DETERMINE_ERROR;
			}
			device->send_fn(device, device->ibpi);
		}

	list_for_each(sysfs_get_block_devices(), device)
		device->flush_fn(device);

	return STATUS_SUCCESS;
}

static status_t _read_shared_conf(void)
{
	status_t status;
	char share_conf_path[PATH_MAX];

	memset(share_conf_path, 0, sizeof(share_conf_path));
	snprintf(share_conf_path, sizeof(share_conf_path), "/dev/shm%s",
		 LEDMON_SHARE_MEM_FILE);

	status = ledmon_read_config(share_conf_path);
	return status;
}

static status_t _init_ledctl_conf(void)
{
	memset(&conf, 0, sizeof(struct ledmon_conf));
	/* initialize with default values */
	conf.log_level = LOG_LEVEL_WARNING;
	list_init(&conf.cntrls_whitelist, NULL);
	list_init(&conf.cntrls_blacklist, NULL);

	return set_log_path(LEDCTL_DEF_LOG_FILE);
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
	struct slot_request slot_req;

	setup_options(&longopt, &shortopt, possible_params,
			possible_params_size);
	set_invocation_name(argv[0]);

	if (_cmdline_parse_non_root(argc, argv) != STATUS_SUCCESS)
		return STATUS_CMDLINE_ERROR;

	openlog(progname, LOG_PERROR, LOG_USER);

	if (geteuid() != 0) {
		fprintf(stderr, "Only root can run this application.\n");
		return STATUS_NOT_A_PRIVILEGED_USER;
	}

	status = _init_ledctl_conf();
	if (status != STATUS_SUCCESS)
		return status;
	if (on_exit(_ledctl_fini, progname))
		exit(STATUS_ONEXIT_ERROR);
	slot_request_init(&slot_req);
	status = _cmdline_parse(argc, argv, &slot_req);
	if (status != STATUS_SUCCESS)
		exit(STATUS_CMDLINE_ERROR);
	free(shortopt);
	free(longopt);
	status = _read_shared_conf();
	if (status != STATUS_SUCCESS)
		return status;
	status = log_open(conf.log_path);
	if (status != STATUS_SUCCESS)
		return STATUS_LOG_FILE_ERROR;

	list_init(&ibpi_list, (item_free_t)ibpi_state_fini);
	sysfs_init();
	sysfs_scan();
	if (slot_req.chosen_opt != OPT_NULL_ELEMENT) {
		status = slot_verify_request(&slot_req);
		if (status == STATUS_SUCCESS)
			return slot_execute(&slot_req);
		else
			exit(status);
	}
	status = _cmdline_ibpi_parse(argc, argv);
	if (status != STATUS_SUCCESS) {
		log_debug("main(): _ibpi_parse() failed (status=%s).",
			  strstatus(status));
		exit(status);
	}
	return _ledctl_execute(&ibpi_list);
}
