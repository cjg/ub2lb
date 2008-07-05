#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define RDB_LOCATION_LIMIT 16

#define	IDNAME_RIGIDDISK	(uint32_t)0x5244534B	/* 'RDSK' */
#define IDNAME_BADBLOCK		(uint32_t)0x42414442	/* 'BADB' */
#define	IDNAME_PARTITION	(uint32_t)0x50415254	/* 'PART' */
#define IDNAME_FILESYSHEADER	(uint32_t)0x46534844	/* 'FSHD' */
#define IDNAME_LOADSEG		(uint32_t)0x4C534547	/* 'LSEG' */
#define IDNAME_BOOT		(uint32_t)0x424f4f54	/* 'BOOT' */
#define IDNAME_FREE		(uint32_t)0xffffffff	

#define IS_FREE(blk) ((*(uint32_t *) (blk)) != IDNAME_RIGIDDISK		\
		      && (*(uint32_t *) (blk)) != IDNAME_BADBLOCK	\
		      && (*(uint32_t *) (blk)) != IDNAME_PARTITION	\
		      && (*(uint32_t *) (blk)) != IDNAME_FILESYSHEADER	\
		      && (*(uint32_t *) (blk)) != IDNAME_LOADSEG	\
		      && (*(uint32_t *) (blk)) != IDNAME_BOOT)

typedef unsigned uint32_t;
typedef int int32_t;

struct AmigaBlock {
	uint32_t	amiga_ID;		/* Identifier 32 bit word */
	uint32_t	amiga_SummedLongss;	/* Size of the structure for checksums */
	int32_t		amiga_ChkSum;		/* Checksum of the structure */
};
#define AMIGA(pos) ((struct AmigaBlock *)(pos)) 

struct RigidDiskBlock {
    uint32_t	rdb_ID;			/* Identifier 32 bit word : 'RDSK' */
    uint32_t	rdb_SummedLongs;	/* Size of the structure for checksums */
    int32_t	rdb_ChkSum;		/* Checksum of the structure */
    uint32_t	rdb_HostID;		/* SCSI Target ID of host, not really used */
    uint32_t	rdb_BlockBytes;		/* Size of disk blocks */
    uint32_t	rdb_Flags;		/* RDB Flags */
    /* block list heads */
    uint32_t	rdb_BadBlockList;	/* Bad block list */
    uint32_t	rdb_PartitionList;	/* Partition list */
    uint32_t	rdb_FileSysHeaderList;	/* File system header list */
    uint32_t	rdb_DriveInit;		/* Drive specific init code */
    uint32_t	rdb_BootBlockList;	/* Amiga OS 4 Boot Blocks */
    uint32_t	rdb_Reserved1[5];	/* Unused word, need to be set to $ffffffff */
    /* physical drive characteristics */
    uint32_t	rdb_Cylinders;		/* Number of the cylinders of the drive */
    uint32_t	rdb_Sectors;		/* Number of sectors of the drive */
    uint32_t	rdb_Heads;		/* Number of heads of the drive */
    uint32_t	rdb_Interleave;		/* Interleave */
    uint32_t	rdb_Park;		/* Head parking cylinder */
    uint32_t	rdb_Reserved2[3];	/* Unused word, need to be set to $ffffffff */
    uint32_t	rdb_WritePreComp;	/* Starting cylinder of write precompensation */
    uint32_t	rdb_ReducedWrite;	/* Starting cylinder of reduced write current */
    uint32_t	rdb_StepRate;		/* Step rate of the drive */
    uint32_t	rdb_Reserved3[5];	/* Unused word, need to be set to $ffffffff */
    /* logical drive characteristics */
    uint32_t	rdb_RDBBlocksLo;	/* low block of range reserved for hardblocks */
    uint32_t	rdb_RDBBlocksHi;	/* high block of range for these hardblocks */
    uint32_t	rdb_LoCylinder;		/* low cylinder of partitionable disk area */
    uint32_t	rdb_HiCylinder;		/* high cylinder of partitionable data area */
    uint32_t	rdb_CylBlocks;		/* number of blocks available per cylinder */
    uint32_t	rdb_AutoParkSeconds;	/* zero for no auto park */
    uint32_t	rdb_HighRDSKBlock;	/* highest block used by RDSK */
					/* (not including replacement bad blocks) */
    uint32_t	rdb_Reserved4;
    /* drive identification */
    char	rdb_DiskVendor[8];
    char	rdb_DiskProduct[16];
    char	rdb_DiskRevision[4];
    char	rdb_ControllerVendor[8];
    char	rdb_ControllerProduct[16];
    char	rdb_ControllerRevision[4];
    uint32_t	rdb_Reserved5[10];
};

