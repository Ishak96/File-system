#ifndef DISK_H
#define DISK_H
#include <stdint.h>

#define BLOCK_SIZE 4096
/**
 * @brief virtual filesystem structure
 * @details contains information about the file used to simulate a disk
 * partition
 */
typedef struct{
	uint32_t fd;      /**< file descriptor */
	uint32_t tot_size;/**< total size of our file (partition) */
	uint32_t nblocks; /**< number of blocks in disk image*/
	uint32_t nreads;  /**< number of reads performed */
	uint32_t nwrites; /**< number of writes performed*/
	uint32_t mounts;  /**< number of mounts*/
}fs_filesyst;

int creatfile(const char* filename, size_t n, fs_filesyst* fs);
int disk_size(fs_filesyst fs);
void disk_close(fs_filesyst* fs);
void disk_write(int blocknum, const char *data, fs_filesyst* fs);
void disk_read(int blocknum, char *data, fs_filesyst* fs);
#endif
