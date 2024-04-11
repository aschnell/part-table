

#define _GNU_SOURCE


#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <ctype.h>
#include <libdevmapper.h>
#include <linux/blkpg.h>
#include <linux/fs.h>
#include <blkid/blkid.h>

#include "linux.h"
#include "disk.h"
#include "utils.h"


enum Tech { TECH_NONE, TECH_BLKPG, TECH_DM };


const uint32_t sysfs_sector_size = 512;
const uint32_t dm_sector_size = 512;


void
linux_discard(disk_t* disk, uint64_t start, uint64_t size)
{
    printf("--- linux-discard ---\n");

    uint64_t range[2] = { start, size };

    if (ioctl(disk_fd(disk), BLKDISCARD, &range) != 0)
	fprintf(stderr, "discard failed\n");
}


void
linux_wipe_signatures(disk_t* disk, uint64_t start, uint64_t size)
{
    printf("--- linux-wipe-signatures ---\n");

    blkid_probe pr = blkid_new_probe();
    if (!pr)
	error("blkid_new_probe failed");

    if (blkid_probe_set_device(pr, disk_fd(disk), start, size) != 0)
	error("blkid_probe_set_device failed");

    if (blkid_probe_set_sectorsize(pr, disk_sector_size(disk)) != 0)
	error("blkid_probe_set_sectorsize failed");

    if (blkid_probe_enable_superblocks(pr, 1) != 0)
	error("blkid_probe_enable_superblocks failed");

    // BLKID_SUBLKS_MAGIC seems to be required for blkid_do_wipe to work
    if (blkid_probe_set_superblocks_flags(pr, BLKID_SUBLKS_MAGIC | BLKID_SUBLKS_TYPE |
					  BLKID_SUBLKS_USAGE | BLKID_SUBLKS_LABEL |
					  BLKID_SUBLKS_UUID ) != 0)
	error("blkid_probe_set_superblocks_flags failed");

    while (blkid_do_probe(pr) == 0)
    {
	printf("wipe\n");

	int n = blkid_probe_numof_values(pr);
	for (int i = 0; i < n; ++i)
	{
	    const char* name;
	    const char* data;

	    if (blkid_probe_get_value(pr, i, &name, &data, NULL) == 0)
		printf("  %s: %s\n", name, data);
	}

	if (blkid_do_wipe(pr, 0) != 0)
	    error("blkid_do_wipe failed");
    }

    blkid_free_probe(pr);
}


static int
blkpg_ioctl(int fd, struct blkpg_partition* partition, int op)
{
    struct blkpg_ioctl_arg ioctl_arg;
    memset(&ioctl_arg, 0, sizeof(ioctl_arg));
    ioctl_arg.op = op;
    ioctl_arg.flags = 0;
    ioctl_arg.datalen = sizeof(struct blkpg_partition);
    ioctl_arg.data = (void*) partition;

    return ioctl(fd, BLKPG, &ioctl_arg);
}


static void
blkpg_create_partition(disk_t* disk, int num, uint64_t start, uint64_t size)
{
    struct blkpg_partition partition;
    memset(&partition, 0, sizeof(partition));
    partition.pno = num;
    partition.start = start;
    partition.length = size;

    if (blkpg_ioctl(disk_fd(disk), &partition, BLKPG_ADD_PARTITION) != 0)
	error_with_errno("ioctl BLKPG_ADD_PARTITION failed", errno);
}


static void
blkpg_resize_partition(disk_t* disk, int num, uint64_t start, uint64_t size)
{
    struct blkpg_partition partition;
    memset(&partition, 0, sizeof(partition));
    partition.pno = num;
    partition.start = start;
    partition.length = size;

    if (blkpg_ioctl(disk_fd(disk), &partition, BLKPG_RESIZE_PARTITION) != 0)
	error_with_errno("ioctl BLKPG_RESIZE_PARTITION failed", errno);
}


static void
blkpg_remove_partition(disk_t* disk, int num)
{
    struct blkpg_partition partition;
    memset(&partition, 0, sizeof(partition));
    partition.pno = num;

    if (blkpg_ioctl(disk_fd(disk), &partition, BLKPG_DEL_PARTITION) != 0)
	error_with_errno("ioctl BLKPG_DEL_PARTITION failed", errno);
}



static bool
read_sysfs(const char* name, uint64_t* val)
{
    FILE* fp = fopen(name, "r");
    if (!fp)
    {
	error("failed to read sysfs\n");
	return false;
    }

    if (fscanf(fp, "%lu", val) != 1)
    {
	fclose(fp);
	error("failed to parse sysfs\n");
	return false;
    }

    fclose(fp);

    return true;
}


