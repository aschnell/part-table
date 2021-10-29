

#include <fcntl.h>
#include <getopt.h>

#include "disk.h"
#include "mbr.h"
#include "gpt.h"
#include "part-table.h"


static bool json = false;


static int
doit()
{
    disk_t* disk = disk_new(device, O_RDONLY);

    mbr_t* mbr = mbr_read(disk);

    mbr_print(mbr);

    mbr_free(mbr);

    gpt_t* gpt = gpt_read(disk);

    if (!json)
	gpt_print(gpt);
    else
	gpt_json(gpt);

    gpt_free(gpt);

    disk_free(disk);

    return 1;
}


int
cmd_print(int argc, char** argv)
{
    static const struct option long_options[] = {
	{ "json", no_argument, NULL, 'j' },
	{ NULL, 0, NULL, 0}
    };

    while (true)
    {
	int c = getopt_long(argc, argv, "+j", long_options, NULL);
	if (c < 0)
	    break;

	switch (c)
	{
	    case 'j':
	    {
		json = true;
	    }
	    break;
	}
    }

    return doit();
}
