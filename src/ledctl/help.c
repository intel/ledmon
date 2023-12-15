#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "help.h"
#include "utils.h"
#include "config_ac.h"

#define DEFAULT_ARG "ARG"
#define CONTROLLER "CONTROLLER"
#define SLOT "SLOT"
#define DEVNODE "DEVNODE"

/**
 * Internal array of ledctl utility.
 * Array of logging options with description.
 */
static struct ledctl_option log_opts[] = {
	{OPT_ALL, NULL, "Set all log level."},
	{OPT_DEBUG, NULL, "Set debug log level."},
	{OPT_ERROR, NULL, "Set error log level."},
	{OPT_INFO, NULL, "Set info log level."},
	{OPT_QUIET, NULL, "Set quiet log level."},
	{OPT_WARNING, NULL, "Set warning log level."},
	{OPT_LOG, "PATH", "Set path to logging."},
	{OPT_LOG_LEVEL, "LEVEL", "Set log level."}
};

/**
 * NOTE: OPT_NULL_ELEMENT was used to define an ledctl option
 * that is not defined in longopt_all array.
 * For OPT_NULL_ELEMENT Only example and description from ledctl_option
 * will be used to print the help line.
 * Example in ibpi_opts.
 */

/**
 * Internal array of ledctl utility.
 * Array of ibpi options with description.
 */
static struct ledctl_option ibpi_opts[] = {
	{OPT_LISTED_ONLY, NULL, "Ledctl will change state only for given devices."},
	{OPT_NULL_ELEMENT, "<IBPI_PATTERN>={devices}", "IBPI pattern to set on given devices."}
};

/**
 * Internal array of ledctl utility.
 * Array of --list-slots options with description.
 */
static struct ledctl_option list_slots_opts[] = {
	{OPT_CNTRL_TYPE, CONTROLLER, "Controller type."}
};

/**
 * Internal array of ledctl utility.
 * Array of --get-slots options with description.
 */
static struct ledctl_option get_slot_opts[] = {
	{OPT_CNTRL_TYPE, CONTROLLER, "Controller type."},
	{OPT_DEVICE, DEVNODE, "Device devnode."},
	{OPT_SLOT, SLOT, "Unique slot identifier"}
};

/**
 * Internal array of ledctl utility.
 * Array of --set-slot options with description.
 */
static struct ledctl_option set_slot_opts[] = {
	{OPT_CNTRL_TYPE, CONTROLLER, "Controller type."},
	{OPT_DEVICE, DEVNODE, "Device devnode."},
	{OPT_SLOT, SLOT, "Unique slot identifier"},
	{OPT_STATE, "IBPI_STATE", "IBPI state is led pattern."}
};

/**
 * Internal array of ledctl utility.
 * Array of modes with the necessary information to print the main help.
 */
static struct ledctl_mode modes[] = {
	{
		.option_id = OPT_IBPI_MODE,
		.description = "Set pattern on given devices and update devices states.",
		.options = ibpi_opts,
		.options_count = ARRAY_SIZE(ibpi_opts),
		.logging_support = 1
	},
	{
		.option_id = OPT_LIST_CTRL,
		.description = "Displays list of controllers detected by ledmon.",
		.long_description = "Prints information (system path and type) of all controllers\n"
				    "detected by ledmon.",
		.options_count = 0,
		.logging_support = 1
	},
	{
		.option_id = OPT_LIST_SLOTS,
		.description = "Print list slots under the controller type.",
		.long_description = "Command print list of all slots for the controller type\n"
				    "with current state and attached device name (if any).\n"
				    "controller-type is type of controller (vmd, NPEM) that\n"
				    "should be scanned here.",
		.options = list_slots_opts,
		.options_count = ARRAY_SIZE(list_slots_opts),
		.logging_support = 1
	},
	{
		.option_id = OPT_GET_SLOT,
		.description = "Prints slot information.",
		.long_description = "Prints slot information like led state,"
				    " slot number and devnode\n for given device or slot.\n"
				    "Options \"slot\" and \"device\" cannot be used simultaneously.",
		.options = get_slot_opts,
		.options_count = ARRAY_SIZE(get_slot_opts),
		.logging_support = 1
	},
	{
		.option_id = OPT_SET_SLOT,
		.description = "Sets given state for chosen slot/device under the controller.",
		.long_description = "Sets given state for chosen slot/device under the controller.\n"
				    "Options \"slot\" and \"device\" cannot be used simultaneously.",
		.options = set_slot_opts,
		.options_count = ARRAY_SIZE(set_slot_opts),
		.logging_support = 1
	},
	{
		.option_id = OPT_HELP,
		.description = "Prints this text out.",
		.long_description = "Displays help text.",
		.options_count = 0,
		.logging_support = 0
	},
	{
		.option_id = OPT_VERSION,
		.description = "Displays version and license information.",
		.options_count = 0,
		.logging_support = 0
	}
};

