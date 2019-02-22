#ifndef MEMINIT_H
#define MEMINIT_H
#include <stdint.h>

struct fs_super_block  {
	uint32_t inode_count;
	uint32_t block_count;
	uint32_t free_inode_count;
	uint32_t free_block_count;

	uint32_t ngroup;
	
	uint32_t inodes_per_group;
	uint32_t blocks_per_group;
	
	uint32_t mtime;	
	uint32_t wtime;
};

struct fs_filesyst {
	uint32_t fd;
	uint32_t tot_size;
	
	uint32_t block_size;
};

int creatfile(const char* filename, size_t size);
#endif
