

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mbr.h"
#include "utils.h"


const uint16_t mbr_signature = 0xaa55;


const uint8_t mbr_efi_system_type_id = 0xef;
const uint8_t mbr_extended_lba_type_id = 0x0f;
const uint8_t mbr_extended_type_id = 0x05;
const uint8_t mbr_gpt_type_id = 0xee;
const uint8_t mbr_intel_rst_id = 0x84;
const uint8_t mbr_linux_lvm_type_id = 0x8e;
const uint8_t mbr_linux_native_type_id = 0x83;
const uint8_t mbr_linux_raid_type_id = 0xfd;
const uint8_t mbr_linux_swap_type_id = 0x82;
const uint8_t mbr_prep_type_id = 0x41;
const uint8_t mbr_unused_type_id = 0x00;


unsigned
chs2c(raw_chs_t raw_chs)
{
    return ((raw_chs.sector & 0xc0) << 2) + raw_chs.cylinder;
}


unsigned
chs2h(raw_chs_t raw_chs)
{
    return raw_chs.head;
}


unsigned
chs2s(raw_chs_t raw_chs)
{
    return raw_chs.sector & 0x3f;
}


void
print_chs(raw_chs_t raw_chs)
{
    printf("%d/%d/%d", chs2c(raw_chs), chs2h(raw_chs), chs2s(raw_chs));
}


static const mbr_name_type_id_mapping_t mbr_name_type_id_mapping[] =
{
    { "efi-system", mbr_efi_system_type_id },
    { "extended", mbr_extended_type_id },
    { "extended-lba", mbr_extended_lba_type_id },
    { "gpt", mbr_gpt_type_id },
    { "intel-rst", mbr_intel_rst_id },
    { "linux-lvm", mbr_linux_lvm_type_id },
    { "linux-raid", mbr_linux_raid_type_id },
    { "linux-swap", mbr_linux_swap_type_id },
    { "linux-native", mbr_linux_native_type_id },
    { "prep", mbr_prep_type_id },
};


const mbr_name_type_id_mapping_t*
mbr_find_name_type_id_mapping(uint8_t type_id)
{
    int n = sizeof(mbr_name_type_id_mapping) / sizeof(mbr_name_type_id_mapping[0]);

    for (int i = 0; i < n; ++i)
	if (type_id == mbr_name_type_id_mapping[i].type_id)
	    return &mbr_name_type_id_mapping[i];

    return NULL;
}


static mbr_partition_entry_t*
mbr_partition(mbr_t* mbr, int num)
{
    return (mbr_partition_entry_t*) &mbr->mbr->partitions[num - 1];
}


static const mbr_partition_entry_t*
mbr_const_partition(const mbr_t* mbr, int num)
{
    return (mbr_partition_entry_t*) &mbr->mbr->partitions[num - 1];
}


