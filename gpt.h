

#include <stdint.h>
#include <uuid/uuid.h>

#include "disk.h"


typedef struct gpt_s gpt_t;


gpt_t*
gpt_create(disk_t* disk);

gpt_t*
gpt_read(disk_t* disk);

void
gpt_print(const gpt_t* gpt);

void
gpt_json(const gpt_t* gpt);

void
gpt_write(gpt_t* gpt, disk_t* disk);

void
gpt_free(gpt_t* gpt);


void
gpt_create_partition(gpt_t* gpt, int num, uint64_t start, uint64_t size);

void
gpt_remove_partition(gpt_t* gpt, int num);


extern const uuid_t gpt_bios_boot_type_guid;
extern const uuid_t gpt_efi_system_type_guid;
extern const uuid_t gpt_linux_data_type_guid;
extern const uuid_t gpt_linux_home_type_guid;
extern const uuid_t gpt_linux_lvm_type_guid;
extern const uuid_t gpt_linux_raid_type_guid;
extern const uuid_t gpt_prep_type_guid;


struct gpt_name_type_guid_mapping_s
{
    const char* name;
    const uuid_t* type_guid;
};

typedef struct gpt_name_type_guid_mapping_s gpt_name_type_guid_mapping_t;


const gpt_name_type_guid_mapping_t*
gpt_find_name_type_guid_mapping(const uuid_t type_guid);
