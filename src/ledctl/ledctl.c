/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2023 Intel Corporation.
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
#include <linux/limits.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
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
#include <stdbool.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "config.h"
#include <common/config_file.h>
#include <lib/utils.h>
#include <lib/list.h>
#include <led/libled.h>
#include "libled_internal.h"
#include "slot.h"

typedef enum {
	LEDCTL_STATUS_SUCCESS=0,
	LEDCTL_STATUS_NULL_POINTER=2,
	LEDCTL_STATUS_DATA_ERROR=6,
	LEDCTL_STATUS_IBPI_DETERMINE_ERROR=7,
	LEDCTL_STATUS_INVALID_PATH=8,
	LEDCTL_STATUS_INVALID_STATE=10,
	LEDCTL_STATUS_FILE_OPEN_ERROR=12,
	LEDCTL_STATUS_FILE_WRITE_ERROR=14,
	LEDCTL_STATUS_LIST_EMPTY=21,
	LEDCTL_STATUS_ONEXIT_ERROR=31,
	LEDCTL_STATUS_INVALID_CONTROLLER=32,
	LEDCTL_STATUS_NOT_SUPPORTED=33,
	LEDCTL_STATUS_STAT_ERROR=34,
	LEDCTL_STATUS_CMDLINE_ERROR=35,
	LEDCTL_STATUS_NOT_A_PRIVILEGED_USER=36,
	LEDCTL_STATUS_CONFIG_FILE_ERROR=39,
	LEDCTL_STATUS_LOG_FILE_ERROR=40,
	LEDCTL_STATUS_UNDEFINED
} ledctl_status_code_t;

struct map ledctl_status_map[] = {
	{ "STATUS_SUCCESSS", LEDCTL_STATUS_SUCCESS },
	{ "STATUS_IBPI_DETERMINE_ERROR", LEDCTL_STATUS_IBPI_DETERMINE_ERROR },
	{ "STATUS_INVALID_PATH", LEDCTL_STATUS_INVALID_PATH },
	{ "STATUS_INVALID_STATE", LEDCTL_STATUS_INVALID_STATE },
	{ "STATUS_LIST_EMPTY", LEDCTL_STATUS_LIST_EMPTY },
	{ "STATUS_ONEXIT_ERROR", LEDCTL_STATUS_ONEXIT_ERROR },
	{ "STATUS_NOT_SUPPORTED", LEDCTL_STATUS_NOT_SUPPORTED },
	{ "STATUS_STAT_ERROR", LEDCTL_STATUS_STAT_ERROR },
	{ "STATUS_CMDLINE_ERROR", LEDCTL_STATUS_CMDLINE_ERROR },
	{ "STATUS_NOT_A_PRIVILEGED_USER", LEDCTL_STATUS_NOT_A_PRIVILEGED_USER },
	{ "STATUS_LOG_FILE_ERROR", LEDCTL_STATUS_LOG_FILE_ERROR },
	{ NULL, LEDCTL_STATUS_UNDEFINED}
};

/**
 * @brief Get string status for ledctl.
 *
 * @param	s	ledctl status code.
 * @return	string status if defined, else "???".
 */
static char *ledctl_strstatus(ledctl_status_code_t s)
{
	char *status_str = str_map(s, ledctl_status_map);

	return status_str ? status_str : "???";
}

/**
 * @brief An IBPI state structure.
 *
 * This structure connects an IBPI pattern and block devices. It is used by
 * _determine() function to figure the correct pattern out.
 */
struct ibpi_state {
	enum led_ibpi_pattern ibpi;
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
	enum led_cntrl_type cntrl;

	/**
	 * IBPI state.
	 */
	enum led_ibpi_pattern state;
};

/**
 * Internal variable of ledctl utility. It is the pattern used to print out
 * information about the version of ledctl utility.
 */
static char *ledctl_version = "Intel(R) Enclosure LED Control Application %s %s\n"
			      "Copyright (C) 2009-2023 Intel Corporation.\n";

/**
 * Internal variable of monitor service. It is used to help parse command line
 * short options.
 */
static char *shortopt;

struct option *longopt;

static struct led_ctx *ctx;

struct ledmon_conf conf;

