

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>
#include <iconv.h>
#include <langinfo.h>
#include <json-c/json.h>

#include "gpt.h"
#include "mbr.h"
#include "utils.h"
#include "linux.h"


static const uint64_t gpt_signature = 0x5452415020494645;  // reverse "EFI PART"

static const uint32_t gpt_header_revision_v1_00 = 0x00010000;


const uuid_t gpt_bios_boot_type_guid = { 0x21, 0x68, 0x61, 0x48, 0x64, 0x49, 0x6e, 0x6f,
    0x74, 0x4e, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49 };
const uuid_t gpt_efi_system_type_guid = { 0xc1, 0x2a, 0x73, 0x28, 0xf8, 0x1f, 0x11, 0xd2,
    0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b };
const uuid_t gpt_intel_rst_type_guid = { 0xd3, 0xbf, 0xe2, 0xde, 0x3d, 0xaf, 0x11, 0xdf,
    0xba, 0x40, 0xe3, 0xa5, 0x56, 0xd8, 0x95, 0x93 };
const uuid_t gpt_linux_data_type_guid = { 0x0f, 0xc6, 0x3d, 0xaf, 0x84, 0x83, 0x47, 0x72,
    0x8e, 0x79, 0x3d, 0x69, 0xd8, 0x47, 0x7d, 0xe4 };
const uuid_t gpt_linux_home_type_guid = { 0x93, 0x3a, 0xc7, 0xe1, 0x2e, 0xb4, 0x4f, 0x13,
    0xb8, 0x44, 0x0e, 0x14, 0xe2, 0xae, 0xf9, 0x15 };
const uuid_t gpt_linux_lvm_type_guid = { 0xe6, 0xd6, 0xd3, 0x79, 0xf5, 0x07, 0x44, 0xc2,
    0xa2, 0x3c, 0x23, 0x8f, 0x2a, 0x3d, 0xf9, 0x28 };
const uuid_t gpt_linux_raid_type_guid = { 0xa1, 0x9d, 0x88, 0x0f, 0x05, 0xfc, 0x4d, 0x3b,
    0xa0, 0x06, 0x74, 0x3f, 0x0f, 0x84, 0x91, 0x1e };
const uuid_t gpt_linux_swap_type_guid = { 0x06, 0x57, 0xfd, 0x6d, 0xa4, 0xab, 0x43, 0xc4,
    0x84, 0xe5, 0x09, 0x33, 0xc8, 0x4b, 0x4f, 0x4f };
const uuid_t gpt_prep_type_guid = { 0x9e, 0x1a, 0x2d, 0x38, 0xc6, 0x12, 0x43, 0x16,
    0xaa, 0x26, 0x8b, 0x49, 0x52, 0x1e, 0x5a, 0x8b };
const uuid_t gpt_windows_data_type_guid = { 0xeb, 0xd0, 0xa0, 0xa2, 0xb9, 0xe5, 0x44, 0x33,
    0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7 };
const uuid_t gpt_windows_recovery_type_guid = { 0xde, 0x94, 0xbb, 0xa4, 0x06, 0xd1, 0x4d, 0x40,
    0xa1, 0x6a, 0xbf, 0xd5, 0x01, 0x79, 0xd6, 0xac };


typedef struct
{
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_lba;
    uint64_t last_lba;
    uuid_t disk_guid;
    uint64_t partition_lba;
    uint32_t partition_entries;
    uint32_t partition_entry_size;
    uint32_t partition_crc;
} __attribute__ ((packed)) gpt_header_t;

_Static_assert(sizeof(gpt_header_t) == 92, "haha");


typedef struct
{
    uuid_t type_guid;
    uuid_t partition_guid;
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t attributes;
    uint16_t name[36];
} gpt_partition_entry_t;

_Static_assert(sizeof(gpt_partition_entry_t) == 128, "haha");


struct gpt_s
{
    mbr_mbr_t* pmbr;

    gpt_header_t* header1;
    gpt_header_t* header2;

    gpt_partition_entry_t* partitions;

    uint32_t header_size;

    uint32_t partition_entries;
    uint32_t partition_entry_size;
};


