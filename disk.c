

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include "disk.h"
#include "utils.h"


struct disk_s
{
    int fd;
    size_t sectors;
    bool is_blk_device;
    dev_t major_minor;
    unsigned int sector_size;
};


disk_t*
disk_new(int flags)
{
    disk_t* disk = malloc(sizeof(disk_t));
    if (!disk)
	error("malloc failed");

    disk->fd = open("/dev/ram0", flags | O_LARGEFILE | O_CLOEXEC);
    if (disk->fd < 0)
	error("open failed");

    struct stat buf;
    if (fstat(disk->fd, &buf) != 0)
	error("fstat failed");

    disk->is_blk_device = S_ISBLK(buf.st_mode);

    if (disk->is_blk_device)
    {
	disk->major_minor = buf.st_rdev;

	printf("node: %d:%d\n", major(disk->major_minor), minor(disk->major_minor));

	int sector_size = 0;	// BLKSSZGET expects int* (see kernel block/ioctl.c)
	if (ioctl(disk->fd, BLKSSZGET, &sector_size) != 0)
	    error("ioctl BLKSSZGET failed");

	uint64_t size = 0;	// BLKGETSIZE64 expects u64* (see kernel block/ioctl.c)
	if (ioctl(disk->fd, BLKGETSIZE64, &size) != 0)
	    error("ioctl BLKGETSIZE64 failed");

	disk->sector_size = sector_size;
	disk->sectors = size / sector_size;
    }
    else
    {
	printf("not a block device\n");

	disk->major_minor = 0;

	disk->sector_size = 512;
	disk->sectors = buf.st_size / disk->sector_size;
    }

    printf("sectors: %zd\n", disk->sectors);
    printf("sector-size: %d\n", disk->sector_size);

    return disk;
}


void
disk_free(disk_t* disk)
{
    close(disk->fd);
    free(disk);
}


void*
disk_read(const disk_t* disk, uint64_t sector, unsigned cnt)
{
    off_t offset = disk->sector_size * sector;
    if (lseek(disk->fd, offset, SEEK_SET) != offset)
	error("seek failed");

    size_t size = disk->sector_size * cnt;

    void* buffer = malloc(size);
    if (!buffer)
	error("malloc failed");

    if (read(disk->fd, buffer, size) != (ssize_t)(size))
	error("read failed");

    return buffer;
}


void
disk_write(const disk_t* disk, uint64_t sector, unsigned cnt, const void* buffer)
{
    off_t offset = disk->sector_size * sector;
    if (lseek(disk->fd, offset, SEEK_SET) != offset)
	error("seek failed");

    size_t size = disk->sector_size * cnt;
    if (write(disk->fd, buffer, size) != (ssize_t)(size))
	error("write failed");
}


int
disk_fd(const disk_t* disk)
{
    return disk->fd;
}


size_t
disk_sectors(const disk_t* disk)
{
    return disk->sectors;
}


size_t
disk_sector_size(const disk_t* disk)
{
    return disk->sector_size;
}


bool
disk_is_blk_device(const disk_t* disk)
{
    return disk->is_blk_device;
}


unsigned int
disk_major(const disk_t* disk)
{
    return major(disk->major_minor);
}


unsigned int
disk_minor(const disk_t* disk)
{
    return minor(disk->major_minor);
}