static int possible_params[] = {
	OPT_HELP,
	OPT_LOG,
	OPT_VERSION,
	OPT_LIST_CTRL,
	OPT_LISTED_ONLY,
	OPT_LIST_SLOTS,
	OPT_GET_SLOT,
	OPT_SET_SLOT,
	OPT_CNTRL_TYPE,
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

static const int possible_params_size = ARRAY_SIZE(possible_params);
static int listed_only;

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
	led_free(ctx);
	list_erase(&ibpi_list);
	log_close(&conf);
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
	print_opt("--list-slots --controller-type CONTROLLER", "-P -c CONTROLLER",
		  "List slots under the controller type, their led states, slot numbers and "
		  "devnodes connected.");
	print_opt("--get-slot --controller-type CONTROLLER --device DEVNODE / --slot SLOT",
		  "-G -c CONTROLLER -d DEVNODE / -p SLOT",
		  "Prints slot information, its led state, slot number and devnode.");
	print_opt("--set-slot --controller-type CONTROLLER --slot SLOT --state STATE",
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
static struct ibpi_state *_ibpi_state_init(enum led_ibpi_pattern ibpi)
{
	struct ibpi_state *state = malloc(sizeof(struct ibpi_state));

	if (!state)
		return NULL;

	list_init(&state->block_list, NULL);
	state->ibpi = ibpi;

	if (!list_append(&ibpi_list, state)) {
		free(state);
		return NULL;
	}

	return state;
}

static struct ibpi_state *_ibpi_find(const struct list *ibpi_local_list,
				     enum led_ibpi_pattern ibpi)
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
	enum led_ibpi_pattern ibpi;

	if (strcmp(name, "locate") == 0) {
		ibpi = LED_IBPI_PATTERN_LOCATE;
	} else if (strcmp(name, "locate_off") == 0) {
		ibpi = LED_IBPI_PATTERN_LOCATE_OFF;
	} else if (strcmp(name, "normal") == 0) {
		ibpi = LED_IBPI_PATTERN_NORMAL;
	} else if (strcmp(name, "off") == 0) {
		ibpi = LED_IBPI_PATTERN_NORMAL;
	} else if ((strcmp(name, "ica") == 0) ||
		   (strcmp(name, "degraded") == 0)) {
		ibpi = LED_IBPI_PATTERN_DEGRADED;
	} else if (strcmp(name, "rebuild") == 0) {
		ibpi = LED_IBPI_PATTERN_REBUILD;
	} else if ((strcmp(name, "ifa") == 0) ||
		   (strcmp(name, "failed_array") == 0)) {
		ibpi = LED_IBPI_PATTERN_FAILED_ARRAY;
	} else if (strcmp(name, "hotspare") == 0) {
		ibpi = LED_IBPI_PATTERN_HOTSPARE;
	} else if (strcmp(name, "pfa") == 0) {
		ibpi = LED_IBPI_PATTERN_PFA;
	} else if ((strcmp(name, "failure") == 0) ||
		   (strcmp(name, "disk_failed") == 0)) {
		ibpi = LED_IBPI_PATTERN_FAILED_DRIVE;
	} else if (strcmp(name, "ses_abort") == 0) {
		ibpi = LED_SES_REQ_ABORT;
	} else if (strcmp(name, "ses_rebuild") == 0) {
		ibpi = LED_SES_REQ_REBUILD;
	} else if (strcmp(name, "ses_ifa") == 0) {
		ibpi = LED_SES_REQ_IFA;
	} else if (strcmp(name, "ses_ica") == 0) {
		ibpi = LED_SES_REQ_ICA;
	} else if (strcmp(name, "ses_cons_check") == 0) {
		ibpi = LED_SES_REQ_CONS_CHECK;
	} else if (strcmp(name, "ses_hotspare") == 0) {
		ibpi = LED_SES_REQ_HOSTSPARE;
	} else if (strcmp(name, "ses_rsvd_dev") == 0) {
		ibpi = LED_SES_REQ_RSVD_DEV;
	} else if (strcmp(name, "ses_ok") == 0) {
		ibpi = LED_SES_REQ_OK;
	} else if (strcmp(name, "ses_ident") == 0) {
		ibpi = LED_SES_REQ_IDENT;
	} else if (strcmp(name, "ses_rm") == 0) {
		ibpi = LED_SES_REQ_RM;
	} else if (strcmp(name, "ses_insert") == 0) {
		ibpi = LED_SES_REQ_INS;
	} else if (strcmp(name, "ses_missing") == 0) {
		ibpi = LED_SES_REQ_MISSING;
	} else if (strcmp(name, "ses_dnr") == 0) {
		ibpi = LED_SES_REQ_DNR;
	} else if (strcmp(name, "ses_active") == 0) {
		ibpi = LED_SES_REQ_ACTIVE;
	} else if (strcmp(name, "ses_enable_bb") == 0) {
		ibpi = LED_SES_REQ_EN_BB;
	} else if (strcmp(name, "ses_enable_ba") == 0) {
		ibpi = LED_SES_REQ_EN_BA;
	} else if (strcmp(name, "ses_devoff") == 0) {
		ibpi = LED_SES_REQ_DEV_OFF;
	} else if (strcmp(name, "ses_fault") == 0) {
		ibpi = LED_SES_REQ_FAULT;
	} else if (strcmp(name, "ses_prdfail") == 0) {
		ibpi = LED_SES_REQ_PRDFAIL;
	} else {
		return NULL;
	}
	state = _ibpi_find(&ibpi_list, ibpi);
	if (state == NULL)
		state = _ibpi_state_init(ibpi);
	return state;
}

bool _block_device_search(const struct list *block_list,
						 const char *path)
{
	char *block;

	list_for_each(block_list, block) {
		if (strcmp(block, path) == 0)
			return true;
	}
	return false;
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
static ledctl_status_code_t _ibpi_state_add_block(struct ibpi_state *state, char *name)
{
	char path[PATH_MAX];
	led_status_t rc = led_device_name_lookup(name, path);

	if (rc != LED_STATUS_SUCCESS)
		return rc;

	if (!led_is_management_supported(ctx, path)) {
		log_error("%s: device not supported", name);
		return LEDCTL_STATUS_NOT_SUPPORTED;
	}

	if (!_block_device_search(&state->block_list, path)) {
		char *path_copy = strdup(path);

		if (!path_copy || !list_append(&state->block_list, path_copy)) {
			free(path_copy);
			return LED_STATUS_OUT_OF_MEMORY;
		}
	} else {
		char buf[IPBI2STR_BUFF_SIZE];
		log_info("%s: %s: device already on the list.",
			 ibpi2str(state->ibpi, buf, sizeof(buf)), path);
	}
	return LEDCTL_STATUS_SUCCESS;
}

static ledctl_status_code_t verify_block_lists(void)
{
	if (!list_is_empty(&ibpi_list)) {
		struct ibpi_state *state;
		char buf[IPBI2STR_BUFF_SIZE];

		list_for_each(&ibpi_list, state) {
			if (list_is_empty(&state->block_list)) {
				log_warning("IBPI %s: missing block device(s)... pattern ignored.",
						ibpi2str(state->ibpi, buf, sizeof(buf)));
			}
		}
	} else {
		log_error("missing operand(s)... run %s --help for details.", progname);
		return LEDCTL_STATUS_LIST_EMPTY;
	}
	return LEDCTL_STATUS_SUCCESS;
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
 * @return LEDCTL_STATUS_SUCCESS if successful, otherwise ledctl_status_code_t.
 */
static ledctl_status_code_t _cmdline_ibpi_parse(int argc, char *argv[])
{
	ledctl_status_code_t t_status, ret_status = LEDCTL_STATUS_SUCCESS;

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
				return LEDCTL_STATUS_INVALID_STATE;
			}
			if (*t == '{') {
				while ((t = argv[optind++]) != NULL) {
					if (*t == '}')
						break;
					t_status =
					_ibpi_state_add_block(state, t);
					if (t_status != LEDCTL_STATUS_SUCCESS)
						ret_status = t_status;
				}
			} else {
				while (*(p = t) != '\0') {
					t = strchrnul(p, ',');
					if (*t != '\0')
						*(t++) = '\0';
					t_status =
					_ibpi_state_add_block(state, p);
					if (t_status != LEDCTL_STATUS_SUCCESS)
						ret_status = t_status;
				}
			}
		}
	}

	if (verify_block_lists() != LEDCTL_STATUS_SUCCESS)
		ret_status = LEDCTL_STATUS_IBPI_DETERMINE_ERROR;

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
 * @return LEDCTL_STATUS_SUCCESS if successful, otherwise ledctl_status_code_t.
 */
static ledctl_status_code_t _cmdline_parse_non_root(int argc, char *argv[])
{
	int opt_index, opt = -1;
	ledctl_status_code_t status = LEDCTL_STATUS_SUCCESS;

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
			return LEDCTL_STATUS_CMDLINE_ERROR;
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
	slot_req->state = LED_IBPI_PATTERN_UNKNOWN;
}

struct led_slot_list_entry *find_slot(struct led_ctx *ctx, struct slot_request *slot_req)
{
	if (slot_req->device[0] != '\0')
		return led_slot_find_by_device_name(ctx, slot_req->cntrl, slot_req->device);
	else if (slot_req->slot[0] != '\0')
		return led_slot_find_by_slot(ctx, slot_req->cntrl, slot_req->slot);
	return NULL;
}

/**
 * @brief Verifies slot request parameters.
 *
 * @param[in]       slot_req       structure with slot request
 *
 * @return LEDCTL_STATUS_SUCCESS if successful, otherwise ledctl_status_code_t.
 */
static ledctl_status_code_t slot_verify_request(struct led_ctx *ctx, struct slot_request *slot_req)
{
	if (slot_req->cntrl == LED_CNTRL_TYPE_UNKNOWN) {
		log_error("Invalid controller in the request.");
		return LEDCTL_STATUS_INVALID_CONTROLLER;
	}
	if (slot_req->chosen_opt == OPT_SET_SLOT && slot_req->state == LED_IBPI_PATTERN_UNKNOWN) {
		log_error("Invalid IBPI state in the request.");
		return LEDCTL_STATUS_INVALID_STATE;
	}
	if (slot_req->device[0] && slot_req->slot[0]) {
		log_error("Device and slot parameters are exclusive.");
		return LEDCTL_STATUS_DATA_ERROR;
	}
	if (slot_req->chosen_opt != OPT_LIST_SLOTS) {
		struct led_slot_list_entry *slot = find_slot(ctx, slot_req);

		if (!slot) {
			log_error("Slot was not found for provided parameters.");
			return LEDCTL_STATUS_CMDLINE_ERROR;
		}
		led_slot_list_entry_free(slot);
	}
	return LEDCTL_STATUS_SUCCESS;
}


static inline void print_slot(struct led_slot_list_entry *s)
{
	char buf[IPBI2STR_BUFF_SIZE];
	const char *device_name = led_slot_device(s);

	printf("slot: %-15s led state: %-15s device: %-15s\n",
		basename(led_slot_id(s)), ibpi2str(led_slot_state(s), buf, sizeof(buf)),
		(device_name != NULL) ? device_name : "(empty)");
}


/**
 * @brief List slots connected to given controller
 *
 * This function scans all available slots connected to given controller
 * and prints their led states and names of the connected devices (if exist).
 *
 * @param[in]       cntrl_type       What type of controller to list slots for
 *
 * @return LEDCTL_STATUS_SUCCESS if successful, otherwise ledctl_status_code_t.
 */

static ledctl_status_code_t list_slots(enum led_cntrl_type cntrl_type)
{
	struct led_slot_list_entry *slot = NULL;
	led_status_t  status = LED_STATUS_SUCCESS;
	struct led_slot_list *slot_list = NULL;

	status = led_slots_get(ctx, &slot_list);
	if (status != LED_STATUS_SUCCESS) {
		log_error("Failed to retrieve slots, reason %s\n",
				ledctl_strstatus((ledctl_status_code_t)status));
		return status;
	}

	while ((slot = led_slot_next(slot_list))) {
		if (cntrl_type == led_slot_cntrl(slot))
			print_slot(slot);
	}

	led_slot_list_free(slot_list);
	return (ledctl_status_code_t)status;
}

/**
 * @brief Executes proper slot mode function.
 *
 * @param[in]       slot_req       Structure with slot request.
 *
 * @return LEDCTL_STATUS_SUCCESS if successful, otherwise ledctl_status_code_t.
 */
ledctl_status_code_t slot_execute(struct led_ctx *ctx, struct slot_request *slot_req)
{
	struct led_slot_list_entry *slot;

	if (slot_req->chosen_opt == OPT_LIST_SLOTS)
		return list_slots(slot_req->cntrl);

	slot = find_slot(ctx, slot_req);
	if (slot == NULL)
		return LEDCTL_STATUS_DATA_ERROR;

	switch (slot_req->chosen_opt) {
	case OPT_SET_SLOT:
	{
		led_status_t set_rc = LEDCTL_STATUS_SUCCESS;
		char buf[IPBI2STR_BUFF_SIZE];

		if (slot_req->state != LED_IBPI_PATTERN_LOCATE_OFF
		    && led_slot_state(slot) == slot_req->state) {
			log_warning("Led state: %s is already set for the slot.",
				    ibpi2str(slot_req->state, buf, sizeof(buf)));
		} else {
			set_rc = led_slot_set(ctx, slot, slot_req->state);
		}
		led_slot_list_entry_free(slot);
		return set_rc;
	}
	case OPT_GET_SLOT:
		print_slot(slot);
		led_slot_list_entry_free(slot);
		return LEDCTL_STATUS_SUCCESS;
	default:
		led_slot_list_entry_free(slot);
		return LEDCTL_STATUS_NOT_SUPPORTED;
	}
}

static void print_cntrl(struct led_cntrl_list_entry *cntrl)
{
	printf("%s (%s)\n", led_cntrl_path(cntrl),
		led_cntrl_type_to_string(led_cntrl_type(cntrl)));
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
 * @return LEDCTL_STATUS_SUCCESS if successful, otherwise ledctl_status_code_t.
 */
ledctl_status_code_t _cmdline_parse(int argc, char *argv[], struct slot_request *req)
{
	int opt, opt_index = -1;
	ledctl_status_code_t status = LEDCTL_STATUS_SUCCESS;

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
					status = set_verbose_level(&conf, log_level);
				else
					status = LEDCTL_STATUS_CMDLINE_ERROR;
				break;
			default:
				status = set_verbose_level(
						&conf, possible_params[opt_index]);

			}
			break;
		case 'l':
			status = set_log_path(&conf, optarg);
			break;
		case 'x':
			status = LEDCTL_STATUS_SUCCESS;
			listed_only = 1;
			break;
		case 'L':
		{
			struct led_cntrl_list_entry *cntrl = NULL;
			struct led_cntrl_list *cntrls = NULL;
			led_status_t status;

			status = led_cntrls_get(ctx, &cntrls);
			if (status != LED_STATUS_SUCCESS) {
				log_error("Error on controller retrieval %s\n",
					  ledctl_strstatus((ledctl_status_code_t)status));
				exit(EXIT_FAILURE);
			}

			while ((cntrl = led_cntrl_next(cntrls)))
				print_cntrl(cntrl);

			led_cntrl_list_free(cntrls);

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
			req->cntrl = led_string_to_cntrl_type(optarg);
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
			return LEDCTL_STATUS_CMDLINE_ERROR;
		}
		opt_index = -1;
		if (status != LEDCTL_STATUS_SUCCESS)
			return status;
	} while (1);

	return LEDCTL_STATUS_SUCCESS;
}