static const gpt_name_type_guid_mapping_t gpt_name_type_guid_mapping[] =
{
    { "bios-boot", &gpt_bios_boot_type_guid },
    { "efi-system", &gpt_efi_system_type_guid },
    { "intel-rst", &gpt_intel_rst_type_guid },
    { "linux-data", &gpt_linux_data_type_guid },
    { "linux-home", &gpt_linux_home_type_guid },
    { "linux-lvm", &gpt_linux_lvm_type_guid },
    { "linux-raid", &gpt_linux_raid_type_guid },
    { "linux-swap", &gpt_linux_swap_type_guid },
    { "prep", &gpt_prep_type_guid },
    { "windows-data", &gpt_windows_data_type_guid },
    { "windows-recovery", &gpt_windows_recovery_type_guid },
};


const gpt_name_type_guid_mapping_t*
gpt_find_name_type_guid_mapping_by_type_guid(const uuid_t type_guid)
{
    int n = sizeof(gpt_name_type_guid_mapping) / sizeof(gpt_name_type_guid_mapping[0]);

    for (int i = 0; i < n; ++i)
	if (uuid_compare(type_guid, *(gpt_name_type_guid_mapping[i].type_guid)) == 0)
	    return &gpt_name_type_guid_mapping[i];

    return NULL;
}


const gpt_name_type_guid_mapping_t*
gpt_find_name_type_guid_mapping_by_name(const char* name)
{
    int n = sizeof(gpt_name_type_guid_mapping) / sizeof(gpt_name_type_guid_mapping[0]);

    for (int i = 0; i < n; ++i)
	if (strcmp(name, gpt_name_type_guid_mapping[i].name) == 0)
	    return &gpt_name_type_guid_mapping[i];

    return NULL;
}


static const gpt_name_attribute_mapping_t gpt_name_attribute_mapping[] =
{
    { "required", 0 },
    { "no-block-io", 1 },
    { "legacy-boot", 2 },
};


const gpt_name_attribute_mapping_t*
gpt_find_name_attribute_by_bit(int bit)
{
    int n = sizeof(gpt_name_attribute_mapping) / sizeof(gpt_name_attribute_mapping[0]);

    for (int i = 0; i < n; ++i)
	if (bit == gpt_name_attribute_mapping[i].bit)
	    return &gpt_name_attribute_mapping[i];

    return NULL;
}


const gpt_name_attribute_mapping_t*
gpt_find_name_attribute_by_name(const char* name)
{
    int n = sizeof(gpt_name_attribute_mapping) / sizeof(gpt_name_attribute_mapping[0]);

    for (int i = 0; i < n; ++i)
	if (strcmp(name, gpt_name_attribute_mapping[i].name) == 0)
	    return &gpt_name_attribute_mapping[i];

    return NULL;
}


static gpt_partition_entry_t*
gpt_partition(gpt_t* gpt, int num)
{
    return (gpt_partition_entry_t*) &gpt->partitions[num - 1];
}


static const gpt_partition_entry_t*
gpt_const_partition(const gpt_t* gpt, int num)
{
    return (const gpt_partition_entry_t*) &gpt->partitions[num - 1];
}


static void
swap_uuid(uuid_t guid)
{
    uint8_t tmp1 = guid[0]; guid[0] = guid[3]; guid[3] = tmp1;
    uint8_t tmp2 = guid[1]; guid[1] = guid[2]; guid[2] = tmp2;

    uint8_t tmp3 = guid[4]; guid[4] = guid[5]; guid[5] = tmp3;

    uint8_t tmp4 = guid[6]; guid[6] = guid[7]; guid[7] = tmp4;
}


static char*
uuid_decode(const uuid_t guid)
{
    char* buffer = malloc(UUID_STR_LEN);
    if (!buffer)
	error("malloc failed");

    uuid_unparse_lower(guid, buffer);
    return buffer;
}


