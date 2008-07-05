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

struct RigidDiskBlock *get_rdisk(block_dev_desc_t *dev_desc)
{
	int i;
	char *s;
	char *block_buffer = malloc(dev_desc->blksz);

	for (i = 0; i < RDB_LOCATION_LIMIT; i++) {
		unsigned res = dev_desc->block_read(dev_desc->dev, i, 1,
						    (unsigned *)block_buffer);
		if (res == 1) {
			struct RigidDiskBlock *trdb = 
				(struct RigidDiskBlock *)block_buffer;
			if (trdb->rdb_ID == IDNAME_RIGIDDISK) {
				/* TODO: here we should check the sum */
				return trdb;
			}
		}
	}
	printf("Done scanning, no RDB found\n");
	return NULL;
}


int get_partition_info (block_dev_desc_t *dev_desc, int part, 
			disk_partition_t *info)
{
	int part_length;
	char *block_buffer; 
	struct RigidDiskBlock *rdb;
	struct PartitionBlock *p;
	struct AmigaPartitionGeometry *g;
	unsigned block, disk_type;

	rdb = get_rdisk(dev_desc);
	block = rdb->rdb_PartitionList;
	block_buffer = malloc(dev_desc->blksz);
	while (block != 0xFFFFFFFF) {
		if (dev_desc->block_read(dev_desc->dev, block, 1,
					 (unsigned *)block_buffer)) {
			p = (struct PartitionBlock *)block_buffer;
			if (p->pb_ID == IDNAME_PARTITION){
				/* TODO: here we should check the sum */
				if(part-- == 0)
					break;
				block = p->pb_Next;
			} else block = 0xFFFFFFFF;
		} else block = 0xFFFFFFFF;
	}

	g = (struct AmigaPartitionGeometry *)&(p->pb_Environment);
	info->start = g->apg_LowCyl  * g->apg_BlockPerTrack * g->apg_Surfaces;
	info->size  = (g->apg_HighCyl - g->apg_LowCyl + 1) 
		* g->apg_BlockPerTrack * g->apg_Surfaces - 1;
	info->blksz = rdb->rdb_BlockBytes;
	strcpy(info->name, p->pb_DriveName);
	
	disk_type = g->apg_DosType;

	info->type[0] = (disk_type & 0xFF000000)>>24;
	info->type[1] = (disk_type & 0x00FF0000)>>16;
	info->type[2] = (disk_type & 0x0000FF00)>>8;
	info->type[3] = '\\';
	info->type[4] = (disk_type & 0x000000FF) + '0';
	info->type[5] = 0;

	return 0;
}

static int do_ext2load (char *addr, char *filename, int dev, int part)
{
	unsigned part_length, filelen;
	disk_partition_t info;
	block_dev_desc_t *dev_desc = NULL;

	dev_desc = get_dev(dev);
	if (dev_desc==NULL) {
		printf ("\n** Block device %d not supported\n", dev);
		return -1;
	}

	if (get_partition_info (dev_desc, part, &info)) {
		printf ("** Bad partition %d **\n", part);
		return -2;
	}

	if ((part_length = ext2fs_set_blk_dev_full(dev_desc, &info)) == 0) {
		printf ("** Bad partition - %d:%d **\n",  dev, part);
		ext2fs_close();
		return -3;
	}

	if (!ext2fs_mount(part_length)) {
		printf ("** Bad ext2 partition or disk - %d:%d **\n",  
			dev, part);
		ext2fs_close();
		return -4;
	}

	filelen = ext2fs_open(filename);
	if (filelen < 0) {
		printf("** File not found %s\n", filename);
		ext2fs_close();
		return -5;
	}

	if (ext2fs_read((char *)addr, filelen) != filelen) {
		printf("\n** Unable to read \"%s\" from %d:%d **\n", 
		       filename, dev, part);
		ext2fs_close();
		return -6;
	}

	ext2fs_close();

	return filelen;
}

typedef struct {
	int (*load_file) (void *this, char *filename, void *buffer);
	void (*destroy) (void *this);
	int discno;
	int partno;
} ext2_boot_dev_t;

static int load_file(ext2_boot_dev_t * this, char *filename, void *buffer)
{
	return do_ext2load (buffer, filename, this->discno, this->partno);
}

static int destroy(ext2_boot_dev_t * this)
{
	free(this);
}

boot_dev_t *ext2_create(int discno, int partno)
{
	ext2_boot_dev_t *boot;
	block_dev_desc_t *dev_desc;
	dev_desc = get_dev(discno);
	if(dev_desc == NULL)
		return NULL;
	boot = malloc(sizeof(ext2_boot_dev_t));
	boot->load_file = (int (*)(void *, char *, void *))load_file;
	boot->destroy = (void (*)(void *))destroy;
	boot->discno = discno;
	boot->partno = partno;
	return (boot_dev_t *) boot;
}
