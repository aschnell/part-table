
// gcc -o print print.c mbr.c gpt.c disk.c utils.c -Wall -O2 -luuid -lblkid -ldevmapper -ljson-c


#include <fcntl.h>
#include <locale.h>

#include "disk.h"
#include "mbr.h"
#include "gpt.h"


int
main()
{
    setlocale(LC_ALL, "");

    disk_t* disk = disk_new(O_RDONLY);

    mbr_t* mbr = mbr_read(disk);

    mbr_print(mbr);

    mbr_free(mbr);

    gpt_t* gpt = gpt_read(disk);

    gpt_print(gpt);
    gpt_json(gpt);

    gpt_free(gpt);

    disk_free(disk);
}
