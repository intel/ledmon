// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Intel Corporation.

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "help.h"
#include "utils.h"
#include "config_ac.h"

struct help_option {
	char *example;
	char *description;
	struct option *option;
};

#define HELP_OPTION_CNTRL_TYPE \
{"CNTRL", "Controller type.", &longopt_all[OPT_CNTRL_TYPE]}

#define HELP_OPTION_DEVICE \
{"DEVNODE", "Device devnode.", &longopt_all[OPT_DEVICE]}

#define HELP_OPTION_HELP \
{NULL, "Display this text.", &longopt_all[OPT_HELP]}

#define HELP_OPTION_LISTED_ONLY \
{NULL, "Change state for given devices only, optional.", &longopt_all[OPT_LISTED_ONLY]}

#define HELP_OPTION_LOG_LEVEL \
{"LEVEL", "Set log level, optional.",  &longopt_all[OPT_LOG_LEVEL]}

#define HELP_OPTION_PRINT_PARAM \
{"PROP", "Print chosen property of the output, optional.",  &longopt_all[OPT_PRINT_PARAM]}

#define HELP_OPTION_SLOT \
{"SLOT", "Unique slot identifier.",  &longopt_all[OPT_SLOT]}

#define HELP_OPTION_STATE \
{"PATTERN", "IBPI pattern to set.",  &longopt_all[OPT_STATE]}

#define HELP_OPTION_VERSION \
{NULL, "Displays version and license information.", &longopt_all[OPT_VERSION]}

/* This one is special because it is not parsed by get_opt_long() */
#define HELP_OPTION_IBPI \
{"<PATTERN>={ devices }", "Set IBPI pattern on given devices.", NULL}

static struct help_option IBPI_HELP_OPTS[] = {
	HELP_OPTION_IBPI,
	HELP_OPTION_HELP,
	HELP_OPTION_LISTED_ONLY,
	HELP_OPTION_LOG_LEVEL,
};

static struct help_option LIST_SLOTS_HELP_OPTS[] = {
	HELP_OPTION_CNTRL_TYPE,
	HELP_OPTION_HELP,
	HELP_OPTION_LOG_LEVEL,
};

static struct help_option GET_SLOT_HELP_OPTS[] = {
	HELP_OPTION_CNTRL_TYPE,
	HELP_OPTION_DEVICE,
	HELP_OPTION_HELP,
	HELP_OPTION_LOG_LEVEL,
	HELP_OPTION_PRINT_PARAM,
	HELP_OPTION_SLOT,
};

static struct help_option SET_SLOT_HELP_OPTS[] = {
	HELP_OPTION_CNTRL_TYPE,
	HELP_OPTION_DEVICE,
	HELP_OPTION_HELP,
	HELP_OPTION_LOG_LEVEL,
	HELP_OPTION_SLOT,
	HELP_OPTION_STATE,
};

static struct help_option LIST_CTRL_HELP_OPTS[] = {
	HELP_OPTION_HELP,
	HELP_OPTION_LOG_LEVEL,
};

static struct help_option BEST_CTRL_HELP_OPTS[] = {
	HELP_OPTION_DEVICE,
	HELP_OPTION_HELP,
	HELP_OPTION_LOG_LEVEL,
};

struct help_mode {
	enum opt option_id;
	struct option *opt;
	char *long_description;
	int help_opts_count;
	/* Supported parameters by this mode */
	struct help_option *help_opts;
};

