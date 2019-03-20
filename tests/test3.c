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

	int nbdata = 20;
	int nbinode = 64;
	for(int i=0; i<nbinode; i++)
		fs_alloc_inode(fs, &super, &no);

	uint32_t data[nbdata];
	
	union fs_block b[nbdata];
	memset(&b, 0, sizeof(b));

	fs_alloc_data(fs, &super, data, nbdata);
	for(int i=1; i<nbdata; i+=2)
		fs_free_data(fs, &super, i);
	for(int i=0; i<nbinode; i+=2){
		fs_free_inode(fs, &super, i);
	}

	printf("data is allocated ...\n");
	int nballoc,nbnotalloc;
	nballoc = nbnotalloc = 0;
	for(int i=1; i<nbdata; i++){
		if(fs_is_data_allocated(fs, super, i))
			nballoc += 1;
		else
			nbnotalloc += 1;
	}
	printf("their is [%d] data allocated and [%d] not allocated\n", nballoc, nbnotalloc);

	printf("inode is allocated ...\n");
	nballoc = nbnotalloc = 0;
	for(int i=0; i<nbinode; i++){
		if(fs_is_inode_allocated(fs, super, i))
			nballoc += 1;
		else
			nbnotalloc += 1;
	}
	printf("their is [%d] inode allocated and [%d] not allocated\n", nballoc, nbnotalloc);
	
	disk_close(&fs);

	return 0;
}