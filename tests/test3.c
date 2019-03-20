/**
 * @file test2.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <fs.h>
#include <disk.h>
#include <devutils.h>

/**
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief program to test filesystem functions
 */
int main(int argc, char** argv) {
	char cwd[512] = "./bin/partition";
	struct fs_filesyst fs;
	printf("Creating filesyst..\n");
	creatfile(cwd, 100000, &fs);

	printf("formatting filesystem..\n");
	fs_format(fs);

	/* read the super block*/
	union fs_block blk;
	struct fs_super_block super;
	if(fs_read_block(fs, 0, &blk) < 0) {
		fprintf(stderr, "fs_format: fs_read_block\n");
		return FUNC_ERROR;
	}
	super = blk.super;

	uint32_t no;
	struct fs_inode ind;
	memset(&ind, 0, sizeof(ind));
	ind.mode = 123;
	ind.uid = 123;
	ind.gid = 123;
	ind.atime = 123;
	ind.mtime = 123;
	for(int i=0; i<60; i++) {
		fs_alloc_inode(fs, &super, &no);
		fs_write_inode(fs, super,no, &ind);
	}

	fs_free_inode(fs, &super, 0);
	
	uint32_t data[20];
	
	union fs_block b[20];
	memset(&b, 0, sizeof(b));

	fs_alloc_data(fs, &super, data, 20);

	int nbdata = 20;
	int nbinode = 60;

	for(int i=0; i<nbdata; i+=2)
		fs_free_data(fs, &super, i);
	for(int i=0; i<nbinode; i+=2)
		fs_free_inode(fs, &super, i);

	printf("data is allocated ...\n");
	for(int i=0; i<nbdata; i++)
		printf("data[%d]===> is %s\n", i, fs_is_data_allocated(fs, super, i)? "allocated" : "not allocated");

	printf("inode is allocated ...\n");
	for(int i=0; i<nbinode; i++)
		printf("inode[%d]===> is %s\n", i, fs_is_inode_allocated(fs, super, i)? "allocated" : "not allocated");
	
	disk_close(&fs);

	return 0;
}