mbr_mbr_t*
gpt_create_pmbr(disk_t* disk)
{
    mbr_mbr_t* pmbr = (mbr_mbr_t*) malloc(disk_sector_size(disk));
    if (!pmbr)
	error("malloc failed");

    memset(pmbr, 0, disk_sector_size(disk));

    pmbr->partitions[0].type_id = mbr_gpt_type_id;

    pmbr->partitions[0].first_chs.cylinder = 0;
    pmbr->partitions[0].first_chs.head = 0;
    pmbr->partitions[0].first_chs.sector = 2;

    pmbr->partitions[0].last_chs.cylinder = 0xff;
    pmbr->partitions[0].last_chs.head = 0xff;
    pmbr->partitions[0].last_chs.sector = 0xff;

    pmbr->partitions[0].first_lba = htole32(1);

    if (disk_sectors(disk) - 1> 0xffffffff)
	pmbr->partitions[0].size_lba = htole32(0xffffffff);
    else
	pmbr->partitions[0].size_lba = htole32(disk_sectors(disk) - 1);

    pmbr->signature = htole16(mbr_signature);

    return pmbr;
}


gpt_t*
gpt_create(disk_t* disk, int partition_entries, int partition_entry_size)
{
    gpt_t* gpt = malloc(sizeof(gpt_t));
    if (!gpt)
	error("malloc failed");

    gpt->header_size = sizeof(gpt_header_t);

    gpt->partition_entries = partition_entries;
    gpt->partition_entry_size = partition_entry_size;

    size_t partition_size = gpt->partition_entries * gpt->partition_entry_size;
    size_t partition_blocks = (partition_size + disk_sector_size(disk) - 1) / disk_sector_size(disk);

    size_t sectors = disk_sectors(disk);

    gpt->pmbr = gpt_create_pmbr(disk);

    gpt->header1 = (gpt_header_t*) malloc(disk_sector_size(disk));
    if (!gpt->header1)
	error("malloc failed");

    memset(gpt->header1, 0, disk_sector_size(disk));

    gpt->header1->signature = htole64(gpt_signature);
    gpt->header1->revision = htole32(gpt_header_revision_v1_00);
    gpt->header1->header_size = htole32(gpt->header_size);

    gpt->header1->first_lba = htole64(2 + partition_blocks);
    gpt->header1->last_lba = htole64(sectors - 2 - partition_blocks);

    gpt->header1->partition_entries = htole32(gpt->partition_entries);
    gpt->header1->partition_entry_size = htole32(gpt->partition_entry_size);

    uuid_generate(gpt->header1->disk_guid);
    swap_uuid(gpt->header1->disk_guid);

    gpt->header2 = (gpt_header_t*) malloc(disk_sector_size(disk));
    if (!gpt->header2)
	error("malloc failed");

    memcpy(gpt->header2, gpt->header1, disk_sector_size(disk));

    gpt->header1->current_lba = gpt->header2->backup_lba = htole64(1);
    gpt->header1->backup_lba = gpt->header2->current_lba = htole64(sectors - 1);

    gpt->header1->partition_lba = htole64(2);
    gpt->header2->partition_lba = htole64(sectors - 1 - partition_blocks);

    gpt->partitions = (gpt_partition_entry_t*) malloc(partition_size);
    if (!gpt->partitions)
	error("malloc failed");

    memset(gpt->partitions, 0, partition_size);

    return gpt;
}


void
gpt_set_guid(gpt_t* gpt, const uuid_t guid)
{
    uuid_copy(gpt->header1->disk_guid, guid);
    swap_uuid(gpt->header1->disk_guid);

    uuid_copy(gpt->header2->disk_guid, gpt->header1->disk_guid);
}


gpt_t*
gpt_read(disk_t* disk)
{
    printf("--- gpt read ---\n");

    printf("path: %s\n", disk_path(disk));

    gpt_t* gpt = malloc(sizeof(gpt_t));
    if (!gpt)
	error("malloc failed");

    gpt->pmbr = (mbr_mbr_t*) disk_read_sectors(disk, 0, 1);

    gpt->header1 = (gpt_header_t*) disk_read_sectors(disk, 1, 1);

    gpt->header2 = (gpt_header_t*) disk_read_sectors(disk, le64toh(gpt->header1->backup_lba), 1);

    printf("signature: 0x%016zx\n", le64toh(gpt->header1->signature));

    if (le64toh(gpt->header1->signature) != gpt_signature)
	error("wrong gpt signature");

    printf("revision: 0x%08x\n", le32toh(gpt->header1->revision));

    gpt->header_size = le32toh(gpt->header1->header_size);

    gpt->partition_entries = le32toh(gpt->header1->partition_entries);
    gpt->partition_entry_size = le32toh(gpt->header1->partition_entry_size);

    size_t partition_size = gpt->partition_entries * gpt->partition_entry_size;
    size_t partition_blocks = (partition_size + disk_sector_size(disk) - 1) / disk_sector_size(disk);

    gpt->partitions = (gpt_partition_entry_t*) disk_read_sectors(disk, le64toh(gpt->header1->partition_lba),
								 partition_blocks);

    for (unsigned int num = 1; num <= gpt->partition_entries; ++num)
    {
	const gpt_partition_entry_t* partition = gpt_const_partition(gpt, num);

	if (uuid_is_null(partition->type_guid))
	    continue;

	printf("number: %d\n", num);

	uint64_t start = le64toh(partition->first_lba);
	uint64_t end = le64toh(partition->last_lba);
	uint64_t size = end - start + 1;

	linux_verify_partition(disk, num, start * disk_sector_size(disk), size * disk_sector_size(disk));
    }

    return gpt;
}


