

#include <stdint.h>
#include <stdbool.h>


typedef struct disk_s disk_t;


disk_t*
disk_new(const char* path, int flags, uint32_t sector_size);

void
disk_free(disk_t* disk);

void*
disk_read_sectors(const disk_t* disk, uint64_t sector, unsigned cnt);

void
disk_write_sectors(const disk_t* disk, uint64_t sector, unsigned cnt, const void* buffer);

void
disk_sync(disk_t* disk);


const char*
disk_path(const disk_t* disk);

int
disk_fd(const disk_t* disk);

uint64_t
disk_sectors(const disk_t* disk);


uint32_t
disk_sector_size(const disk_t* disk);

bool
disk_is_blk_device(const disk_t* disk);

unsigned int
disk_major(const disk_t* disk);

unsigned int
disk_minor(const disk_t* disk);
