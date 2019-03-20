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

	uint32_t no;
	struct fs_inode ind;
	memset(&ind, 0, sizeof(ind));
	ind.mode = 123;
	ind.uid = 123;
	ind.gid = 123;
	ind.atime = 123;
	ind.mtime = 123;	
	fs_alloc_inode(fs, &super, &no);
	fs_write_inode(fs, super, no, &ind);
	int fd = io_iopen(fs, super, no);
	fs_dump_super(fs);
	printf("fd = %d\n", fd);
	char str[4096*3] = {0};
	char str2[4096*3] = {0};
	for(int i=0; i<4096*3-1; i++) {
		str[i] = 'A' + rand()%26;
		str2[i] = str[i];
	}
	str[4096*3-1] = '\0';
	str2[4096*3-1] = '\0';

	io_lseek(fs, super, fd, 128);
	io_write(fs, super, fd, str, 400);
	io_lseek(fs, super, fd, 4096*6+128);
	io_write(fs, super, fd, str, sizeof(str));
	io_lseek(fs, super, fd, 4096*12+128);
	io_write(fs, super, fd, str, sizeof(str));

	memset(str, 0, sizeof(str));
	io_lseek(fs, super, fd, 128);
	
	char strtest[4096*15] = {0};
	io_read(fs, super, fd, strtest, sizeof(strtest));
	printf("1/%d\n", strcmp(strtest+4096*6, str2));
	printf("2/%d\n", strcmp(strtest+4096*12, str2));
	str2[400] = '\0',
	printf("3/%ld %d\n",strlen(strtest), strcmp(strtest, str2));
	//~ printf("str = %ld %d\n", strlen(str), strcmp(str, str2));

	disk_close(&fs);
	return 0;
}


