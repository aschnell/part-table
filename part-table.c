

#include <stdio.h>
#include <locale.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "part-table.h"


int cmd_print(int argc, char** argv);
int cmd_create_partition(int argc, char** argv);
int cmd_modify_partition(int argc, char** argv);
int cmd_remove_partition(int argc, char** argv);
int cmd_create_partition_table(int argc, char** argv);


struct cmd_s
{
    const char* name;
    int (*cmd)(int argc, char** argv);
};


static const struct cmd_s cmds[] =
{
    { "print", &cmd_print },
    { "create-partition", &cmd_create_partition },
    { "modify-partition", &cmd_modify_partition },
    { "remove-partition", &cmd_remove_partition },
    { "create-partition-table", &cmd_create_partition_table },
};


static const struct cmd_s*
find_cmd(const char* name)
{
    int n = sizeof(cmds) / sizeof(cmds[0]);

    for (int i = 0; i < n; ++i)
	if (strcmp(name, cmds[i].name) == 0)
	    return &cmds[i];

    return NULL;
}


char* device = NULL;

bool berserker = false;

bool discard = false;
bool wipe_signatures = false;

uint32_t fallback_sector_size = 512;


int
main(int argc, char** argv)
{
    setlocale(LC_ALL, "");

    enum { OPT_BERSERKER = 128, OPT_DISCARD, OPT_WIPE_SIGNATURES, OPT_SECTOR_SIZE };

    static const struct option long_options[] = {
	{ "berserker", no_argument, NULL, OPT_BERSERKER },
	{ "discard", no_argument, NULL, OPT_DISCARD },
	{ "wipe-signatures", no_argument, NULL, OPT_WIPE_SIGNATURES },
	{ "sector-size", required_argument, NULL, OPT_SECTOR_SIZE },
	{ NULL, 0, NULL, 0}
    };

    while (true)
    {
	int c = getopt_long(argc, argv, "+", long_options, NULL);

	if (c == -1 && !device)
	{
	    device = strdup(argv[optind++]);
	    continue;
	}

	if (c < 0)
	    break;

	switch (c)
	{
	    case OPT_BERSERKER:
		berserker = true;
		break;

	    case OPT_DISCARD:
		discard = true;
		break;

	    case OPT_WIPE_SIGNATURES:
		wipe_signatures = true;
		break;

	    case OPT_SECTOR_SIZE:
		fallback_sector_size = strtol(optarg, NULL, 0);
		break;
	}
    }

    printf("device: %s\n", device);

    const struct cmd_s* cmd = find_cmd(argv[optind++]);
    if (cmd)
	return (*cmd->cmd)(argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE;

    fprintf(stderr, "unknown command\n");

    return EXIT_FAILURE;
}
