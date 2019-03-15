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
#include <io.h>
#include <devutils.h>
#include <time.h>
#include <dirent.h>
/**
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief program to test the disk functions
 */
int main(int argc, char** argv) {
	srand(time(NULL));
	char filename[512] = "./bin/partition";
	struct fs_filesyst fs;
	
	printf("Creating filesyst..\n");
	/* test creatfile */
	creatfile(filename, 100000, &fs);

	fs_format(fs);

	/* read the super block*/
	union fs_block blk;
	struct fs_super_block super;
	if(fs_read_block(fs, 0, &blk) < 0) {
		fprintf(stderr, "fs_format: fs_read_block\n");
		return FUNC_ERROR;
	}
	super = blk.super;

	int dirfd = opendir(fs, super);
	if(dirfd < 0)  {
		printf("NOO\n");
		return 0;
	}
	struct dirent ent = {
		.d_ino = 323232,
		.d_type = 12,
		.d_name = "FILENAME"
	};
	insertFile(fs, super, dirfd, ent);
	disk_close(&fs);
	return 0;
}


