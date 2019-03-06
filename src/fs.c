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
#include <string.h>

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
	super.data_bitmap_size = NOT_NULL((int) log2(blocks_left / (FS_BLOCK_SIZE*BITS_PER_BYTE))); /* approximation */

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
	super.free_inode_count = super.inode_count * FS_INODES_PER_BLOCK;
	
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
	print_range(super.inode_bitmap_loc, super.inode_bitmap_size);
	
	printf("         data: \f");
	print_range(super.data_bitmap_loc, super.data_bitmap_size);
	
	printf("Inode table:\f");
	print_range(super.inode_loc, super.inode_count);
	
	printf("Data blocks:\f");
	print_range(super.data_loc, super.data_count);
	
	printf("Log:\n");
	printf("    number of reads: %d\n", super.nreads);
	printf("    number of writes: %d\n", super.nwrites);
	printf("    number of mounts: %d\n", super.mounts);
	printf("    number of free inodes spaces: %d\n", super.free_inode_count);
	printf("    number of free data spaces: %d\n", super.free_data_count);
	
	printf("    last mount time: %s", timetostr(super.mtime));
	printf("    last write time: %s", timetostr(super.wtime));
	
	return 0;
}

/**
 * @brief format the filesystem
 * @details formats the superblock and sets the bitmaps to 0
 */
int fs_format(struct fs_filesyst fs) {
	/* format the superblock */
	if(fs_format_super(fs) < 0) {
		fprintf(stderr, "fs_format: fs_format_super\n");
		return FUNC_ERROR;
	}
	
	/* set the bitmaps to 0 */
	union fs_block blk;
	
	/* read the super block*/
	struct fs_super_block super;
	if(fs_read_block(fs, 0, &blk) < 0) {
		fprintf(stderr, "fs_format: fs_read_block\n");
		return FUNC_ERROR;
	}
	super = blk.super;
	
	/* reset the blk buffer to 0 */
	memset(&blk, 0, FS_BLOCK_SIZE);
	
	/* set the data bitmap to 0 */
	for(int i=super.data_bitmap_loc; i<super.data_bitmap_loc+super.data_bitmap_size; i++) {
		if(fs_write_block(fs, i, &blk, FS_BLOCK_SIZE) < 0) {
			fprintf(stderr, "fs_format: fs_write_block\n");
			return FUNC_ERROR;
		}
	}

	/* set the inode bitmap to 0 */
	for(int i=super.inode_bitmap_loc; i<super.inode_bitmap_loc+super.inode_bitmap_size; i++) {
		if(fs_write_block(fs, i, &blk, FS_BLOCK_SIZE) < 0) {
			fprintf(stderr, "fs_format: fs_write_block\n");
			return FUNC_ERROR;
		}
	}
	
	return 0;
}

/**
 * @brief allocate an inode
 * @details allocates the first free inode in the inode table
 * @arg fs: the virtual filesystem
 * @arg super: the superblock
 * @arg inodenum: the inode number allocated
 */
int fs_alloc_inode(struct fs_filesyst fs, struct fs_super_block* super, uint32_t *inodenum) {
	if(super->free_inode_count == 0){
		fprintf(stderr, "fs_alloc_inode: no space left!\n");
		return FUNC_ERROR;
	}

	uint32_t start = super->inode_bitmap_loc;
	uint32_t end = super->inode_bitmap_loc + super->inode_bitmap_size;
	if(inodenum == NULL){
		fprintf(stderr, "fs_alloc_inode: invalid inodenum!\n");
		return FUNC_ERROR;		
	}

	if(end <= start) {
		fprintf(stderr, "fs_alloc_inode: invalid bitmap blocks!\n");
		return FUNC_ERROR;
	}
	
	int found = 0;
	uint32_t off; /* offset of the first null bit in the first block containing one */
	uint32_t blknum;
	union fs_block blk;
	/* parse the inode bitmap and look for the first bit in the first block that is free */
	for(blknum=start; blknum<end && found==0; blknum++) {
		/* read the block */
		if(fs_read_block(fs, blknum, &blk) < 0) {
			fprintf(stderr, "fs_alloc_inode: fs_read_block!\n");
			return FUNC_ERROR;
		}
		/* parse the block for a null bit */
		for(int i=0; i<FS_BLOCK_SIZE && found==0; i++) {
			uint8_t byte = blk.data[i];
			if(byte != 255) {
				off = 0;
				uint8_t temp = byte;
				while(temp) { /* to get the first one */
					temp >>= 1;
					off ++;
				}
				/* calculate the marked byte */
				uint8_t marked_byte = 1;
				marked_byte <<= off;
				marked_byte |= byte;
				off += i * 8; /* add the offset of the byte location in the block */
				found = 1;
				/* mark as read */
				blk.data[i] = marked_byte;
				
				/* write to disk */
				if(fs_write_block(fs, blknum, &blk, FS_BLOCK_SIZE) < 0) {
					fprintf(stderr, "fs_alloc_inode: fs_write_block!\n");
					return FUNC_ERROR;
				}
			}
		}
	}
	
	/* real inode offset in blocks from super->inode_loc */
	uint32_t indno = ((blknum - start - 1) * FS_BLOCK_SIZE * BITS_PER_BYTE) + off;
	*inodenum = indno;

	uint32_t blkno = indno / FS_INODES_PER_BLOCK;

	if(!found || blkno >= super->inode_count) {
		fprintf(stderr, "fs_alloc_inode: no space left\n");
		return FUNC_ERROR;
	}

	super->free_inode_count--;
	printf("free = %d\n", super->free_inode_count);
	/* write to disk */
	if(fs_write_block(fs, 0, super, sizeof(*super)) < 0) {
		fprintf(stderr, "fs_alloc_inode: fs_write_block!\n");
		return FUNC_ERROR;
	}
	return 0;
}