static bool
huhu_verify_partition(const disk_t* disk, int num, uint64_t start, uint64_t size)
{
    const char* path = disk_path(disk);

    char* b = basename(path);

    int l = strlen(b);

    bool need_p = isdigit(b[l - 1]);

    char* part_name = sformat("%s%s%d", b, need_p ? "p" : "", num);

    char* sysfs1 = sformat("/sys/block/%s/%s/start", b, part_name);
    char* sysfs2 = sformat("/sys/block/%s/%s/size", b, part_name);

    printf("sysfs1: %s\n", sysfs1);
    printf("sysfs2: %s\n", sysfs2);

    uint64_t val1;
    if (!read_sysfs(sysfs1, &val1))
	error("failed to read start\n");

    if (val1 * sysfs_sector_size != start)
	error("start mismatch\n");

    uint64_t val2;
    if (!read_sysfs(sysfs2, &val2))
	error("failed to read size\n");

    if (val2 * sysfs_sector_size != size)
	error("size mismatch\n");

    free(sysfs2);
    free(sysfs1);

    return true;
}


static void
dm_haha(const disk_t* disk, const char** disk_name, const char** disk_uuid)
{
    struct dm_task *task = dm_task_create(DM_DEVICE_INFO);
    if (!task)
	error("dm_task_create failed");

    if (!dm_task_set_major_minor(task, disk_major(disk), disk_minor(disk), 0))
	error("dm_task_set_major_minor");

    if (!dm_task_run(task))
	error("dm_task_run failed");

    // dm_task_destroy seems to free name and uuid

    if (disk_name)
    {
	*disk_name = strdup(dm_task_get_name(task));
	if (!*disk_name)
	    error("strdup failed");

	printf("disk-name: %s\n", *disk_name);
    }

    if (disk_uuid)
    {
	*disk_uuid = strdup(dm_task_get_uuid(task));
	if (!*disk_uuid)
	    error("strdup failed");

	printf("disk-uuid: %s\n", *disk_uuid);
    }

    dm_task_destroy(task);
}


static void
dm_task_run_and_wait(struct dm_task* task)
{
    uint32_t cookie = 0;
    if (!dm_task_set_cookie(task, &cookie, 0))
	error("dm_task_set_cookie failed");

    if (!dm_task_run(task))
	error("dm_task_run failed");

    dm_udev_wait(cookie);
}


static void
dm_create_partition(disk_t* disk, int num, uint64_t start, uint64_t size)
{
    const char* disk_name = NULL;
    const char* disk_uuid = NULL;

    dm_haha(disk, &disk_name, &disk_uuid);

    struct dm_task* task = dm_task_create(DM_DEVICE_CREATE);
    if (!task)
	error("dm_task_create failed");

    char* part_name = sformat("%s-part%d", disk_name, num);
    printf("part-name: %s\n", part_name);
    dm_task_set_name(task, part_name);

    char* part_uuid = sformat("part%d-%s", num, disk_uuid);
    printf("part-uuid: %s\n", part_uuid);
    dm_task_set_uuid(task, part_uuid);

    char* params = sformat("%u:%u %zu", disk_major(disk), disk_minor(disk), start / dm_sector_size);
    dm_task_add_target(task, 0, size / dm_sector_size, "linear", params);

    dm_task_run_and_wait(task);

    dm_task_update_nodes();
    dm_task_destroy(task);

    free(params);
    free(part_name);
    free(part_uuid);
}


static void
dm_resize_partition(disk_t* disk, int num, uint64_t start, uint64_t size)
{
    const char* disk_name = NULL;

    dm_haha(disk, &disk_name, NULL);

    struct dm_task* task = dm_task_create(DM_DEVICE_RELOAD);
    if (!task)
	error("dm_task_create failed");

    char* part_name = sformat("%s-part%d", disk_name, num);
    printf("part-name: %s\n", part_name);
    dm_task_set_name(task, part_name);

    char* params = sformat("%d:%d %zd", disk_major(disk), disk_minor(disk), start / dm_sector_size);
    dm_task_add_target(task, 0, size / dm_sector_size, "linear", params);

    if (!dm_task_run(task))
	error("dm_task_run failed");

    dm_task_destroy(task);

    task = dm_task_create(DM_DEVICE_RESUME);
    if (!task)
	error("dm_task_create failed");

    dm_task_set_name(task, part_name);

    dm_task_run_and_wait(task);

    dm_task_update_nodes();
    dm_task_destroy(task);

    free(params);
    free(part_name);
}


