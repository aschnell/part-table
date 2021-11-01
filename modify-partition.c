

#include <stdio.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"
#include "mbr.h"
#include "gpt.h"
#include "linux.h"
#include "part-table.h"
#include "utils.h"


static int number = 0;
static uint64_t size = 0;
static uuid_t type_guid;
static uuid_t guid;
static uint64_t attributes = 0;
static char* name = NULL;


static int
doit()
{
    disk_t* disk = disk_new(device, O_RDWR, fallback_sector_size);

    uint32_t sector_size = disk_sector_size(disk);

#if 0

    mbr_t* mbr = mbr_read(disk);

    linux_resize_partition(disk, number, start * sector_size, size * sector_size);

    mbr_write(mbr, disk);

    mbr_free(mbr);

#else

    gpt_t* gpt = gpt_read(disk);

    if (size != 0)
    {
	gpt_set_size(gpt, number, size);

	uint64_t start = gpt_get_start(gpt, number);
	linux_resize_partition(disk, number, start * sector_size, size * sector_size);
    }

    if (!uuid_is_null(type_guid))
	gpt_set_type_guid(gpt, number, type_guid);

    if (!uuid_is_null(guid))
	gpt_set_partition_guid(gpt, number, guid);

    if (attributes)
	gpt_set_attributes(gpt, number, attributes);

    if (name)
	gpt_set_name(gpt, number, name);

    gpt_write(gpt, disk);

    gpt_free(gpt);

#endif

    disk_free(disk);

    return 1;
}


int
cmd_modify_partition(int argc, char** argv)
{
    enum { OPT_SIZE = 128, OPT_TYPE_GUID, OPT_TYPE_GUID_NAME, OPT_GUID, OPT_ATTRIBUTES,
	OPT_ATTRIBUTE_NAMES, OPT_NAME };

    static const struct option long_options[] = {
	{ "number", required_argument, NULL, 'n' },
	{ "size", required_argument, NULL, OPT_SIZE },
	{ "type-guid", required_argument, NULL, OPT_TYPE_GUID },
	{ "type-guid-name", required_argument, NULL, OPT_TYPE_GUID_NAME },
	{ "guid", required_argument, NULL, OPT_GUID },
	{ "attributes", required_argument, NULL, OPT_ATTRIBUTES },
	{ "attribute-names", required_argument, NULL, OPT_ATTRIBUTE_NAMES },
	{ "name", required_argument, NULL, OPT_NAME },
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

	    case OPT_SIZE:
	    {
		if (parse_size(optarg, &size) != 0)
		{
		    fprintf(stderr, "failed to parse size\n");
		    exit(EXIT_FAILURE);
		}
	    }
	    break;

	    case OPT_TYPE_GUID:
	    {
		if (uuid_parse(optarg, type_guid) != 0)
		{
		    fprintf(stderr, "failed to parse type-guid\n");
		    exit(EXIT_FAILURE);
		}
	    }
	    break;

	    case OPT_TYPE_GUID_NAME:
	    {
		const gpt_name_type_guid_mapping_t* p = gpt_find_name_type_guid_mapping_by_name(optarg);
		if (!p)
		{
		    fprintf(stderr, "type-guid name not found\n");
		    exit(EXIT_FAILURE);
		}
		uuid_copy(type_guid, *(p->type_guid));
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

	    case OPT_ATTRIBUTES:
	    {
		attributes = strtoll(optarg, NULL, 0);
	    }
	    break;

	    case OPT_ATTRIBUTE_NAMES:
	    {
		// TODO something like +required,-legacy-boot to set and clear
		// TODO split optarg by comma
		const gpt_name_attribute_mapping_t* p = gpt_find_name_attribute_by_name(optarg);
		if (!p)
		{
		    fprintf(stderr, "attribute-name not found\n");
		    exit(EXIT_FAILURE);
		}

		attributes |= (1ULL << p->bit);
	    }
	    break;

	    case OPT_NAME:
	    {
		name = strdup(optarg);
	    }
	    break;
	}
    }

    // TODO check optind for extra args

    printf("number: %d\n", number);
    printf("size: %zd\n", size);

    if (number == 0)
    {
	fprintf(stderr, "number missing\n");
	exit(EXIT_FAILURE);
    }

    return doit();
}
