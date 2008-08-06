/* ext2.c */

/* <project_name> -- <project_description>
 *
 * Copyright (C) 2006 - 2007
 *     Giuseppe Coviello <cjg@cruxppc.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "context.h"
#include "rdb.h"
#include "ext2.h"

static block_dev_desc_t *get_dev(int dev)
{
	block_dev_desc_t *bdev = NULL;
	SCAN_HANDLE hnd;
	uint32_t blocksize;

	for (hnd = start_unit_scan(get_scan_list(), &blocksize);
	     hnd != NULL; hnd = next_unit_scan(hnd, &blocksize)) {
		if (hnd->ush_device.type == DEV_TYPE_HARDDISK) {
			bdev = malloc(sizeof(block_dev_desc_t));
			memmove(bdev, &hnd->ush_device,
				sizeof(block_dev_desc_t));
			break;
		}
	}

	end_unit_scan(hnd);
	end_global_scan();

	return bdev;
}

struct RigidDiskBlock *get_rdb(block_dev_desc_t * dev_desc)
{
	int i;
	char *block_buffer = malloc(dev_desc->blksz);

	for (i = 0; i < RDB_LOCATION_LIMIT; i++) {
		unsigned res = dev_desc->block_read(dev_desc->dev, i, 1,
						    (unsigned *)block_buffer);
		if (res == 1) {
			struct RigidDiskBlock *trdb =
			    (struct RigidDiskBlock *)block_buffer;
			if (trdb->rdb_ID == IDNAME_RIGIDDISK) {
				if (checksum((struct AmigaBlock *)trdb) != 0)
					continue;
				return trdb;
			}
		}
	}
	printf("Done scanning, no RDB found\n");
	return NULL;
}

int get_partition_info(block_dev_desc_t * dev_desc, int part,
		       disk_partition_t * info)
{
	char *block_buffer;
	struct RigidDiskBlock *rdb;
	struct PartitionBlock *p;
	struct AmigaPartitionGeometry *g;
	unsigned block, disk_type;

	rdb = get_rdb(dev_desc);
	block = rdb->rdb_PartitionList;
	block_buffer = malloc(dev_desc->blksz);
	p = NULL;
	while (block != 0xFFFFFFFF) {
		if (dev_desc->block_read(dev_desc->dev, block, 1,
					 (unsigned *)block_buffer)) {
			p = (struct PartitionBlock *)block_buffer;
			if (p->pb_ID == IDNAME_PARTITION) {
				if (checksum((struct AmigaBlock *)p) != 0)
					continue;
				if (part-- == 0)
					break;
				block = p->pb_Next;
			} else
				block = 0xFFFFFFFF;
			p = NULL;
		} else
			block = 0xFFFFFFFF;
	}

	if (p == NULL)
		return -1;

	g = (struct AmigaPartitionGeometry *)&(p->pb_Environment);
	info->start = g->apg_LowCyl * g->apg_BlockPerTrack * g->apg_Surfaces;
	info->size = (g->apg_HighCyl - g->apg_LowCyl + 1)
	    * g->apg_BlockPerTrack * g->apg_Surfaces - 1;
	info->blksz = rdb->rdb_BlockBytes;
	strcpy((char *)info->name, p->pb_DriveName);

	disk_type = g->apg_DosType;

	info->type[0] = (disk_type & 0xFF000000) >> 24;
	info->type[1] = (disk_type & 0x00FF0000) >> 16;
	info->type[2] = (disk_type & 0x0000FF00) >> 8;
	info->type[3] = '\\';
	info->type[4] = (disk_type & 0x000000FF) + '0';
	info->type[5] = 0;

	return 0;
}

typedef struct {
	int (*load_file) (void *this, char *filename, void *buffer);
	void (*destroy) (void *this);
	int discno;
	int partno;
	unsigned part_length;
} ext2_boot_dev_t;

static int load_file(ext2_boot_dev_t * this, char *filename, void *buffer)
{
	unsigned filelen;

	if (!ext2fs_mount(this->part_length)) {
		printf("** Bad ext2 partition or disk - %d:%d **\n",
		       this->discno, this->partno);
		ext2fs_close();
		return -4;
	}

	filelen = ext2fs_open(filename);
	if (filelen < 0) {
		printf("** File not found %s\n", filename);
		ext2fs_close();
		return -5;
	}

	if (ext2fs_read((char *)buffer, filelen) != filelen) {
		printf("\n** Unable to read \"%s\" from %d:%d **\n",
		       filename, this->discno, this->partno);
		ext2fs_close();
		return -6;
	}

	ext2fs_close();

	return filelen;
}

static int destroy(ext2_boot_dev_t * this)
{
	free(this);
	return 0;
}

boot_dev_t *ext2_create(int discno, int partno)
{
	ext2_boot_dev_t *boot;
	disk_partition_t info;
	block_dev_desc_t *dev_desc = NULL;
	unsigned part_length;

	dev_desc = get_dev(discno);
	if (dev_desc == NULL) {
		printf("\n** Block device %d not supported\n", discno);
		return NULL;
	}

	if (get_partition_info(dev_desc, partno, &info)) {
		printf("** Bad partition %d **\n", partno);
		return NULL;
	}

	if ((part_length = ext2fs_set_blk_dev_full(dev_desc, &info)) == 0) {
		printf("** Bad partition - %d:%d **\n", discno, partno);
		ext2fs_close();
		return NULL;
	}

	if (!ext2fs_mount(part_length)) {
		printf("** Bad ext2 partition or disk - %d:%d **\n",
		       discno, partno);
		ext2fs_close();
		return NULL;
	}

	ext2fs_close();

	boot = malloc(sizeof(ext2_boot_dev_t));
	boot->load_file = (int (*)(void *, char *, void *))load_file;
	boot->destroy = (void (*)(void *))destroy;
	boot->discno = discno;
	boot->partno = partno;
	boot->part_length = part_length;

	return (boot_dev_t *) boot;
}

static int has_file(ext2_boot_dev_t * this, char *filename)
{
	unsigned filelen;

	ext2fs_mount(this->part_length);
	filelen = ext2fs_open(filename);
	ext2fs_close();

	return (int)filelen > 0;
}

boot_dev_t *ext2_guess_booting(int discno)
{
	int i;
	boot_dev_t *boot;
	for (i = 0; i < 16; i++) {
		boot = ext2_create(discno, i);
		if (boot == NULL)
			continue;
		if (has_file((ext2_boot_dev_t *) boot, "menu.lst")
		    || has_file((ext2_boot_dev_t *) boot, "boot/menu.lst"))
			return boot;
		boot->destroy(boot);
	}

	return NULL;
}
