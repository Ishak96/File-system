/**
 * @file fs.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief main filesystem functions
 * @details initializing the partition files and utility functions
 *  to interact with the os
 */
#include <devutils.h>
#include <fs.h>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

/**
 * @brief format the superblock into the virtual filesystem
 * @details format and calculate the positions and sizes of each section
 * of the filesystem (eg. bitmaps and inode and data blocks)
 * @return this functions returns -1 in case of error and 0 on success
 */
int fs_format_super(struct fs_filesyst fs) {
	struct fs_super_block super;
	
	/* initializing the log values */
	super.magic = FS_MAGIC;
	super.nreads = 0;
	super.nwrites = 0;
	super.mounts = 1;
	
	super.mtime = get_cur_time();
	super.wtime = super.mtime;
	
	/* calculating the positions of sections */
	/* inode count is prefixed with a ratio, it cannot be null*/
	uint32_t nblocks = fs.nblocks - 1; /* for the super block */
	super.inode_count = SET_MINMAX(nblocks*FS_INODE_RATIO, 1, FS_MAX_INODE_COUNT);
	
	/* calculating the number of bits needed to rep inode count */
	uint32_t bits_to_rep = FS_INODES_PER_BLOCK*super.inode_count;
	uint32_t bits_per_block = FS_BLOCK_SIZE*BITS_PER_BYTE;
	super.inode_bitmap_size = NOT_NULL(bits_to_rep/bits_per_block);
	
	/* the rest is for data and data bitmap */
	uint32_t blocks_left = nblocks - (super.inode_count + super.inode_bitmap_size);
	super.data_bitmap_size = NOT_NULL((int) log2(blocks_left)); /* approximation */
	super.data_count = NOT_NULL(blocks_left - super.data_bitmap_size);
	
	/* sanity check */
	/* all are positive */
	if(	 !(super.inode_count > 0 &&
		   super.inode_bitmap_size > 0 &&
		   super.data_bitmap_size > 0 &&
		   super.data_count > 0))
	{
		fprintf(stderr, "fs_format_super: null size\n");
		return FUNC_ERROR;
	}
	
	/* total is equal to nblocks */
	if(   !(1 +
		   super.inode_count +
		   super.data_count +
		   super.data_bitmap_size +
		   super.inode_bitmap_size == fs.nblocks)) 
	{
		fprintf(stderr, "fs_format_super: wrong total\n");
		return FUNC_ERROR;
	}
	
	/* all blocks are free */
	super.free_data_count = super.data_count;
	super.free_inode_count = super.inode_count;
	
	/* getting the locations */
	super.inode_bitmap_loc = 1; /* directly after the superblock */
	super.data_bitmap_loc = 1 + super.inode_bitmap_size;
	super.inode_loc = super.data_bitmap_loc + super.data_bitmap_size;
	super.data_loc = super.inode_loc + super.inode_count;
	
	if(fs_write_block(fs, 0, &super, sizeof(super)) < 0) {
		fprintf(stderr, "fs_format_super: cannot write super!\n");
		return FUNC_ERROR;
	}
	return 0;
}

/**
 * @brief dump formatted content of the superblock
 * @details prints a human readable superblock from teh filesystem fs
 */
int fs_dump_super(struct fs_filesyst fs) {
	/* read the super block */
	union fs_block blk;
	if(fs_read_block(fs, 0, &blk) < 0) {
		fprintf(stderr, "fd_dump_super: dump failed, cannot read!\n");
		return FUNC_ERROR;
	}
	struct fs_super_block super = blk.super;
	
	printf("[%d] nblocks: %d\nSuperblock dump:\n", fs.fd, fs.nblocks);
	printf("Magic: %x\n", super.magic);

	printf("Bitmaps: \finode: \f");
	print_range(super.inode_bitmap_loc, super.inode_count);
	
	printf("         data: \f");
	print_range(super.data_bitmap_loc, super.data_count);
	
	printf("Inode table:\f");
	print_range(super.inode_loc, super.inode_count);
	
	printf("Data blocks:\f");
	print_range(super.data_loc, super.data_count);
	
	printf("Log:\n");
	printf("    number of reads: %d\n", super.nreads);
	printf("    number of writes: %d\n", super.nwrites);
	printf("    number of mounts: %d\n", super.nwrites);
	
	printf("    last mount time: %s", timetostr(super.mtime));
	printf("    last write time: %s", timetostr(super.wtime));
	
	return 0;
}