mbr_t*
mbr_read(disk_t* disk)
{
    printf("--- mbr read ---\n");

    mbr_t* mbr = malloc(sizeof(mbr_t));
    if (!mbr)
	error("malloc failed");

    mbr->mbr = (mbr_mbr_t*) disk_read_sectors(disk, 0, 1);

    printf("signature: 0x%04x\n", le16toh(mbr->mbr->signature));

    if (le16toh(mbr->mbr->signature) != mbr_signature)
	error("wrong mbr signature");

    for (unsigned int num = 1; num <= 4; ++num)
    {
	const mbr_partition_entry_t* partition = mbr_partition(mbr, num);

	if (partition->type_id != mbr_extended_lba_type_id &&
	    partition->type_id != mbr_extended_type_id)
	    continue;

	const uint64_t offset1 = partition->first_lba;
	uint64_t offset2 = offset1;

	uint64_t tmp1 = 0;

	for (unsigned int l = 5; ; ++l)
	{
	    mbr_ebr_t* ebr = disk_read_sectors(disk, offset2, 1);

	    if (le32toh(ebr->signature) != mbr_signature)
		error("wrong mbr ebr signature");

	    const mbr_partition_entry_t* partition = &ebr->partitions[0];

	    if (partition->type_id == 0x00) // or is it allowed to have "unused" slots?
		break;

	    printf("number: %d\n", l);
	    printf("  ebr location: %zd\n", offset2);

	    const mbr_name_type_id_mapping_t* p = mbr_find_name_type_id_mapping(partition->type_id);
	    if (p)
		printf("  type-id: 0x%02x (%s)\n", partition->type_id, p->name);
	    else
		printf("  type-id: 0x%02x\n", partition->type_id);

	    {
		uint64_t start = offset2 + le32toh(partition->first_lba);
		uint64_t size = le32toh(partition->size_lba);

		printf("  start-lba: %zu\n", start);
		printf("  end-lba: %zu\n", start + size - 1);
		printf("  size-lba: %zu\n", size);

		if (l > 5)
		{
		    uint64_t tmp2 = start + size - offset2;

		    if (tmp1 != tmp2)
			printf("warning: number of sectors in ebr for partition %d wrong, %ld != %ld\n",
			       l, tmp1, tmp2);
		}
	    }

	    {
		printf("  first-chs: ");
		print_chs(partition->first_chs);
		printf("\n");

		printf("  last-chs: ");
		print_chs(partition->last_chs);
		printf("\n");
	    }

	    const mbr_partition_entry_t* partition2 = &ebr->partitions[1];

	    printf("  second type-id: 0x%02x\n", partition2->type_id);
	    if (partition2->type_id != mbr_extended_lba_type_id &&
		partition2->type_id != mbr_extended_type_id)
	    {
		if (le32toh(partition2->first_lba) != 0 || le32toh(partition2->size_lba) != 0)
		    printf("values for non-existing logical partition not null\n");

		break;
	    }

	    offset2 = offset1 + le32toh(partition2->first_lba);

	    tmp1 = le32toh(partition2->size_lba);
	}
    }

    return mbr;
}


void
mbr_print(const mbr_t* mbr)
{
    printf("--- mbr print ---\n");

    printf("id: 0x%08x\n", le32toh(mbr->mbr->id));

    for (unsigned int num = 1; num <= 4; ++num)
    {
	const mbr_partition_entry_t* partition = mbr_const_partition(mbr, num);

	if (partition->type_id == 0x00)
	    continue;

	printf("number: %d\n", num);

	const mbr_name_type_id_mapping_t* p = mbr_find_name_type_id_mapping(partition->type_id);
	if (p)
	    printf("  type-id: 0x%02x (%s)\n", partition->type_id, p->name);
	else
	    printf("  type-id: 0x%02x\n", partition->type_id);

	{
	    uint64_t start = le32toh(partition->first_lba);
	    uint64_t size = le32toh(partition->size_lba);

	    printf("  start-lba: %zu\n", start);
	    printf("  end-lba: %zu\n", start + size - 1);
	    printf("  size-lba: %zu\n", size);
	}

	{
	    printf("  first-chs: ");
	    print_chs(partition->first_chs);
	    printf("\n");

	    printf("  last-chs: ");
	    print_chs(partition->last_chs);
	    printf("\n");
	}
    }
}


void
mbr_write(mbr_t* mbr, disk_t* disk)
{
    printf("--- mbr write ---\n");

    disk_write_sectors(disk, 0, 1, mbr->mbr);
}


void
mbr_free(mbr_t* mbr)
{
    free(mbr->mbr);

    free(mbr);
}


void
mbr_create_partition(mbr_t* mbr, int num, uint64_t start, uint64_t size)
{
    mbr_partition_entry_t* partition = mbr_partition(mbr, num);

    if (partition->type_id != 0x00)
	error("partition slot already used");

    memset(partition, 0, sizeof(mbr_partition_entry_t));

    partition->type_id = mbr_linux_native_type_id;

    partition->first_lba = htole32(start);
    partition->size_lba = htole32(size);
}


void
mbr_remove_partition(mbr_t* mbr, int num)
{
    mbr_partition_entry_t* partition = mbr_partition(mbr, num);

    if (partition->type_id == 0x00)
	error("partition slot already unused");

    memset(partition, 0, sizeof(mbr_partition_entry_t));
}
