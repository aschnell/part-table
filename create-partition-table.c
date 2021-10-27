

#include <fcntl.h>

#include "disk.h"
#include "mbr.h"
#include "gpt.h"
#include "part-table.h"


static int
doit()
{
    disk_t* disk = disk_new(device, O_RDWR);

#if 0



#else

    gpt_t* gpt = gpt_create(disk);

    gpt_write(gpt, disk);

    gpt_free(gpt);

#endif

    disk_free(disk);

    return 1;
}


int
cmd_create_partition_table(int argc, char** argv)
{
    return doit();
}
