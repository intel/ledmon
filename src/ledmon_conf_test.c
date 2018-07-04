#include <stdio.h>
#include <stdlib.h>

#include "config_file.h"
#include "ibpi.h"
#include "ibpi_logging.h"
#include "status.h"
#include "utils.h"

extern void ledmon_free_config(void);

/*
 * usage: ledmon_conf_test [<filename>]
 */
int main(int argc, char *argv[])
{
	char *filename = NULL;
	char *s;

	if (argc == 2)
		filename = argv[1];

	if (ledmon_read_config(filename) != STATUS_SUCCESS)
		return EXIT_FAILURE;

	printf("INTERVAL: %d\n", conf.scan_interval);
	printf("LOG_LEVEL: %d\n", conf.log_level);
	printf("LOG_PATH: %s\n", conf.log_path);
	printf("BLINK_ON_MIGR: %d\n", conf.blink_on_migration);
	printf("BLINK_ON_INIT: %d\n", conf.blink_on_init);
	printf("REBUILD_BLINK_ON_ALL: %d\n", conf.rebuild_blink_on_all);
	printf("RAID_MEMBERS_ONLY: %d\n", conf.raid_members_only);

	if (list_is_empty(&conf.cntrls_whitelist))
		printf("WHITELIST: NONE\n");
	else {
		printf("WHITELIST: ");
		list_for_each(&conf.cntrls_whitelist, s)
			printf("%s, ", s);
		printf("\n");
	}

	if (list_is_empty(&conf.cntrls_blacklist))
		printf("BLACKLIST: NONE\n");
	else {
		printf("BLACKLIST: ");
		list_for_each(&conf.cntrls_blacklist, s)
			printf("%s, ", s);
		printf("\n");
	}

	ledmon_free_config();
	return EXIT_SUCCESS;
}
