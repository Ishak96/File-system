/**
 * @file shell.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief main dirent functions
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <errno.h>

#include <fs.h>
#include <ui.h>
#include <disk.h>
#include <io.h>
#include <devutils.h>
#include <dirent.h>

int main(int argc, char *argv[])
{

	char line[1024];
	int quit = 0;
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

	while(!quit){

		if(!fgets(line,sizeof(line),stdin))
			quit = 1;

	}

	closedir_(rootdir);
	close_(filepswd);
	closefs();
	return 0;
}
