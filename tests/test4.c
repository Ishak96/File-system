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
	uint32_t dirino;
	formatdir(fs, super, &dirino, S_DIR);
	int dirfd = io_open_fd(dirino);
	struct dirent ent = {
		.d_ino = 323232,
		.d_type = 12,
		.d_name = "FILENAME"
	};
	
	insertFile(fs, super, dirino, ent);
	strcpy(ent.d_name, "BANANA");
	insertFile(fs, super, dirino, ent);
	strcpy(ent.d_name, "BANANA1");
	insertFile(fs, super, dirino, ent);
	strcpy(ent.d_name, "BANANA2");
	insertFile(fs, super, dirino, ent);
	strcpy(ent.d_name, "BANANA3");
	insertFile(fs, super, dirino, ent);
	strcpy(ent.d_name, "BANANA4");
	insertFile(fs, super, dirino, ent);
	
	//io_lseek(fs, super, dirfd, 0);
	delFile(fs, super, dirino, "BANANA4");

	io_lseek(fs, super, dirfd, 0);
	int size = 0;
	io_read(fs, super, dirfd, &size, sizeof(int));
	printf("%d\n", size);
	
	for(int i=0; i<size; i++) {
		io_read(fs, super, dirfd, &ent, sizeof(struct dirent));
		printf("%d %d %s\n", ent.d_ino, ent.d_type, ent.d_name);
	}
	
	int idx;
	findFile(fs, super, dirfd, "BANANA4", &ent, &idx);
	printf("FOUND %d %d %s\n", ent.d_ino, ent.d_type, ent.d_name);
	findFile(fs, super, dirfd, "BANANA3", &ent, &idx);
	printf("FOUND %d %d %s\n", ent.d_ino, ent.d_type, ent.d_name);

	disk_close(&fs);
	return 0;
}


