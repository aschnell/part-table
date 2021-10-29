

#include <stdio.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>

#include "disk.h"
#include "mbr.h"
#include "gpt.h"
#include "part-table.h"
#include "utils.h"


static int partition_entries = 128;
static int partition_entry_size = 128;
static uuid_t guid;


static int
doit()
{
    disk_t* disk = disk_new(device, O_RDWR);

#if 0



#else

    gpt_t* gpt = gpt_create(disk, partition_entries, partition_entry_size);

    if (!uuid_is_null(guid))
	gpt_set_guid(gpt, guid);

    gpt_write(gpt, disk);

    gpt_free(gpt);

#endif

    disk_free(disk);

    return 1;
}


int
cmd_create_partition_table(int argc, char** argv)
{
    enum { OPT_PARTITION_ENTRIES = 128, OPT_PARTITION_ENTRY_SIZE, OPT_GUID };

    static const struct option long_options[] = {
	{ "partition-entries", required_argument, NULL, OPT_PARTITION_ENTRIES },
	{ "partition-entry-size", required_argument, NULL, OPT_PARTITION_ENTRY_SIZE },
	{ "guid", required_argument, NULL, OPT_GUID },
	{ NULL, 0, NULL, 0}
    };

    while (true)
    {
	int c = getopt_long(argc, argv, "+", long_options, NULL);
	if (c < 0)
	    break;

	switch (c)
	{
	    case OPT_PARTITION_ENTRIES:
	    {
		partition_entries = strtol(optarg, NULL, 0);
	    }
	    break;

	    case OPT_PARTITION_ENTRY_SIZE:
	    {
		partition_entry_size = strtol(optarg, NULL, 0);

		if (partition_entry_size < 128 || __builtin_popcount(partition_entry_size) != 1)
		{
		    if (!berserker)
			error("invalid partition-entry-size");
		}
	    }
	    break;

	    case OPT_GUID:
	    {
		if (uuid_parse(optarg, guid) != 0)
		{
		    fprintf(stderr, "failed to parse guid\n");
		    exit(EXIT_FAILURE);
		}
	    }
	    break;
	}
    }

    // TODO check optind for extra args

    return doit();
}