char*
gpt_get_name(const gpt_partition_entry_t* partition)
{
    iconv_t cd = iconv_open(nl_langinfo(CODESET), "UCS-2LE");
    if (cd == (iconv_t) -1)
	error("iconv_open failed");

    char* inbuf = (char*) &partition->name;
    size_t inbuf_size = 2 * 36;

    char buffer[256];
    char* outbuf = buffer;
    size_t outbuf_size = sizeof(buffer);

    if (iconv(cd, &inbuf, &inbuf_size, &outbuf, &outbuf_size) == (size_t) -1)
	error("iconv failed");

    *outbuf = 0;

    iconv_close(cd);

    return strdup(buffer);
}


void
gpt_set_name(gpt_t* gpt, int num, const char* name)
{
    gpt_partition_entry_t* partition = gpt_partition(gpt, num);

    if (uuid_is_null(partition->type_guid))
	error("partition slot unused");

    memset(partition->name, 0, 2 * 36);

    iconv_t cd = iconv_open("UCS-2LE", nl_langinfo(CODESET));
    if (cd == (iconv_t) -1)
	error("iconv_open failed");

    char* inbuf = strdup(name);	// TODO
    size_t inbuf_size = strlen(name);

    char* outbuf = (char*) &partition->name;
    size_t outbuf_size = 2 * 36;

    if (iconv(cd, &inbuf, &inbuf_size, &outbuf, &outbuf_size) == (size_t) -1)
	error("iconv failed");

    iconv_close(cd);
}


void
gpt_print(const gpt_t* gpt)
{
    printf("--- gpt print ---\n");

    printf("header-size: %d\n", gpt->header_size);

    {
	uuid_t tmp1;
	uuid_copy(tmp1, gpt->header1->disk_guid);
	swap_uuid(tmp1);

	char* tmp2 = uuid_decode(tmp1);
	printf("guid: %s\n", tmp2);
	free(tmp2);
    }

    {
	uint64_t start = le64toh(gpt->header1->first_lba);
	uint64_t end = le64toh(gpt->header1->last_lba);

	printf("first-lba: %zd\n", start);
	printf("last-lba: %zd\n", end);
	printf("size-lba: %zu\n", end - start + 1);
    }

    printf("partition-entries: %d\n", gpt->partition_entries);
    printf("partition-entry-size: %d\n", gpt->partition_entry_size);

    for (unsigned int num = 1; num <= gpt->partition_entries; ++num)
    {
	const gpt_partition_entry_t* partition = gpt_const_partition(gpt, num);

	if (uuid_is_null(partition->type_guid))
	    continue;

	printf("number: %d\n", num);

	{
	    uint64_t start = le64toh(partition->first_lba);
	    uint64_t end = le64toh(partition->last_lba);

	    printf("  start-lba: %zu\n", start);
	    printf("  end-lba: %zu\n", end);
	    printf("  size-lba: %zu\n", end - start + 1);
	}

	{
	    uuid_t tmp1;
	    uuid_copy(tmp1, partition->type_guid);
	    swap_uuid(tmp1);

	    char* tmp2 = uuid_decode(tmp1);

	    const gpt_name_type_guid_mapping_t* p = gpt_find_name_type_guid_mapping_by_type_guid(tmp1);
	    if (p)
		printf("  type-guid: %s (%s)\n", tmp2, p->name);
	    else
		printf("  type-guid: %s\n", tmp2);

	    free(tmp2);
	}

	{
	    uuid_t tmp1;
	    uuid_copy(tmp1, partition->partition_guid);
	    swap_uuid(tmp1);

	    char* tmp2 = uuid_decode(tmp1);
	    printf("  guid: %s\n", tmp2);
	    free(tmp2);
	}

	{
	    uint64_t attributes = le64toh(partition->attributes);

	    printf("  attributes: 0x%016lx", attributes);

	    int first = 1;

	    for (int bit = 0; bit < 64; ++bit)
	    {
		if (attributes & (1ULL << bit))
		{
		    const gpt_name_attribute_mapping_t* p = gpt_find_name_attribute_by_bit(bit);
		    if (p)
		    {
			if (first)
			    printf(" (%s", p->name);
			else
			    printf(", %s", p->name);
			first = 0;
		    }
		}
	    }

	    if (!first)
		printf(")\n");
	    else
		printf("\n");
	}

	{
	    char* tmp = gpt_get_name(partition);
	    printf("  name: %s\n", tmp);
	    free(tmp);
	}
    }
}


