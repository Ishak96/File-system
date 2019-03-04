#ifndef DISK_H
#define DISK_H
#include <stdint.h>

#define BLOCK_SIZE 4096

struct fs_filesyst {
	uint32_t fd;
	uint32_t tot_size;
};

int creatfile(const char* filename, size_t size);
#endif