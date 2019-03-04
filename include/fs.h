#ifndef FS_H
#define FS_H
#include <stdint.h>
#include <disk.h>

#define MAGIC_NUMBER 0xf0f03410
#define INODES_PER_BLOCK 128
#define POINTERS_PER_INODE 5
#define POINTERS_PER_BLOCK 1024

struct fs_super_block  {
	uint32_t inode_count;
	uint32_t block_count;
	uint32_t free_inode_count;
	uint32_t free_block_count;
	
	uint32_t mtime;	
	uint32_t wtime;
};

struct fs_inode {
    uint32_t valid;		// Whether or not inode is valid
    uint32_t size;		// Size of file
	uint32_t direct[POINTERS_PER_INODE]; // Direct pointers
	uint32_t indirect;	// Indirect pointer
};

union fs_block {
	struct fs_super_block  super;			    // Superblock
    struct fs_inode	       inodes[INODES_PER_BLOCK];	    // Inode block
    uint32_t    		   pointers[POINTERS_PER_BLOCK];   // Pointer block
	char	    		   data[BLOCK_SIZE];	    // Data block
};

#endif