void
gpt_json(const gpt_t* gpt)
{
    json_object* json_root = json_object_new_object();

    json_object* json_header = json_object_new_object();

    json_object_object_add(json_header, "type", json_object_new_string("gpt"));

    {
	uuid_t tmp1;
	uuid_copy(tmp1, gpt->header1->disk_guid);
	swap_uuid(tmp1);

	char* tmp2 = uuid_decode(tmp1);
	json_object_object_add(json_header, "guid", json_object_new_string(tmp2));

	free(tmp2);
    }

    json_object_object_add(json_header, "partition-entries", json_object_new_uint64(gpt->partition_entries));
    json_object_object_add(json_header, "partition-entry-size", json_object_new_uint64(le64toh(gpt->partition_entry_size)));

    json_object_object_add(json_header, "first-lba", json_object_new_uint64(le64toh(gpt->header1->first_lba)));
    json_object_object_add(json_header, "last-lba", json_object_new_uint64(le64toh(gpt->header1->last_lba)));

    json_object* json_partitions = json_object_new_array();

    for (unsigned int num = 1; num <= gpt->partition_entries; ++num)
    {
	const gpt_partition_entry_t* partition = gpt_const_partition(gpt, num);

	if (uuid_is_null(partition->type_guid))
	    continue;

	json_object* json_partition = json_object_new_object();

	json_object_object_add(json_partition, "number", json_object_new_int(num));

	json_object_object_add(json_partition, "start", json_object_new_uint64(le64toh(partition->first_lba)));
	json_object_object_add(json_partition, "end", json_object_new_uint64(le64toh(partition->last_lba)));

	{
	    uuid_t tmp1;
	    uuid_copy(tmp1, partition->type_guid);
	    swap_uuid(tmp1);

	    char* tmp2 = uuid_decode(tmp1);
	    json_object_object_add(json_partition, "type-guid", json_object_new_string(tmp2));

	    free(tmp2);
	}

	{
	    uuid_t tmp1;
	    uuid_copy(tmp1, partition->partition_guid);
	    swap_uuid(tmp1);

	    char* tmp2 = uuid_decode(tmp1);
	    json_object_object_add(json_partition, "guid", json_object_new_string(tmp2));

	    free(tmp2);
	}

	{
	    char buffer[64];
	    snprintf(buffer, 64, "0x%016lx", le64toh(partition->attributes));
	    json_object_object_add(json_partition, "attributes", json_object_new_string(buffer));
	}

	{
	    char* tmp = gpt_get_name(partition);
	    if (strlen(tmp) != 0)
		json_object_object_add(json_partition, "name", json_object_new_string(tmp));
	    free(tmp);
	}

	json_object_array_add(json_partitions, json_partition);
    }

    json_object_object_add(json_header, "partitions", json_partitions);

    json_object_object_add(json_root, "partition-table", json_header);

    printf("%s\n", json_object_to_json_string_ext(json_root, JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_SPACED |
						  JSON_C_TO_STRING_NOSLASHESCAPE));

    json_object_put(json_root);
}