#define HELP_MODE(MODE, DESC) \
{\
	.option_id = OPT_##MODE,\
	.opt = &longopt_all[OPT_##MODE],\
	.long_description = DESC,\
	.help_opts_count = ARRAY_SIZE(MODE##_HELP_OPTS),\
	.help_opts = MODE##_HELP_OPTS,\
}

/**
 * Array of help for modes.
 */
static struct help_mode modes[] = {
	HELP_MODE(IBPI,
		  "Set IBPI pattern or patterns on given devices.\n"
		  "By default, It may update states on the remaining devices.\n"
		  "Refer to ledctl(8) man page for more examples of usage."),

	HELP_MODE(GET_SLOT,
		  "Print slot details for given slot or device under the given controller.\n"
		  "Options \"--slot\" and \"--device\" cannot be used simultaneously."),

	HELP_MODE(LIST_CTRL,
		  "Print information of all controllers detected by ledmon."),

	HELP_MODE(LIST_SLOTS,
		  "Print all slots for a controller in the request."),

	HELP_MODE(SET_SLOT,
		  "Set given state for given slot or device under the given controller.\n"
		  "Options \"--slot\" and \"--device\" cannot be used simultaneously."),

	HELP_MODE(BEST_CTRL,
		  "Print the controller with the highest priority for given device."),
};

/**
 * @brief Displays the credits.
 *
 * It prints out the name and version of the program.
 */
void _ledctl_version(void)
{
	printf("Intel(R) Enclosure LED Control Application %s %s\n"
	       "Copyright (C) 2009-2024 Intel Corporation.\n\n", PACKAGE_VERSION, BUILD_LABEL);
}

/**
 * @brief Print help footer.
 *
 * It prints out the reference to manual and method of reporting defects.
 */
static void print_ledctl_help_footer(void)
{
	printf("\nRefer to ledctl(8) man page for more detailed description (man ledctl).\n");
	printf("Bugs should be reported at: https://github.com/intel/ledmon/issues\n");
}

void _print_incorrect_help_usage(void)
{
	printf("Incorrect usage of --help detected\n");
	printf("Help can be used alone or with mode e.g %s --help, %s --ibpi --help\n",
	       progname, progname);
}

/**
 * @brief Print help header.
 *
 * It prints out the name, version of the program and general or mode usage.
 *
 * @param[in]      mode        mode, optional.
 */
static void print_ledctl_help_header(struct help_mode *mode)
{
	_ledctl_version();

	if (!mode->opt)
		printf("Usage: %s --<mode> [option...] ...", progname);
	else
		printf("Usage: %s --%s [option...] ...", progname, mode->opt->name);

	printf("\n\n");
}

/**
 * @brief Prepares longopt string.
 *
 * The function prepares long option with example argument if applicable:
 * - optional argument "[<arg>]"
 * - required argument "<arg>"
 *
 * @param[in]      help_opt     option.
 * @param[out]     res          prepared long opt string.
 * @param[in]      res_size     result buffer size.
 *
 * @return Length of generated string is returned or negative value on error.
 */
static int prepare_longopt_string(struct help_option *opt, char *res, int res_size)
{
	if (!opt->option) {
		/* If no option must use example */
		assert(opt->example);
		return snprintf(res, res_size, "%s", opt->example);
	}

	switch (opt->option->has_arg) {
	case no_argument:
		return snprintf(res, res_size, "--%s", opt->option->name);
	case required_argument:
		return snprintf(res, res_size, "--%s <%s>", opt->option->name, opt->example);
	case optional_argument:
		return snprintf(res, res_size, "--%s [<%s>]", opt->option->name, opt->example);
	}

	return -1;
}

/**
 * Max size of generated long option string.
 */
#define LONG_OPT_HELP_STR_MAX 30

/**
 * @brief Displays struct help_mode content.
 *
 * The function prints description and available help options of help_mode.
 * First, all long options strings are generated to determine short option width to keep output
 * consistent.
 *
 * @param[in]      help_mode       help_mode to be printed.
 *
 */
static void print_help_mode(struct help_mode *help_mode)
{
	char longopts[help_mode->help_opts_count][LONG_OPT_HELP_STR_MAX];
	int max_long_opt_length = 0;
	int i;

	print_ledctl_help_header(help_mode);
	printf("%s\n", help_mode->long_description);

	/* mode->opt is not set for general help */
	if (help_mode->opt)
		printf("\nOptions:\n");
	else
		printf("\nModes:\n");

	for (i = 0; i < help_mode->help_opts_count; i++) {
		int long_opt_length = prepare_longopt_string(&help_mode->help_opts[i], longopts[i],
							     LONG_OPT_HELP_STR_MAX);

		/* String may be truncated if it is longer than buffer */
		assert(long_opt_length < LONG_OPT_HELP_STR_MAX);

		if (long_opt_length > max_long_opt_length)
			max_long_opt_length = long_opt_length;
	}

	for (i = 0; i < help_mode->help_opts_count; i++) {
		struct help_option *help_opt = &help_mode->help_opts[i];
		char shortopt = ' ';

		if (help_opt->option && help_opt->option->val != '\0')
			shortopt = help_opt->option->val;

		printf("%-*s  -%-4c%s\n", max_long_opt_length, longopts[i], shortopt,
		       help_opt->description);
	}

	print_ledctl_help_footer();
}

/**
 * @brief Displays help for mode referenced mode_id.
 *
 * @param[in]      mode_id       mode.
 */
void print_mode_help(enum opt mode_id)
{
	int i;
	struct help_mode *mode = NULL;

	for (i = 0; i < ARRAY_SIZE(modes); i++)
		if (modes[i].option_id == mode_id)
			mode = &modes[i];

	/* All modes are described */
	assert(mode && mode->help_opts_count > 0);
	print_help_mode(mode);
}

/**
 * Array of general help modes with description.
 */
static struct help_option GENERAL_HELP_OPTS[] = {
	{NULL, "Print slot details for device/slot.",  &longopt_all[OPT_GET_SLOT]},
	{NULL, "Indicate IBPI mode, it is used as default.", &longopt_all[OPT_IBPI]},
	{NULL, "Display list of controllers recognizable by ledctl.", &longopt_all[OPT_LIST_CTRL]},
	{NULL, "Print best supported controller for device.", &longopt_all[OPT_BEST_CTRL]},
	{NULL, "Print all slots for a controller requested.", &longopt_all[OPT_LIST_SLOTS]},
	{NULL, "Set state for slot/device by controller requested.", &longopt_all[OPT_SET_SLOT]}
};

/**
 * General help. Implemented as special mode.
 */
static struct help_mode general_mode = {
	.option_id = OPT_NULL_ELEMENT,
	.opt = NULL,
	.long_description = "Ledctl allows to manipulate LED states for chosen devices or slots.\n"
			    "See 'ledctl --<mode> --help' for help of a specific mode.",
	.help_opts_count = ARRAY_SIZE(GENERAL_HELP_OPTS),
	.help_opts = GENERAL_HELP_OPTS,
};

/**
 * @brief Displays the main ledctl help.
 *
 * The function prints main help with modes listed.
 */
void _print_main_help(void)
{
	print_help_mode(&general_mode);
}
