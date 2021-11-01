

#include <stdio.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>

#include "disk.h"
#include "mbr.h"
#include "gpt.h"
#include "linux.h"
#include "part-table.h"


static int number = 0;


static int
doit()
{
    disk_t* disk = disk_new(device, O_RDWR, fallback_sector_size);

    // uint32_t sector_size = disk_sector_size(disk);

#if 0

    mbr_t* mbr = mbr_read(disk);

    linux_remove_partition(disk, number);

    mbr_remove_partition(mbr, number);

    // linux_discard(disk, start * sector_size, size * sector_size);
    // linux_wipe_signatures(disk, start * sector_size, size * sector_size);

    mbr_write(mbr, disk);

    mbr_free(mbr);

#else

    gpt_t* gpt = gpt_read(disk);

    linux_remove_partition(disk, number);

    gpt_remove_partition(gpt, number);

    // linux_discard(disk, start * sector_size, size * sector_size);
    // linux_wipe_signatures(disk, start * sector_size, size * sector_size);

    gpt_write(gpt, disk);

    gpt_free(gpt);

#endif

    disk_free(disk);

    return 1;
}


int
cmd_remove_partition(int argc, char** argv)
{
    static const struct option long_options[] = {
	{ "number", required_argument, NULL, 'n' },
	{ NULL, 0, NULL, 0}
    };

    while (true)
    {
	int c = getopt_long(argc, argv, "+n:", long_options, NULL);
	if (c < 0)
	    break;

	switch (c)
	{
	    case 'n':
	    {
		number = strtol(optarg, NULL, 0);
	    }
	    break;
	}
    }

    printf("number: %d\n", number);

    if (number == 0)
    {
	fprintf(stderr, "number missing\n");
	exit(EXIT_FAILURE);
    }

    return doit();
}
