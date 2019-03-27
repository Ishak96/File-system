/**
 * @file shell.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief main dirent functions
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fs.h>
#include <ui.h>
#include <disk.h>
#include <io.h>
#include <devutils.h>
#include <dirent.h>

int main(int argc, char *argv[])
{

	char id[100], password[100];
	char* root = "root";

	if(argc!=3) {
		printf("use: %s <disk> <nblocks>\n",argv[0]);
		return 1;
	}

	if(initfs(argv[1],atoi(argv[2])) < 0) {
		fprintf(stderr,"shell : creatfile %s\n",argv[1]);
		return 1;
	}
	printf("opened emulated disk image %s\n",argv[1]);

	DIR_* rootdir = opendir_("/", 0);
	int filepswd = open_("/passwd", 0);
	write_(filepswd, root, sizeof(root));

	printf("Username :\n");
	gets(id);
	printf("Password :\n");
	gets(password);

	printf("%s, %s\n", id, password);

	closedir_(rootdir);
	close_(filepswd);
	closefs();
	return 0;
}
