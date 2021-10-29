

#include <stdint.h>
#include <uuid/uuid.h>

#include "disk.h"


typedef struct gpt_s gpt_t;


gpt_t*
gpt_create(disk_t* disk, int partition_entries, int partition_entry_size);

void
gpt_set_guid(gpt_t* gpt, const uuid_t guid);

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


uint64_t
gpt_get_start(const gpt_t* gpt, int num);

uint64_t
gpt_get_size(const gpt_t* gpt, int num);

void
gpt_set_size(gpt_t* gpt, int num, uint64_t size);

void
gpt_set_type_guid(gpt_t* gpt, int num, const uuid_t type_guid);

void
gpt_set_partition_guid(gpt_t* gpt, int num, const uuid_t guid);

void
gpt_set_attributes(gpt_t* gpt, int num, uint64_t attributes);

void
gpt_set_name(gpt_t* gpt, int num, const char* name);


extern const uuid_t gpt_bios_boot_type_guid;
extern const uuid_t gpt_efi_system_type_guid;
extern const uuid_t gpt_intel_rst_type_guid;
extern const uuid_t gpt_linux_data_type_guid;
extern const uuid_t gpt_linux_home_type_guid;
extern const uuid_t gpt_linux_lvm_type_guid;
extern const uuid_t gpt_linux_raid_type_guid;
extern const uuid_t gpt_linux_swap_type_guid;
extern const uuid_t gpt_prep_type_guid;
extern const uuid_t gpt_windows_data_type_guid;
extern const uuid_t gpt_windows_recovery_type_guid;


struct gpt_name_type_guid_mapping_s
{
    const char* name;
    const uuid_t* type_guid;
};

typedef struct gpt_name_type_guid_mapping_s gpt_name_type_guid_mapping_t;


const gpt_name_type_guid_mapping_t*
gpt_find_name_type_guid_mapping_by_type_guid(const uuid_t type_guid);

const gpt_name_type_guid_mapping_t*
gpt_find_name_type_guid_mapping_by_name(const char* name);


struct gpt_name_attribute_mapping_s
{
    const char* name;
    const int bit;
};

typedef struct gpt_name_attribute_mapping_s gpt_name_attribute_mapping_t;


const gpt_name_attribute_mapping_t*
gpt_find_name_attribute_mapping_by_n(int bit);

const gpt_name_attribute_mapping_t*
gpt_find_name_attribute_by_name(const char* name);
