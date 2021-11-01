

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
    char* path;
    int fd;
    uint64_t sectors;
    bool is_blk_device;
    dev_t major_minor;
    uint32_t sector_size;
};


disk_t*
disk_new(const char* path, int flags, uint32_t sector_size)
{
    disk_t* disk = malloc(sizeof(disk_t));
    if (!disk)
	error("malloc failed");

    disk->path = realpath(path, NULL);
    if (!disk->path)
	error("realpath failed");

    disk->fd = open(disk->path, flags | O_LARGEFILE | O_CLOEXEC);
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

	disk->sector_size = sector_size;
	disk->sectors = buf.st_size / disk->sector_size;
    }

    printf("sectors: %lu\n", disk->sectors);
    printf("sector-size: %u\n", disk->sector_size);

    return disk;
}


void
disk_free(disk_t* disk)
{
    close(disk->fd);
    free(disk->path);
    free(disk);
}


void*
disk_read_sectors(const disk_t* disk, uint64_t sector, unsigned cnt)
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
disk_write_sectors(const disk_t* disk, uint64_t sector, unsigned cnt, const void* buffer)
{
    off_t offset = disk->sector_size * sector;
    if (lseek(disk->fd, offset, SEEK_SET) != offset)
	error("seek failed");

    size_t size = disk->sector_size * cnt;
    if (write(disk->fd, buffer, size) != (ssize_t)(size))
	error("write failed");
}


void
disk_sync(disk_t* disk)
{
    // TODO: We want the partition table to be written to disk. But fsync might be slow
    // and sync also all file systems.
    // or ioctl(fd, BLKFLSBUF)?

    fsync(disk->fd);

    sync();
}


const char*
disk_path(const disk_t* disk)
{
    return disk->path;
}


int
disk_fd(const disk_t* disk)
{
    return disk->fd;
}


uint64_t
disk_sectors(const disk_t* disk)
{
    return disk->sectors;
}


uint32_t
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
