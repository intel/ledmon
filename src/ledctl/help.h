#ifndef HELP_H_
#define HELP_H_

#include <stdlib.h>
#include "utils.h"

struct ledctl_option {
	enum opt option_id;
	char *example;
	char *description;
};

struct ledctl_mode {
	enum opt option_id;
	char *description;
	char *long_description;
	struct ledctl_option *options;
	int options_count;
	int logging_support;
};

void _cmdline_parse_mode_help(int argc, char *argv[], int req);
void _print_main_help(void);
void _ledctl_version(void);

#endif // HELP_H_INCLUDED_