/**
 * Internal variable of ledctl utility.
 * It is the pattern used to print out information
 * about the version of ledctl utility.
 */
static char *ledctl_version = "Intel(R) Enclosure LED Control Application %s %s\n"
			      "Copyright (C) 2009-2023 Intel Corporation.\n\n";

/**
 * @brief Displays the credits.
 *
 * This is internal function of ledctl utility.
 * It prints out the name and version of the program.
 * It displays the copyright notice and information
 * about the author and license, too.
 *
 * @return The function does not return a value.
 */
void _ledctl_version(void)
{
	printf(ledctl_version, PACKAGE_VERSION, BUILD_LABEL);
	printf("This is free software; see the source for copying conditions."
	       " There is NO warranty;\nnot even for MERCHANTABILITY or FITNESS"
	       " FOR A PARTICULAR PURPOSE.\n\n");
}

/**
 * @brief Print help footer.
 *
 * This is internal function of ledctl utility.
 * It prints out the reference to manual and
 * method of reporting defects.
 */
void print_ledctl_help_footer(void)
{
	printf("\nRefer to ledctl(8) man page for more detailed description (man ledctl).\n");
	printf("Bugs should be reported at: https://github.com/intel/ledmon/issues\n");
}

/**
 * @brief Get mode from modes array.
 *
 * This is internal function of ledctl utility.
 * This function looks for a mode in modes array.
 *
 * @param[in]      requested_mode_id    mode id to find.
 *
 * @return ledctl_mode structure if mode exist in modes array, NULL otherwise.
 */
struct ledctl_mode *get_mode_help(int requested_mode_id)
{
	int i = 0;

	while (i < ARRAY_SIZE(modes)) {
		if (modes[i].option_id == requested_mode_id)
			return &modes[i];
		i++;
	}
	return NULL;
}

/**
 * @brief Print help header.
 *
 * This is internal function of ledctl utility.
 * It prints out the name, version of the program
 * and example usage of ledctl/mode.
 *
 * @param[in]      mode        optional mode.
 */
void print_ledctl_help_header(struct ledctl_mode *mode)
{
	printf(ledctl_version, PACKAGE_VERSION, BUILD_LABEL);

	printf("Usage: %s --", progname);
	if (!mode)
		printf("<mode>");
	else
		printf("%s", longopt_all[mode->option_id].name);

	if (mode == NULL || mode->options_count != 0 || mode->logging_support == 1)
		printf(" [option...] ...");
	printf("\n\n");
}

/**
 * @brief Get max optlong help string length.
 *
 * This is internal function of ledctl utility.
 * The function calculates the max string length for the long options in mode.
 * It also includes in the calculations of example argument to print
 * and whether it is required or optional.
 * Max length allows options to be printed correctly in columns.
 *
 * @param[in]      mode_options        mode options.
 * @param[in]      options_count       count of options.
 *
 * @return max length if options_count > 0, otherwise 0.
 */
