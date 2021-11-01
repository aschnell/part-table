

#include <stddef.h>

#include "disk.h"



// start and size are always in bytes


void
linux_discard(disk_t* disk, uint64_t start, uint64_t size);


void
linux_wipe_signatures(disk_t* disk, uint64_t start, uint64_t size);


void
linux_create_partition(disk_t* disk, int num, uint64_t start, uint64_t size);

void
linux_resize_partition(disk_t* disk, int num, uint64_t start, uint64_t size);

void
linux_remove_partition(disk_t* disk, int num);

bool
linux_verify_partition(const disk_t* disk, int num, uint64_t start, uint64_t size);