static void
dm_remove_partition(disk_t* disk, int num)
{
    const char* disk_name = NULL;

    dm_haha(disk, &disk_name, NULL);

    struct dm_task *task = dm_task_create(DM_DEVICE_REMOVE);
    if (!task)
	error("dm_task_create failed");

    char* part_name = sformat("%s-part%d", disk_name, num);
    printf("part-name: %s\n", part_name);
    dm_task_set_name(task, part_name);

    dm_task_retry_remove(task);

    dm_task_run_and_wait(task);

    dm_task_update_nodes();
    dm_task_destroy(task);

    free(part_name);
}


static bool
dm_verify_partition(const disk_t* disk, int num, uint64_t start, uint64_t size)
{
    const char* disk_name = NULL;

    dm_haha(disk, &disk_name, NULL);

    struct dm_task* task = dm_task_create(DM_DEVICE_TABLE);
    if (!task)
	error("dm_task_create failed");

    char* part_name = sformat("%s-part%d", disk_name, num);
    printf("part-name: %s\n", part_name);
    dm_task_set_name(task, part_name);

    if (!dm_task_run(task))
	error("dm_task_run failed");

    char* target_type;
    char* params;

    uint64_t a, b;

    void* next = dm_get_next_target(task, NULL, &a, &b, &target_type, &params);

    printf("start and size: %zd %zd\n", a, b);
    printf("target-type: %s\n", target_type);
    printf("params: %s\n", params);

    // In theory there could be several sequential linear targets instead of one. But we
    // are not considering that case.

    if (next != NULL)
	error("more than one target");

    if (strcmp(target_type, "linear") != 0)
	error("target-type mismatch");

    if (a != 0)
	error("very strange\n");

    if (size != b * dm_sector_size)
	error("size mismatch\n");

    unsigned int major, minor;
    uint64_t c;

    if (sscanf (params, "%d:%d %zu", &major, &minor, &c) != 3)
	error("scanning table failed");

    printf("major and minor: %d %d\n", major, minor);
    printf("start: %zu\n", c);

    if (disk_major(disk) != major)
	error("major mismatch\n");

    if (disk_minor(disk) != minor)
	error("minor mismatch\n");

    if (start != c * dm_sector_size)
	error("start mismatch\n");

    free(part_name);

    dm_task_destroy(task);

    return true;
}


static enum Tech
linux_tech(const disk_t* disk)
{
    if (!disk_is_blk_device(disk))
	return TECH_NONE;

    if (dm_is_dm_major(disk_major(disk)))
	return TECH_DM;

    return TECH_BLKPG;
}


void
linux_create_partition(disk_t* disk, int num, uint64_t start, uint64_t size)
{
    printf("--- linux-create-partition ---\n");

    switch (linux_tech(disk))
    {
	case TECH_BLKPG:
	    blkpg_create_partition(disk, num, start, size);
	    break;

	case TECH_DM:
	    dm_create_partition(disk, num, start, size);
	    break;

	case TECH_NONE:
	    break;
    }
}


void
linux_resize_partition(disk_t* disk, int num, uint64_t start, uint64_t size)
{
    printf("--- linux-resize-partition ---\n");

    switch (linux_tech(disk))
    {
	case TECH_BLKPG:
	    blkpg_resize_partition(disk, num, start, size);
	    break;

	case TECH_DM:
	    dm_resize_partition(disk, num, start, size);
	    break;

	case TECH_NONE:
	    break;
    }
}


void
linux_remove_partition(disk_t* disk, int num)
{
    printf("--- linux-remove-partition ---\n");

    switch (linux_tech(disk))
    {
	case TECH_BLKPG:
	    blkpg_remove_partition(disk, num);
	    break;

	case TECH_DM:
	    dm_remove_partition(disk, num);
	    break;

	case TECH_NONE:
	    break;
    }
}


bool
linux_verify_partition(const disk_t* disk, int num, uint64_t start, uint64_t size)
{
    printf("--- linux-verify-partition ---\n");

    switch (linux_tech(disk))
    {
	case TECH_BLKPG:
	    huhu_verify_partition(disk, num, start, size);
	    return false;

	case TECH_DM:
	    return dm_verify_partition(disk, num, start, size);

	case TECH_NONE:
	    return false;
    }

    return false;
}
