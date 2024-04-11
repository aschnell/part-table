

#include "disk.h"


typedef struct
{
    uint8_t head;
    uint8_t sector;
    uint8_t cylinder;
} __attribute__ ((packed)) raw_chs_t;

_Static_assert(sizeof(raw_chs_t) == 3, "haha");


typedef struct
{
    uint8_t boot;
    raw_chs_t first_chs;
    uint8_t type_id;
    raw_chs_t last_chs;
    uint32_t first_lba;
    uint32_t size_lba;
} __attribute__ ((packed)) mbr_partition_entry_t;

_Static_assert(sizeof(mbr_partition_entry_t) == 16, "haha");


typedef struct
{
    uint8_t boot_code[440];
    uint32_t id;
    uint16_t null;
    mbr_partition_entry_t partitions[4];
    uint16_t signature;
} __attribute__ ((packed)) mbr_mbr_t;

_Static_assert(sizeof(mbr_mbr_t) == 512, "haha");


typedef struct
{
    uint8_t unused1[446];
    mbr_partition_entry_t partitions[2];
    uint8_t unused2[32];
    uint16_t signature;
} __attribute__ ((packed)) mbr_ebr_t;

_Static_assert(sizeof(mbr_ebr_t) == 512, "haha");


struct mbr_s
{
    mbr_mbr_t* mbr;
};

typedef struct mbr_s mbr_t;


mbr_t*
mbr_read(disk_t* disk);

void
mbr_print(const mbr_t* mbr);

void
mbr_write(mbr_t* mbr, disk_t* disk);

void
mbr_free(mbr_t* mbr);


void
mbr_create_partition(mbr_t* mbr, int num, uint64_t start, uint64_t size);

void
mbr_remove_partition(mbr_t* mbr, int num);


extern const uint16_t mbr_signature;

extern const uint8_t mbr_efi_system_type_id;
extern const uint8_t mbr_extended_lba_type_id;
extern const uint8_t mbr_extended_type_id;
extern const uint8_t mbr_gpt_type_id;
extern const uint8_t mbr_intel_rst_id;
extern const uint8_t mbr_linux_lvm_type_id;
extern const uint8_t mbr_linux_native_type_id;
extern const uint8_t mbr_linux_raid_type_id;
extern const uint8_t mbr_linux_swap_type_id;
extern const uint8_t mbr_prep_type_id;
extern const uint8_t mbr_unused_type_id;


struct mbr_name_type_id_mapping_s
{
    const char* name;
    const uint8_t type_id;
};

typedef struct mbr_name_type_id_mapping_s mbr_name_type_id_mapping_t;


const mbr_name_type_id_mapping_t*
mbr_find_name_type_id_mapping(uint8_t type_id);
