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
	
	int filefd = open_("/FILE", 0);
	
	int value = 12312313;
	write_(filefd, &value, sizeof(int));
	value = 0;
	
	lseek_(filefd, 0);
	read_(filefd, &value, sizeof(int));
	
	printf("The value written is %d\n", value);
	ls_("/");
	int fd = open_("/", 0);
	close_(filefd);
	closefs();
	return 0;
}
