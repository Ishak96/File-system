#ifndef DISK_H
#define DISK_H
#include <stdint.h>

#define BLOCK_SIZE 4096

typedef struct{
	uint32_t fd;
	uint32_t tot_size;
	uint32_t nblocks;
	uint32_t nreads;
	uint32_t nwrites;
}fs_filesyst;

int creatfile(const char* filename, size_t n, fs_filesyst* fs);
int disk_size(fs_filesyst fs);
void disk_close(fs_filesyst* fs);
void disk_write(int blocknum, const char *data, fs_filesyst* fs);
void disk_read(int blocknum, char *data, fs_filesyst* fs);
#endif