int get_max_optlong_width(struct ledctl_option *mode_options, int options_count)
{
	int i = 0;
	int len = 0;
	int max_len = 0;
	enum opt option_id;
	int optional_arg_additional_chars = 3; // " <>"
	int required_arg_additional_chars = 5; // " [<>]"
	int dashes_len = 2;


	while (i < options_count) {
		option_id = mode_options[i].option_id;

		// workaround for "<pattern>=" case.
		if (option_id == OPT_NULL_ELEMENT)
			len += strlen(mode_options[i].example) / sizeof(char);

		if (longopt_all[option_id].name != NULL) {
			len += dashes_len;
			len += strlen(longopt_all[option_id].name) / sizeof(char);
		}

		if (longopt_all[option_id].has_arg == required_argument)
			len += optional_arg_additional_chars;
		else if (longopt_all[option_id].has_arg == optional_argument)
			len += required_arg_additional_chars;

		if (longopt_all[option_id].has_arg != no_argument) {
			if (!is_string_null_or_empty(mode_options[i].example))
				len += strlen(mode_options[i].example) / sizeof(char);
			else
				len += strlen(DEFAULT_ARG);
		}

		if (len > max_len)
			max_len = len;
		len = 0;
		i++;
	}

	return max_len;
}

/**
 * @brief Prepare mode option string.
 *
 * This is internal function of ledctl utility.
 * The function prepares long option with example argument if applicable.
 * If the option has an argument, based on optionality,
 * the argument is prepared as described below:
 * - optional "[<arg>]"
 * - required "<arg>"
 * If option name is null or empty string is prepared.
 *
 * @param[in]      ledctl_opt      mode option.
 * @param[out]     result          prepared long opt string.
 * @param[in]      result_size     result buffer size.
 */
void prepare_opt_string(struct ledctl_option ledctl_opt, char *result, int result_size)
{
	char *argument_name = NULL;
	struct option option = longopt_all[ledctl_opt.option_id];

	if (!result)
		return;

	if (is_string_null_or_empty((char *)option.name)) {
		result[0] = '\0';
		return;
	}

	argument_name = !is_string_null_or_empty(ledctl_opt.example) ?
			ledctl_opt.example : DEFAULT_ARG;

	switch (option.has_arg) {
	case no_argument:
		snprintf(result, result_size, "--%s", option.name);
		break;
	case required_argument:
		snprintf(result, result_size, "--%s <%s>", option.name, argument_name);
		break;
	case optional_argument:
		snprintf(result, result_size, "--%s [<%s>]", option.name, argument_name);
	}
}

/**
 * @brief Print formatted option line.
 *
 * This is internal function of ledctl utility.
 *
 * @param[in]        long_opt       long option string.
 * @param[in]        long_opt_len   long option string length.
 * @param[in]        short_opt      short option char.
 * @param[in]        desc           option description.
 */
void print_opt_ledctl(char *long_opt, int long_opt_len, char short_opt, char *desc)
{
	assert(long_opt != NULL && long_opt[0] != '\0');

	desc = desc != NULL ? desc : "";

	if (short_opt == '\0')
		short_opt = ' ';

	printf("%-*s-%-4c%s\n", long_opt_len + 2, long_opt, short_opt, desc);
}

/**
 * @brief Print options help.
 *
 * This is internal function of ledctl utility.
 * It prints options with description of given array.
 *
 * @param[in]      options       array with options to print help.
 * @param[in]      log_opts_count       number of options in array.
 *
 */
void print_options(struct ledctl_option *options, int log_opts_count)
{
	int i = 0;
	int longopt_max_len = 0;
	enum opt option_id;
	char longopt_string[BUFSIZ];

	longopt_max_len = get_max_optlong_width(options, log_opts_count);

	while (i < log_opts_count) {
		option_id = options[i].option_id;

		if (option_id == OPT_NULL_ELEMENT) {
			print_opt_ledctl(options[i].example, longopt_max_len,
					 longopt_all[option_id].val, options[i].description);
		} else {
			prepare_opt_string(options[i], longopt_string, BUFSIZ);
			print_opt_ledctl(longopt_string, longopt_max_len,
					 longopt_all[option_id].val, options[i].description);
		}
		i++;
	};
}

