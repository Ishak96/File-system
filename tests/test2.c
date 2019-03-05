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

	printf("formatting superblock..\n");
	fs_format_super(fs);
	
	printf("dumping superblock\n");
	fs_dump_super(fs);
	
	return 0;
}
