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
	opendir_creat(fs, super, &dirino, S_DIR, "/"); // open two dirs
	int dirfd = io_open_fd(dirino);
	printf("created /\n");
	
	uint32_t childino;
	opendir_creat(fs, super, &childino, S_DIR, "/BANANA"); // open two dirs
	int childfd = io_open_fd(childino);
	printf("created /BANANA\n");
	uint32_t childfileino;
	open_creat(fs, super, &childfileino, 0, "/BANANA/FILE");
	open_creat(fs, super, &childfileino, 0, "/BANANA/FILE2");
	
	
	uint32_t dchildino;
	opendir_creat(fs, super, &dchildino, S_DIR, "/BANANA/BANANA1");
	int dchildfd = io_open_fd(dchildino);
	printf("created /BANANA/BANANA1\n");
	struct dirent ent = {0};

	/* ls root */
	printf("ls of /:\n");
	io_lseek(fs, super, dirfd, 0);
	int size = 0;
	io_read(fs, super, dirfd, &size, sizeof(int));
	printf("dir size = %d\n", size);
	
	for(int i=0; i<size; i++) {
		io_read(fs, super, dirfd, &ent, sizeof(struct dirent));
		printf("%d %d %s\n", ent.d_ino, ent.d_type, ent.d_name);
	}
	/* * */

	//~ char abc[] = "/BANANA";
	//~ int fd = findpath(fs, super, abc);
	//~ printf("FD = %d\n", fd);

	/* ls /BANANA */
	printf("\nls of /BANANA:\n");
	io_lseek(fs, super, childfd, 0);
	size = 0;
	io_read(fs, super, childfd, &size, sizeof(int));
	printf("dir size = %d\n", size);
	
	for(int i=0; i<size; i++) {
		io_read(fs, super, childfd, &ent, sizeof(struct dirent));
		printf("%d %d %s\n", ent.d_ino, ent.d_type, ent.d_name);
	}
	/* * */
	
	/* ls /BANANA/BANANA1 */
	printf("\nls of /BANANA/BANANA1:\n");
	io_lseek(fs, super, dchildfd, 0);
	size = 0;
	io_read(fs, super, dchildfd, &size, sizeof(int));
	printf("dir size = %d\n", size);
	
	for(int i=0; i<size; i++) {
		io_read(fs, super, dchildfd, &ent, sizeof(struct dirent));
		printf("%d %d %s\n", ent.d_ino, ent.d_type, ent.d_name);
	}
	/* * */

	disk_close(&fs);
	return 0;
}
