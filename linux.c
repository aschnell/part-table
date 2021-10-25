

#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <libdevmapper.h>
#include <linux/blkpg.h>
#include <linux/fs.h>
#include <blkid/blkid.h>

#include "linux.h"
#include "disk.h"
#include "utils.h"


enum Tech { TECH_NONE, TECH_BLKPG, TECH_DM };


void
linux_discard(disk_t* disk, size_t start, size_t length)
{
    printf("--- linux-discard ---\n");

    uint64_t range[2] = { start, length };

    if (ioctl(disk_fd(disk), BLKDISCARD, &range) != 0)
	fprintf(stderr, "discard failed\n");
}


void
linux_wipe_signatures(disk_t* disk, size_t start, size_t length)
{
    printf("--- linux-wipe-signatures ---\n");

    blkid_probe pr = blkid_new_probe();
    if (!pr)
	error("blkid_new_probe failed");

    if (blkid_probe_set_device(pr, disk_fd(disk), start, length) != 0)
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
blkpg_create_partition(disk_t* disk, int num, size_t start, size_t length)
{
    struct blkpg_partition partition;
    memset(&partition, 0, sizeof(partition));
    partition.pno = num;
    partition.start = start;
    partition.length = length;

    if (blkpg_ioctl(disk_fd(disk), &partition, BLKPG_ADD_PARTITION) != 0)
	error("ioctl BLKPG_ADD_PARTITION failed");
}


static void
blkpg_resize_partition(disk_t* disk, int num, size_t start, size_t length)
{
    struct blkpg_partition partition;
    memset(&partition, 0, sizeof(partition));
    partition.pno = num;
    partition.start = start;
    partition.length = length;

    if (blkpg_ioctl(disk_fd(disk), &partition, BLKPG_RESIZE_PARTITION) != 0)
	error("ioctl BLKPG_RESIZE_PARTITION failed");
}


static void
blkpg_remove_partition(disk_t* disk, int num)
{
    struct blkpg_partition partition;
    memset(&partition, 0, sizeof(partition));
    partition.pno = num;

    if (blkpg_ioctl(disk_fd(disk), &partition, BLKPG_DEL_PARTITION) != 0)
	error("ioctl BLKPG_DEL_PARTITION failed");
}


static void
dm_haha(disk_t* disk, const char** disk_name, const char** disk_uuid)
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
dm_create_partition(disk_t* disk, int num, size_t start, size_t length)
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

    char* params = sformat("%d:%d %zd", disk_major(disk), disk_minor(disk), start / 512);
    dm_task_add_target(task, 0, length / 512, "linear", params);

    dm_task_run_and_wait(task);

    dm_task_update_nodes();
    dm_task_destroy(task);

    free(part_name);
    free(part_uuid);
}


static void
dm_resize_partition(disk_t* disk, int num, size_t start, size_t length)
{
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


static enum Tech
linux_tech(disk_t* disk)
{
    if (!disk_is_blk_device(disk))
	return TECH_NONE;

    if (dm_is_dm_major(disk_major(disk)))
	return TECH_DM;

    return TECH_BLKPG;
}


void
linux_create_partition(disk_t* disk, int num, size_t start, size_t length)
{
    printf("--- linux-create-partition ---\n");

    switch (linux_tech(disk))
    {
	case TECH_BLKPG:
	    blkpg_create_partition(disk, num, start, length);
	    break;

	case TECH_DM:
	    dm_create_partition(disk, num, start, length);
	    break;

	case TECH_NONE:
	    break;
    }
}


void
linux_resize_partition(disk_t* disk, int num, size_t start, size_t length)
{
    printf("--- linux-resize-partition ---\n");

    switch (linux_tech(disk))
    {
	case TECH_BLKPG:
	    blkpg_resize_partition(disk, num, start, length);
	    break;

	case TECH_DM:
	    dm_resize_partition(disk, num, start, length);
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