/**
 * @brief Send IBPI pattern.
 *
 * This is internal function of ledctl utility. The function set a requested
 * ibpi_state for devices linked with this ibpi_state on ibpi_local_list.
 * For other devices LED_IBPI_PATTERN_LOCATE_OFF might be set - depending on
 * listed_only parameter. Then it sends a LED control message to controller
 * to visualize the pattern.
 *
 * @param[in]      ibpi_local_list	    pointer to list of ipbi_state.
 *
 * @return LEDCTL_STATUS_SUCCESS if successful, otherwise LEDCTL_STATUS_IBPI_DETERMINE_ERROR
 */
static ledctl_status_code_t _ledctl_execute(struct list *ibpi_local_list)
{
	struct ibpi_state *state;
	char *device;

	if (!listed_only) {
		off_all(ctx);
	}

	list_for_each(ibpi_local_list, state)
		list_for_each(&state->block_list, device) {
			led_set(ctx, device, state->ibpi);
		}

	led_flush(ctx);

	return LEDCTL_STATUS_SUCCESS;
}

static ledctl_status_code_t _read_shared_conf(void)
{
	ledctl_status_code_t status;
	char share_conf_path[PATH_MAX];

	memset(share_conf_path, 0, sizeof(share_conf_path));
	snprintf(share_conf_path, sizeof(share_conf_path), "/dev/shm%s",
		 LEDMON_SHARE_MEM_FILE);

	status = ledmon_read_conf(share_conf_path, &conf);
	return status;
}