struct BootstrapCodeBlock {
	uint32_t bcb_ID;             /* 4 character identifier */
	uint32_t bcb_SummedLongs;    /* size of this checksummed structure */
	int32_t bcb_ChkSum;         /* block checksum (longword sum to zero) */
	uint32_t bcb_HostID;         /* SCSI Target ID of host */
	uint32_t bcb_Next;           /* block number of the next BootstrapCodeBlock */
	uint32_t   bcb_LoadData[123];  /* binary data of the bootstrapper */
    /* note [123] assumes 512 byte blocks */
};

int checksum(struct AmigaBlock *_header)
{
	struct AmigaBlock *header;
	header = malloc(512);
	memmove(header, _header, 512);
	int32_t *block = (int32_t *)header;
	uint32_t i;
	int32_t sum = 0;
	if (header->amiga_SummedLongss > 512)
		header->amiga_SummedLongss = 512;
	for (i = 0; i < header->amiga_SummedLongss; i++)
		sum += *block++;
	free(header);
	return sum;
}


static void calculate_checksum (struct AmigaBlock *blk) {
	blk->amiga_ChkSum = blk->amiga_ChkSum - checksum(blk);
	return;	
}

static struct AmigaBlock *read_block (FILE *dev, 
				      struct AmigaBlock *blk,
				      uint32_t block)
{
	uint32_t pos;

	pos = ftell(dev);

	fseek(dev, block * 512, SEEK_SET);

	fread(blk, 512, 1, dev);

	fseek(dev, pos, SEEK_SET);

/*	printf("block: %d ID: %x\n", block, blk->amiga_ID); */
	
	if(checksum(blk) != 0) {
/*		printf("parthenope-install: Bad checksum at block: %d!\n", block);*/
  		return NULL;
	}
	
	return blk;
}

static struct RigidDiskBlock *read_rdb (FILE *dev, 
					struct RigidDiskBlock *blk,
					uint32_t block)
{
	uint32_t pos;

	pos = ftell(dev);
	fseek(dev, block * 512, SEEK_SET);
	fread(blk, sizeof(struct RigidDiskBlock), 1, dev);
	fseek(dev, pos, SEEK_SET);
	
	return blk;
}

struct RigidDiskBlock *find_rdb(FILE *dev, uint32_t *rdb_block)
{
	uint32_t i;
	struct RigidDiskBlock *rdb;

	rdb = malloc(512);

	for (i = 0; i < RDB_LOCATION_LIMIT; i++) {
		if (read_block (dev, (struct AmigaBlock *) rdb, i) == NULL
		    || rdb->rdb_ID != IDNAME_RIGIDDISK) 
			continue;
		if (checksum((struct AmigaBlock *) rdb) != 0) {
			printf("parthenope-install: Bad checksum at block "
			       "%d\n", i);
			continue;
		}
		*rdb_block = i;
		return read_rdb(dev, rdb, i);
	}

	free(rdb); 
	return NULL;
}

int next_free_block(FILE *dev, uint32_t *block, uint32_t *free_pointer, 
		    uint32_t nblocks)
{
	struct AmigaBlock *blk;
	
	blk = malloc(512);
	
	do {
		(*free_pointer)++;
		if(read_block(dev, blk, *free_pointer) == NULL)
			continue;
/*		printf("next_free_block(): block: %u free: %d ID: %x %x\n", *free_pointer,
		       IS_FREE(blk), blk->amiga_ID, *((uint32_t *) blk));*/
	} while(!IS_FREE(blk) && *free_pointer < nblocks);

	if(IS_FREE(blk)) {
		free(blk);
		*block = *free_pointer;
		return *block;
	}

	free(blk);

	return -1;
}