/**
 * @brief Displays the logging options help.
 *
 * This is internal function of ledctl utility.
 * The function prints description and available logging options.
 */
void print_log_options(void)
{
	int log_opts_count = ARRAY_SIZE(log_opts);

	if (log_opts_count == 0)
		return;

	printf("\nLogging options:\n");

	print_options(log_opts, log_opts_count);
}

/**
 * @brief Displays the mode options help.
 *
 * This is internal function of ledctl utility.
 * The function prints description and available options of mode.
 *
 * @param[in]      mode       mode to print options help.
 */
void print_mode_options(struct ledctl_mode *mode)
{
	if (mode->options_count == 0)
		return;

	printf("\nOptions:\n");

	print_options(mode->options, mode->options_count);
}

/**
 * @brief Displays the mode help.
 *
 * This is internal function of ledctl utility.
 * The function prints the name and version of the program out.
 * It displays description and available options of mode.
 * Each option is described.
 *
 * @param[in]      mode       mode to print help.
 */
void print_mode_help(struct ledctl_mode *mode)
{
	print_ledctl_help_header(mode);

	printf("%s\n", is_string_null_or_empty(mode->long_description) == 0 ?
			mode->long_description : mode->description);

	if (mode->options_count > 0)
		print_mode_options(mode);

	if (mode->logging_support == 1)
		print_log_options();

	print_ledctl_help_footer();
}

/**
 * @brief Displays the main ledctl help.
 *
 * This is internal function of ledctl utility.
 * The function prints the name and version of the program out.
 * It displays available modes. Each mode is described.
 */
void _print_main_help(void)
{
	int i = 0;
	int longopt_len = 17;
	struct option option;
	char longopt_string[BUFSIZ];

	print_ledctl_help_header(NULL);

	while (i < ARRAY_SIZE(modes)) {
		option = longopt_all[modes[i].option_id];

		if (is_string_null_or_empty((char *)option.name))
			continue;

		snprintf(longopt_string, BUFSIZ, "--%s", option.name);
		print_opt_ledctl(longopt_string, longopt_len, option.val, modes[i].description);
		i++;
	}

	printf("\nSee '%s --<mode> --help' for more information on a specific mode.\n"
	       "e.g. %s %s --help\n", progname, progname, longopt_all[modes[0].option_id].name);

	print_ledctl_help_footer();
}

void _print_incorrect_help_usage(void)
{
	printf("Incorrect usage of --help.\nHelp can be used alone or with mode "
	       "e.g %s --help, %s --ibpi --help\n", progname, progname);
}

/**
 * @brief Displays the help.
 *
 * This is internal function of ledctl utility.
 * The function prints the name and version of the program out.
 * It displays the usage and available options and its arguments (if any).
 * Each option is described.
 */
void _cmdline_parse_mode_help(int argc, char *argv[], int command_id)
{
	char *shortopts;
	struct option *longopts;
	int opt;
	int opt_index;
	int status = EXIT_SUCCESS;
	int optind_backup = optind;
	struct ledctl_mode *mode;

	static int possible_params[] = {
		OPT_HELP,
	};

	setup_options(&longopts, &shortopts, possible_params,
		ARRAY_SIZE(possible_params));

	opt = getopt_long(argc, argv, shortopts, longopts, &opt_index);
	if (opt != 'h') {
		if (argc > 2 && (command_id == OPT_HELP)) {
			_print_incorrect_help_usage();
			status = EXIT_FAILURE;
			goto end;
		}
		/*
		 * Restore option index to enable correct
		 * processing of arguments in further stages.
		 */
		optind = optind_backup;
		return;
	}
	if (argc > 3) {
		_print_incorrect_help_usage();
		status = EXIT_FAILURE;
		goto end;
	}

	mode = get_mode_help(command_id);

	if (mode == NULL) {
		fprintf(stderr, "Help is unavailable for mode %s.\n", longopt_all[command_id].name);
		status = EXIT_FAILURE;
		goto end;
	}

	print_mode_help(mode);

end:
	free(longopts);
	free(shortopts);
	exit(status);
}
