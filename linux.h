

#include <stddef.h>

#include "disk.h"


void
linux_discard(disk_t* disk, size_t start, size_t length);


void
linux_wipe_signatures(disk_t* disk, size_t start, size_t length);


void
linux_create_partition(disk_t* disk, int num, size_t start, size_t length);

void
linux_resize_partition(disk_t* disk, int num, size_t start, size_t length);

void
linux_remove_partition(disk_t* disk, int num);
