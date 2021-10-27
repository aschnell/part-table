

#include "disk.h"


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


extern const uint8_t mbr_efi_system_type_id;
extern const uint8_t mbr_extended_type_id;
extern const uint8_t mbr_extended_lba_type_id;
extern const uint8_t mbr_gpt_type_id;
extern const uint8_t mbr_intel_rst_id;
extern const uint8_t mbr_linux_lvm_type_id;
extern const uint8_t mbr_linux_native_type_id;
extern const uint8_t mbr_linux_raid_type_id;
extern const uint8_t mbr_linux_swap_type_id;
extern const uint8_t mbr_prep_type_id;


struct mbr_name_type_id_mapping_s
{
    const char* name;
    const uint8_t type_id;
};

typedef struct mbr_name_type_id_mapping_s mbr_name_type_id_mapping_t;


const mbr_name_type_id_mapping_t*
mbr_find_name_type_id_mapping(uint8_t type_id);