int fs_write_inode(struct fs_filesyst fs, struct fs_super_block super, uint32_t indno, struct fs_inode *inode){
	uint32_t blkno = indno / FS_INODES_PER_BLOCK;
	/* getting the real block offset from the start */
	blkno += super.inode_loc;
	
	/* offset in the block containing the inode */
	uint8_t indoff = indno % FS_INODES_PER_BLOCK;
	
	//~ /* reading the block containing the inode */
	union fs_block iblk;
	if(fs_read_block(fs, blkno, &iblk) < 0) {
		fprintf(stderr, "fs_alloc_inode: fs_read_block!\n");
		return FUNC_ERROR;
	}
	//~ /* setting the inode in the block */
	iblk.inodes[indoff] = *inode;
	
	/* writing changes to disk */
	if(fs_write_block(fs, blkno, &iblk, FS_BLOCK_SIZE) < 0) {
		fprintf(stderr, "fs_alloc_inode: fs_write_block!\n");
		return FUNC_ERROR;
	}
	
	return 0;
}

int fs_dump_inode(struct fs_filesyst fs, struct fs_super_block super, uint32_t inodenum) {
	uint32_t blkno = inodenum / FS_INODES_PER_BLOCK + super.inode_loc;
	uint8_t indoff = inodenum % FS_INODES_PER_BLOCK;
	
	union fs_block blk;
	if(fs_read_block(fs, blkno, &blk) < 0) {
		fprintf(stderr, "fd_dump_super: dump failed, cannot read!\n");
		return FUNC_ERROR;
	}
	
	struct fs_inode ind;
	ind = blk.inodes[indoff];
	
	printf("Inode %d dump:\n", inodenum);
	printf("mode: %d\n", ind.mode);
	printf("uid: %d\n", ind.uid);
	printf("gid: %d\n", ind.gid);
	printf("atime: %s", timetostr(ind.atime));
	printf("mtime: %s", timetostr(ind.mtime));
	printf("size: %d\n", ind.size);
	return 0;
}

int fs_alloc_data(struct fs_filesyst fs, struct fs_super_block* super, uint32_t data[], size_t size) {
	if(super->free_data_count > size){
		fprintf(stderr, "fs_alloc_inode: no space left!\n");
		return FUNC_ERROR;
	}
	/* param check */
	if(data == NULL || size <= 0) {
		fprintf(stderr, "fs_alloc_data: invalid arguments!\n");
		return FUNC_ERROR;
	}

	uint32_t start = super->data_bitmap_loc;
	uint32_t end = super->data_bitmap_loc + super->data_bitmap_size;
	if(end <= start) {
		fprintf(stderr, "fs_alloc_inode: invalid bitmap blocks!\n");
		return FUNC_ERROR;
	}
	
	int left = size;
	uint32_t off; /* offset of the first null bit in the first block containing one */
	uint32_t blknum;
	union fs_block blk;
	/* parse the data bitmap and look for the first bit in the first block that is free */
	for(blknum=start; blknum<end && left>0; blknum++) {
		/* read the block */
		if(fs_read_block(fs, blknum, &blk) < 0) {
			fprintf(stderr, "fs_alloc_inode: fs_read_block!\n");
			return FUNC_ERROR;
		}
		/* parse the block for a null bit */
		for(int i=0; i<FS_BLOCK_SIZE && left>0; i++) {
			uint8_t byte = blk.data[i];
			if(byte != 255) {
				off = 0;
				uint8_t temp = byte;
				while(temp) { /* to get the first one */
					temp >>= 1;
					off ++;
				}
				/* calculate the marked byte */
				uint8_t marked_byte = 1;
				marked_byte <<= off;
				marked_byte |= byte;
				off += i * 8; /* add the offset of the byte location in the block */
				/* mark as read */
				blk.data[i] = marked_byte;

				left --;
				data[left] = ((blknum - start) * FS_BLOCK_SIZE * BITS_PER_BYTE) + off;
				
				/* write to disk */
				if(fs_write_block(fs, blknum, &blk, FS_BLOCK_SIZE) < 0) {
					fprintf(stderr, "fs_alloc_inode: fs_write_block!\n");
					return FUNC_ERROR;
				}
				if(byte != 255 && left != 0) {
					i--;
				}
			}
		}
	}
	super->free_data_count -= size;

	/* write to disk */
	if(fs_write_block(fs, 0, super, sizeof(*super)) < 0) {
		fprintf(stderr, "fs_alloc_inode: fs_write_block!\n");
		return FUNC_ERROR;
	}
	return 0;
}

int fs_free_inod(struct fs_filesyst fs, struct fs_super_block super, uint32_t no){

	return 0;
}