void write_block(FILE *dev, void *blk, uint32_t block)
{
	fseek(dev, block * 512, SEEK_SET);
        fwrite(blk, 1, 512, dev);            
}

void erase_slb(FILE *dev, struct RigidDiskBlock *rdb)
{
	uint32_t block;
	struct AmigaBlock *blk;

	printf("parthenope-install: Erasing old slb ...");
	fflush(stdout);

	blk = malloc(512);
       	for(block = rdb->rdb_BootBlockList; 
	    block != IDNAME_FREE 
		    && read_block(dev, blk, block) != NULL
		    && blk->amiga_ID == IDNAME_BOOT; 
	    block = ((struct BootstrapCodeBlock *) blk)->bcb_Next) {
		blk->amiga_ID = IDNAME_FREE;
		calculate_checksum(blk);
		write_block(dev, blk, block);
	       
	}

	printf(" done!\n");
	
}

void install_slb(FILE *dev, struct RigidDiskBlock *rdb, uint32_t *slb, 
		 uint32_t slb_size)
{
	uint32_t offset;
	uint32_t block, next_block, free_block_pointer;
	struct BootstrapCodeBlock *blk;

	free_block_pointer = 0;

	if(next_free_block(dev, &block, &free_block_pointer,  
			   rdb->rdb_RDBBlocksHi - rdb->rdb_RDBBlocksLo) < 0) {
		printf("parthenope-install: Not enough free blocks!\n");
		return;
	}

	rdb->rdb_BootBlockList = block;
	
	blk = malloc(512);
	for(offset = 0; offset < slb_size; offset += 123) {
		if(next_free_block(dev, &next_block, &free_block_pointer,
				   rdb->rdb_RDBBlocksHi - rdb->rdb_RDBBlocksLo)
		   < 0) {
		                             
			printf("parthenope-install: Not enough free blocks!\n");
		                                      
			return;
		                                      
		}   
		blk->bcb_ID = IDNAME_BOOT;
		blk->bcb_SummedLongs = 128;
		blk->bcb_HostID = 0;
		blk->bcb_Next = next_block;
		if(slb_size - offset <= 123) {
			blk->bcb_SummedLongs = 5 + slb_size - offset;
			blk->bcb_Next = IDNAME_FREE;
		}
		memmove(blk->bcb_LoadData, slb + offset, 123 
			* sizeof(uint32_t));
		calculate_checksum(blk);
		write_block(dev, blk, block);
		block = next_block;
	}
}

int main(void)
{
	FILE *dev, *f;
	struct RigidDiskBlock *rdb;
	uint32_t rdb_block, *slb, slb_size;
	struct stat st;

	dev = fopen("/dev/sda", "w+");
	if(dev == NULL) {
		fprintf(stderr, "parthenope-install: Cannot open /dev/sda!\n");
		return -1;
	}
		
	rdb = find_rdb(dev, &rdb_block);
	if(rdb == NULL) {
		fprintf(stderr, "parthenope-install: Cannot find RDB!\n");
		fclose(dev);
		return -1;
	}

	if(lstat("Parthenope", &st) < 0) {
		fprintf(stderr, "parthenope-install: Cannot stat Parthenope!\n");
		free(rdb);
		fclose(dev);
		return -1;
	}

	f = fopen("Parthenope", "r");
	if (f == NULL) {
		fprintf(stderr, "parthenope-install: Cannot open Parthenope!\n");
		free(rdb);
		fclose(dev);
		return -1;
	}
	
	slb_size = st.st_size / sizeof(uint32_t);
	slb = malloc(st.st_size);

	fread(slb, 1, st.st_size, f);
	fclose(f);

	erase_slb(dev, rdb);
	install_slb(dev, rdb, slb, slb_size);
	calculate_checksum(rdb);
	write_block(dev, rdb, rdb_block);
	printf("BootBlockList: %u\n", rdb->rdb_BootBlockList);
	free(rdb);
	fclose(dev);

	return 0;
}
