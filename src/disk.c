/**
 * @file disk.c
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
#include <sys/types.h>

/**
 * @brief creat a disk image for storing the disk data
 * @details if this function is called on a disk image
 * that already exists, the function will retun -1, 
 * otherwise it will initialize the fs_filesyst struct 
 * @see fs_filesyst
 * @param filename partition name
 * @param n the number of blocks
 * @param fs virtual filesystem structure
 */
int creatfile(const char* filename, size_t size, struct fs_filesyst* fs) {
	if(size <= 0) {
		perror("creatfile: null size");
		return FUNC_ERROR;
	}
	
	fs->fd = open(filename, O_RDWR | O_CREAT, 0777);
	if(fs->fd < 0) {
		perror("creatfile: open");
		return FUNC_ERROR;
	}
	lseek(fs->fd, size-1, SEEK_SET);
	write(fs->fd, "\0", 1);

	fs->tot_size = size;
	fs->nblocks = size / FS_BLOCK_SIZE;

	return 0;
}

/**
* @brief discover the number of blocks on the disk
* @param fs virtual filesystem structure
* @return the virtual filesystem number of blocks
*/
int disk_size(struct fs_filesyst fs){
	return fs.nblocks;
}

/**
* @brief release the file
* @details this function close the file descriptor using the virtual filesystem structure 
* @param fs virtual filesystem structure
*/
void disk_close(struct fs_filesyst* fs){
	if(fs->fd) {
		close(fs->fd);
		fs->fd = 0;
	}
}

/**
 * @brief write a chunk of data into a block
 * @details write a block of data blk of size blksize into the filesystem fs
 * in block number blocknum
 */
int fs_write_block(struct fs_filesyst fs, int blocknum, const void* blk, size_t blksize) {
	/* checking the params */
	if(blocknum >= fs.nblocks || blocknum < 0) {
		/* blocknum too big or too small */
		fprintf(stderr, 
			   "fs_write_block: Cannot write block, invalid blocknum %d!",
			   blocknum);
		return FUNC_ERROR;
	}
	if(blksize > FS_BLOCK_SIZE) { /* blksize too big */
		fprintf(stderr, "fs_write_block: Cannot write block, data too big %ld!", blksize);
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

/**
 * @brief read a chunk of data from a filesystem
 * @details read a block of data blk from the filesystem fs
 * from block number blocknum
 */
int fs_read_block(struct fs_filesyst fs, int blocknum, void* blk) {
	/* checking the params */
	if(blocknum  >= fs.nblocks || blocknum < 0) {
		/* blocknum too big or too small */
		fprintf(stderr,
			   "fs_read_block: Cannot read block, invalid blocknum %d!",
				blocknum);
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
