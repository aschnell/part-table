
// gcc -o create-partition-table create-partition-table.c mbr.c gpt.c disk.c linux.c utils.c -Wall -O2 -luuid -lblkid -ldevmapper -ljson-c


#include <fcntl.h>
#include <locale.h>

#include "disk.h"
#include "mbr.h"
#include "gpt.h"
#include "linux.h"


int
main()
{
    setlocale(LC_ALL, "");

    disk_t* disk = disk_new(O_RDWR);

#if 0



#else

    gpt_t* gpt = gpt_create(disk);

    gpt_write(gpt, disk);

    gpt_free(gpt);

#endif

    disk_free(disk);
}
