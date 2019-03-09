/**
 * @file test1.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <fs.h>
#include <disk.h>


/**
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief program to test the disk functions
 */
int main(int argc, char** argv) {
	char filename[512] = "./bin/partition";
	struct fs_filesyst fs;
	
	printf("Creating filesyst..\n");
	/* test creatfile */
	int ret = creatfile(filename, 100000, &fs);
	assert(ret == 0);
	fs_format(fs);

	printf("writing block..\n");
	/* test write */
	union fs_block blk;
	for(int i=0; i<FS_POINTERS_PER_BLOCK; i++) {
		blk.pointers[i] = 123;
	}
	fs_write_block(fs, 0, &blk, FS_BLOCK_SIZE);
	
	printf("resetting block..\n");
	/* reset blk values */
	memset(&blk, 0, FS_BLOCK_SIZE);
	
	printf("reading block..\n");
	/* test read */
	fs_read_block(fs, 0, &blk);
	int check = 0;
	for(int i=0; i<FS_POINTERS_PER_BLOCK; i++) {
		check += 123 - blk.pointers[i];
	}
	assert(check == 0);
	
	printf("closing..\n");
	/* test disk_close */
	disk_close(&fs);
	
	return 0;
}
