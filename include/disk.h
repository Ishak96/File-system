/**
 * @file disk.h
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief main functions for interacting with the os
 * @details constains structs and prototypes used to manipulate the 
 * virtual filesystem
 */
#ifndef DISK_H
#define DISK_H
#include <stdint.h>
#include <stdlib.h>

#define FS_BLOCK_SIZE 4096 			   /* block size in bytes */

/**
 * @brief virtual filesystem structure
 * @details contains information about the file used to simulate a disk
 * partition
 */
struct fs_filesyst{
	uint32_t fd;      /**< file descriptor */
	uint32_t tot_size;/**< total size of our file (partition) */
	uint32_t nblocks; /**< number of blocks in disk image*/
};
int fs_check_magicnum(int fd);
int creatfile(const char* filename, size_t size, struct fs_filesyst* fs);
int disk_size(struct fs_filesyst fs);
void disk_close(struct fs_filesyst* fs);
int fs_write_block(struct fs_filesyst fs, int blocknum, const void* blk, size_t blksize);
int fs_read_block(struct fs_filesyst fs, int blocknum, void* blk);
#endif
