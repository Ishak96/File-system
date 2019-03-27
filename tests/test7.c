/**
 * @file test1.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <fs.h>
#include <ui.h>
#include <disk.h>
#include <io.h>
#include <devutils.h>
#include <dirent.h>
/**
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief program to test the disk functions
 */
int main(int argc, char** argv) {
	initfs("./bin/partition", 100000);
	
	DIR_* rootdir = opendir_("/", 0);
	int filefd = open_("/FILE", 0);
	int filefd1 = open_("/FILE2", 0);
	
	int value = 12312313;
	write_(filefd, &value, sizeof(int));
	value = 0;
	
	lseek_(filefd, 0);
	read_(filefd, &value, sizeof(int));
	rm_("/FILE2");
	
	printf("The value written is %d\n", value);
	
	//~ printf("ls of /DIR\n");
	//~ ls_(dir);
	printf("ls of /\n");
	ls_(rootdir);
	
	//~ open_("/", 0);
	
	//~ closedir_(dir);
	//~ closedir_(rootdir);
	close_(filefd);
	close_(filefd1);
	closefs();
	return 0;
}