void
gpt_write(gpt_t* gpt, disk_t* disk)
{
    printf("--- gpt write ---\n");

    size_t partition_size = gpt->partition_entries * gpt->partition_entry_size;

    uint32_t partition_crc = chksum_crc32(gpt->partitions, partition_size);
    gpt->header1->partition_crc = htole32(partition_crc);
    gpt->header2->partition_crc = htole32(partition_crc);

    gpt->header1->header_crc = 0;
    uint32_t header1_crc = chksum_crc32(gpt->header1, gpt->header_size);
    gpt->header1->header_crc = htole32(header1_crc);

    gpt->header2->header_crc = 0;
    uint32_t header2_crc = chksum_crc32(gpt->header2, gpt->header_size);
    gpt->header2->header_crc = htole32(header2_crc);

    size_t partition_blocks = (partition_size + disk_sector_size(disk) - 1) / disk_sector_size(disk);

    disk_write_sectors(disk, 0, 1, gpt->pmbr);

    disk_write_sectors(disk, le64toh(gpt->header1->current_lba), 1, gpt->header1);
    disk_write_sectors(disk, le64toh(gpt->header1->partition_lba), partition_blocks, gpt->partitions);

    disk_write_sectors(disk, le64toh(gpt->header2->current_lba), 1, gpt->header2);
    disk_write_sectors(disk, le64toh(gpt->header2->partition_lba), partition_blocks, gpt->partitions);

    disk_sync(disk);
}


void
gpt_free(gpt_t* gpt)
{
    free(gpt->header1);
    free(gpt->header2);

    free(gpt->partitions);

    free(gpt);
}


void
gpt_create_partition(gpt_t* gpt, int num, uint64_t start, uint64_t size)
{
    gpt_partition_entry_t* partition = gpt_partition(gpt, num);

    if (!uuid_is_null(partition->type_guid))
	error("partition slot already used");

    uuid_copy(partition->type_guid, gpt_linux_data_type_guid);
    swap_uuid(partition->type_guid);

    uuid_generate(partition->partition_guid);
    swap_uuid(partition->partition_guid);

    partition->first_lba = htole64(start);
    partition->last_lba = htole64(start + size - 1);
}


void
gpt_remove_partition(gpt_t* gpt, int num)
{
    gpt_partition_entry_t* partition = gpt_partition(gpt, num);

    if (uuid_is_null(partition->type_guid))
	error("partition slot already unused");

    memset(partition, 0, gpt->partition_entry_size);
}


uint64_t
gpt_get_start(const gpt_t* gpt, int num)
{
    const gpt_partition_entry_t* partition = gpt_const_partition(gpt, num);

    uint64_t start = le64toh(partition->first_lba);

    return start;
}


uint64_t
gpt_get_size(const gpt_t* gpt, int num)
{
    const gpt_partition_entry_t* partition = gpt_const_partition(gpt, num);

    uint64_t start = le64toh(partition->first_lba);
    uint64_t end = le64toh(partition->last_lba);

    return end - start + 1;
}


void
gpt_set_size(gpt_t* gpt, int num, uint64_t size)
{
    gpt_partition_entry_t* partition = gpt_partition(gpt, num);

    uint64_t start = le64toh(partition->first_lba);
    uint64_t end = start + size - 1;

    partition->last_lba = htole64(end);
}


void
gpt_set_type_guid(gpt_t* gpt, int num, const uuid_t type_guid)
{
    gpt_partition_entry_t* partition = gpt_partition(gpt, num);

    if (uuid_is_null(partition->type_guid))
	error("partition slot unused");

    uuid_copy(partition->type_guid, type_guid);
    swap_uuid(partition->type_guid);
}


void
gpt_set_partition_guid(gpt_t* gpt, int num, const uuid_t guid)
{
    gpt_partition_entry_t* partition = gpt_partition(gpt, num);

    if (uuid_is_null(partition->type_guid))
	error("partition slot unused");

    uuid_copy(partition->partition_guid, guid);
    swap_uuid(partition->partition_guid);
}


void
gpt_set_attributes(gpt_t* gpt, int num, uint64_t attributes)
{
    gpt_partition_entry_t* partition = gpt_partition(gpt, num);

    if (uuid_is_null(partition->type_guid))
	error("partition slot unused");

    partition->attributes = attributes;
}
