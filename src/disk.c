/**
 * @file meminit.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief initializing the partition
 * @details initializing the partition files and utility functions to interact with the os
 */
#include <devutils.h>
#include <disk.h>
#include <fs.h>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int creatfile(const char* filename, size_t size) {
	if(size <= 0) {
		die("creatfile: null size");
	}
	
	int fd = open(filename, O_RDWR | O_CREAT, 0777);
	if(fd < 0) {
		die("creatfile: open");
	}
	
	lseek(fd, size-1, SEEK_SET);
	write(fd, "\0", 1);
	return fd;
}

/**
 * @brief write a chunk of data into a block
 * @details write a block of data blk of size blksize into the filesystem fs
 * in block number blocknum
 */
int fs_write_block(struct fs_filesyst fs, int blocknum, const void* blk, size_t blksize) {
	/* checking the params */
	int fs_size = fs.tot_size;
	if(blocknum * FS_BLOCK_SIZE >= fs_size || blocknum < 0) {/* blocknum too big or too small */
		fprintf(stderr, "fs_write_block: Cannot write block, invalid blocknum %d!", blocknum);
		return FUNC_ERROR;
	}
	if(blksize > FS_BLOCK_SIZE) { /* blksize too big */
		fprintf(stderr, "fs_write_block: Cannot write block, data too big %d!", size);
		return FUNC_ERROR;
	}
	
	
	/* main functionality */
	int fd = fs.fd;
	/* goto the specified block */
	if(lseek(fd, blocknum * FS_BLOCK_SIZE, SEEK_SET) < 0) {
		perror("fs_write_block: lseek error!");
		return FUNC_ERROR;
	}
	
	/* write the data */
	if(write(fd, blk, blksize) != blksize) { /* write didn't write blksize bytes */
		perror("fs_write_block: write error!");
		return FUNC_ERROR;
	}
	
	return 0;
}

int fs_read_block(struct fs_filesyst fs, int blocknum, void* blk) {
	/* checking the params */
	int fs_size = fs.tot_size;
	if(blocknum * FS_BLOCK_SIZE >= fs_size || blocknum < 0) {/* blocknum too big or too small */
		fprintf(stderr, "fs_read_block: Cannot read block, invalid blocknum %d!", blocknum);
		return FUNC_ERROR;
	}
	
	/* main functionality */
	int fd = fs.fd;
	
	/* goto the specified block */
	if(lseek(fd, blocknum * FS_BLOCK_SIZE, SEEK_SET) < 0) {
		perror("fs_read_block: lseek error!");
		return FUNC_ERROR;
	}
	
	/* read the data */
	if(read(fd, blk, FS_BLOCK_SIZE) != FS_BLOCK_SIZE) { /* read didn't read all bytes */
		perror("fs_read_block: read error!");
		return FUNC_ERROR;		
	}
	
	return 0;
}