/**
 * @brief Unset unsupported config parameters.
 *
 * For ledctl only LOG_LEVEL and LOG_PATH are supported and desired.
 * Unset other options.
 */
static void _unset_unused_options(void)
{
	conf.blink_on_init = false;
	conf.blink_on_migration = false;
	list_erase(&conf.cntrls_excludelist);
	list_erase(&conf.cntrls_allowlist);
	conf.raid_members_only = false;
	conf.rebuild_blink_on_all = false;
	conf.scan_interval = 0;
}

static ledctl_status_code_t _init_ledctl_conf(void)
{
	return ledmon_init_conf(&conf, LOG_LEVEL_WARNING, LEDCTL_DEF_LOG_FILE);
}

/**
 * @brief Propagate the configuration setting to the library settings
 */
static led_status_t load_library_prefs(void)
{
	device_blink_behavior_set(ctx,
		conf.blink_on_migration,
		conf.blink_on_init,
		conf.rebuild_blink_on_all,
		conf.raid_members_only);

	led_log_fd_set(ctx, get_log_fd(&conf));
	led_log_level_set(ctx, conf.log_level);
	return LEDCTL_STATUS_SUCCESS;
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
 * @return LEDCTL_STATUS_SUCCESS if successful, otherwise ledctl_status_code_t.
 */
int main(int argc, char *argv[])
{
	led_status_t lib_rc;

	ledctl_status_code_t status;
	struct slot_request slot_req;

	setup_options(&longopt, &shortopt, possible_params,
			possible_params_size);
	set_invocation_name(argv[0]);

	if (_cmdline_parse_non_root(argc, argv) != LEDCTL_STATUS_SUCCESS)
		return LEDCTL_STATUS_CMDLINE_ERROR;

	openlog(progname, LOG_PERROR, LOG_USER);

	if (geteuid() != 0) {
		fprintf(stderr, "Only root can run this application.\n");
		return LEDCTL_STATUS_NOT_A_PRIVILEGED_USER;
	}

	lib_rc = led_new(&ctx);
	if (lib_rc != LED_STATUS_SUCCESS) {
		fprintf(stderr, "Unable to initialize LED library.\n");
		return lib_rc;
	}

	status = _init_ledctl_conf();
	if (status != LEDCTL_STATUS_SUCCESS)
		return status;

	if (on_exit(_ledctl_fini, progname))
		exit(LEDCTL_STATUS_ONEXIT_ERROR);

	status = _read_shared_conf();
	if (status != LEDCTL_STATUS_SUCCESS)
		return status;
	_unset_unused_options();
	status = log_open(&conf);
	if (status != LEDCTL_STATUS_SUCCESS)
		return LEDCTL_STATUS_LOG_FILE_ERROR;

	status = load_library_prefs();
	if (status != LEDCTL_STATUS_SUCCESS) {
		log_error("Unable to set library preferences %s", ledctl_strstatus(status));
		return status;
	}
	status = led_scan(ctx);
	if (status != LEDCTL_STATUS_SUCCESS) {
		log_error("Error on led_scan %s", ledctl_strstatus(status));
		return status;
	}

	slot_request_init(&slot_req);
	status = _cmdline_parse(argc, argv, &slot_req);
	if (status != LEDCTL_STATUS_SUCCESS)
		exit(LEDCTL_STATUS_CMDLINE_ERROR);
	free(shortopt);
	free(longopt);

	list_init(&ibpi_list, (item_free_t)ibpi_state_fini);
	if (slot_req.chosen_opt != OPT_NULL_ELEMENT) {
		status = slot_verify_request(ctx, &slot_req);
		if (status == LEDCTL_STATUS_SUCCESS)
			return slot_execute(ctx, &slot_req);
		else
			exit(status);
	}
	status = _cmdline_ibpi_parse(argc, argv);
	if (status != LEDCTL_STATUS_SUCCESS) {
		log_debug("main(): _ibpi_parse() failed (status=%s).",
			  ledctl_strstatus(status));
		exit(status);
	}
	return _ledctl_execute(&ibpi_list);
}
