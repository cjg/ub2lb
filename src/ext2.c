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
#include "ext2.h"

#define RDB_ALLOCATION_LIMIT	16
#define	IDNAME_RIGIDDISK	0x5244534B	/* "RDSK" */
#define	IDNAME_PARTITION	0x50415254	/* "PART" */

struct RigidDiskBlock {
	unsigned rdb_ID;
	int rdb_SummedLongs;
	int rdb_ChkSum;
	unsigned rdb_HostID;
	int rdb_BlockBytes;
	unsigned rdb_Flags;
	unsigned	rdb_BadBlockList;
	int	rdb_PartitionList;
	unsigned	rdb_FileSysHeaderList;
	unsigned	rdb_DriveInit;
	unsigned	rdb_Reserved1[6];
	unsigned	rdb_Cylinders;
	unsigned	rdb_Sectors;
	unsigned	rdb_Heads;
	unsigned	rdb_Interleave;
	unsigned	rdb_Park;
	unsigned	rdb_Reserved2[3];
	unsigned	rdb_WritePreComp;
	unsigned	rdb_ReducedWrite;
	unsigned	rdb_StepRate;
	unsigned	rdb_Reserved3[5];
	unsigned	rdb_RDBBlocksLo;
	unsigned	rdb_RDBBlocksHi;
	unsigned	rdb_LoCylinder;
	unsigned	rdb_HiCylinder;
	unsigned	rdb_CylBlocks;
	unsigned	rdb_AutoParkSeconds;
	unsigned	rdb_HighRDSKBlock;
	unsigned	rdb_Reserved4;
	char	rdb_DiskVendor[8];
	char	rdb_DiskProduct[16];
	char	rdb_DiskRevision[4];
	char	rdb_ControllerVendor[8];
	char	rdb_ControllerProduct[16];
	char	rdb_ControllerRevision[4];
	unsigned	rdb_Reserved5[10];
};

struct PartitionBlock {
	int pb_ID;
	int pb_SummedLongs;
	int pb_ChkSum;
	unsigned pb_HostID;
	int pb_Next;
	unsigned pb_Flags;
	unsigned pb_Reserved1[2];
	unsigned pb_DevFlags;
	char pb_DriveName[32];
	unsigned pb_Reserved2[15];
	int pb_Environment[17];
	unsigned pb_EReserved[15];
};

struct amiga_part_geometry
{
    unsigned table_size;
    unsigned size_blocks;
    unsigned unused1;
    unsigned surfaces;
    unsigned sector_per_block;
    unsigned block_per_track;
    unsigned reserved;
    unsigned prealloc;
    unsigned interleave;
    unsigned low_cyl;
    unsigned high_cyl;
    unsigned num_buffers;
    unsigned buf_mem_type;
    unsigned max_transfer;
    unsigned mask;
    int boot_priority;
    unsigned dos_type;
    unsigned baud;
    unsigned control;
    unsigned boot_blocks;
};


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

	for (i = 0; i < RDB_ALLOCATION_LIMIT; i++) {
		unsigned res = dev_desc->block_read(dev_desc->dev, i, 1,
						    (unsigned *)block_buffer);
		if (res == 1) {
			struct RigidDiskBlock *trdb = 
				(struct RigidDiskBlock *)block_buffer;
			if (trdb->rdb_ID == IDNAME_RIGIDDISK) {
				printf("Rigid disk block at %d\n", i);
				/* here we should check the sum */
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
	struct amiga_part_geometry *g;
	unsigned block, disk_type;

	rdb = get_rdisk(dev_desc);
	block = rdb->rdb_PartitionList;
	block_buffer = malloc(dev_desc->blksz);
	while (block != 0xFFFFFFFF) {
		if (dev_desc->block_read(dev_desc->dev, block, 1,
					 (unsigned *)block_buffer)) {
			p = (struct PartitionBlock *)block_buffer;
			if (p->pb_ID == IDNAME_PARTITION){
				/* here we should check the sum */
				if(part-- == 0)
					break;
				block = p->pb_Next;
			} else block = 0xFFFFFFFF;
		} else block = 0xFFFFFFFF;
	}

	g = (struct amiga_part_geometry *)&(p->pb_Environment);
	info->start = g->low_cyl  * g->block_per_track * g->surfaces;
	info->size  = (g->high_cyl - g->low_cyl + 1) * g->block_per_track * 
		g->surfaces - 1;
	info->blksz = rdb->rdb_BlockBytes;
	strcpy(info->name, p->pb_DriveName);
	
	disk_type = g->dos_type;

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

static int load_file(boot_dev_t * this, char *filename, void *buffer)
{
	return do_ext2load (buffer, filename, 0, 6);
}

static int destroy(boot_dev_t * this)
{
	free(this);
}

boot_dev_t *ext2_create(int discno, int partno)
{
	boot_dev_t *boot;
	block_dev_desc_t *dev_desc;
	dev_desc = get_dev(0);
	boot = malloc(sizeof(boot_dev_t));
	boot->load_file = (int (*)(void *, char *, void *))load_file;
	boot->destroy = (void (*)(void *))destroy;
	return boot;
}
