
// gcc test1.c disk.c mbr.c -o test1 -Wall


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "disk.h"
#include "mbr.h"


raw_chs_t
huhu(uint32_t s)
{
    raw_chs_t ret;

    if (s == 0)
    {
	ret.cylinder = 0;
	ret.head = 0;
	ret.sector = 0;
    }
    else
    {
	ret.cylinder = 1;
	ret.head = 1;
	ret.sector = s;
    }

    return ret;
}


int
main()
{
    // offsets of EBRs from start of extended partition, all placed at start of extended
    // partition

    uint32_t offset1 = 0;
    uint32_t offset2 = 1;
    uint32_t offset3 = 2;

    // MBR

    mbr_mbr_t mbr;
    memset(&mbr, 0, sizeof(mbr));
    mbr.signature = mbr_signature;
    mbr.id = 0x12345678;

    // extended partition starts at sector 2048, size 8192, end 10239

    mbr.partitions[0].type_id = mbr_extended_lba_type_id;
    mbr.partitions[0].first_lba = 2048;
    mbr.partitions[0].size_lba = 8192;
    mbr.partitions[0].first_chs = huhu(1);
    mbr.partitions[0].last_chs = huhu(2);

    // first EBR

    mbr_ebr_t ebr1;
    memset(&ebr1, 0, sizeof(ebr1));
    ebr1.signature = mbr_signature;

    // first logical partition starts at 4096, size 2048

    ebr1.partitions[0].type_id = mbr_linux_raid_type_id;
    ebr1.partitions[0].first_lba = 1 * 2048 - offset1;
    ebr1.partitions[0].size_lba = 2048;
    ebr1.partitions[0].first_chs = huhu(1);
    ebr1.partitions[0].last_chs = huhu(2);

    // link to second EBR

    ebr1.partitions[1].type_id = mbr_extended_lba_type_id;
    ebr1.partitions[1].first_lba = offset2;
    ebr1.partitions[1].size_lba = 2 * 2048 - offset1;
    ebr1.partitions[1].first_chs = huhu(1);
    ebr1.partitions[1].last_chs = huhu(2);

    // second EBR

    mbr_ebr_t ebr2;
    memset(&ebr2, 0, sizeof(ebr2));
    ebr2.signature = mbr_signature;

    // second logical partition starts at 6144, size 2048

    ebr2.partitions[0].type_id = mbr_linux_lvm_type_id;
    ebr2.partitions[0].first_lba = 2 * 2048 - offset2;
    ebr2.partitions[0].size_lba = 2048;
    ebr2.partitions[0].first_chs = huhu(1);
    ebr2.partitions[0].last_chs = huhu(2);

    // link to third EBR

    ebr2.partitions[1].type_id = mbr_extended_lba_type_id;
    ebr2.partitions[1].first_lba = offset3;
    ebr2.partitions[1].size_lba = 3 * 2048 - offset2;
    ebr2.partitions[1].first_chs = huhu(1);
    ebr2.partitions[1].last_chs = huhu(2);

    // third EBR

    mbr_ebr_t ebr3;
    memset(&ebr3, 0, sizeof(ebr3));
    ebr3.signature = mbr_signature;

    // third logical partition starts at 8192, size 2048

    ebr3.partitions[0].type_id = mbr_linux_swap_type_id;
    ebr3.partitions[0].first_lba = 3 * 2048 - offset3;
    ebr3.partitions[0].size_lba = 2048;
    ebr3.partitions[0].first_chs = huhu(1);
    ebr3.partitions[0].last_chs = huhu(2);

    // no further EBR

    ebr3.partitions[1].type_id = mbr_unused_type_id;
    ebr3.partitions[1].first_lba = 0;
    ebr3.partitions[1].size_lba = 0;
    ebr3.partitions[1].first_chs = huhu(0);
    ebr3.partitions[1].last_chs = huhu(0);

    // write MBR and EBRs

    disk_t* disk = disk_new("/dev/sdd", O_RDWR, 512);

    disk_write_sectors(disk, 0, 1, &mbr);
    disk_write_sectors(disk, 2048 + offset1, 1, &ebr1);
    disk_write_sectors(disk, 2048 + offset2, 1, &ebr2);
    disk_write_sectors(disk, 2048 + offset3, 1, &ebr3);

    disk_free(disk);
}
