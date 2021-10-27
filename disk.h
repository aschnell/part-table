

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


typedef struct disk_s disk_t;


disk_t*
disk_new(const char* name, int flags);

void
disk_free(disk_t* disk);

void*
disk_read(const disk_t* disk, uint64_t sector, unsigned cnt);

void
disk_write(const disk_t* disk, uint64_t sector, unsigned cnt, const void* buffer);

int
disk_fd(const disk_t* disk);

size_t
disk_sectors(const disk_t* disk);

size_t
disk_sector_size(const disk_t* disk);

bool
disk_is_blk_device(const disk_t* disk);

unsigned int
disk_major(const disk_t* disk);

unsigned int
disk_minor(const disk_t* disk);
