
// gcc -o remove-partition remove-partition.c mbr.c gpt.c disk.c linux.c utils.c -Wall -O2 -luuid -lblkid -ldevmapper -ljson-c


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

    size_t sector_size = disk_sector_size(disk);

    size_t start = 2048;
    size_t size = 10000;

#if 0

    mbr_t* mbr = mbr_new(disk);

    mbr_remove_partition(mbr, 2);

    linux_remove_partition(disk, 2);

    linux_discard(disk, start * sector_size, size * sector_size);
    linux_wipe_signatures(disk, start * sector_size, size * sector_size);

    mbr_write(mbr, disk);

    mbr_free(mbr);

#else

    gpt_t* gpt = gpt_read(disk);

    gpt_remove_partition(gpt, 2);

    linux_remove_partition(disk, 2);

    linux_discard(disk, start * sector_size, size * sector_size);
    linux_wipe_signatures(disk, start * sector_size, size * sector_size);

    gpt_write(gpt, disk);

    gpt_free(gpt);

#endif

    disk_free(disk);
}
