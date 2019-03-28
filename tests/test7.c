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
	initfs("./bin/partition", 100000, 1);
	
	DIR_* dir = opendir_("/DIR", 1, 0);
	closedir_(dir);
	dir = opendir_("/DIR/SUBDIR", 1, 0);
	closedir_(dir);
	int filefd = open_("/DIR/FILE", 1, 0);
	int filefd1 = open_("/DIR/FILE2", 1, 0);
	int filefd3 = open_("/DIR/SUBDIR/FILE3", 1, 0);
	close_(filefd3);
	int value = 12312313;
	write_(filefd1, &value, sizeof(int));
	value = 0;
	
	lseek_(filefd1, 0);
	read_(filefd1, &value, sizeof(int));
	printf("The value written is %d\n", value);
	value = 0;
	printf("BEFORE\n");
	ls_("/");
	ls_("/DIR");

	printf("AFTER REMOVE OF /DIR/FILE\n");
	rm_("/DIR/FILE");
	ls_("/");
	ls_("/DIR");
	
	printf("AFTER ln /dir/FILE2 /FILE\n");
	ln_("/DIR/FILE2", "/FILE");
	ls_("/");
	ls_("/DIR");

	printf("AFTER mv /FILE /FILE2\n");
	mv_("/FILE", "/FILE2");
	ls_("/");
	ls_("/DIR");

	printf("AFTER mv /FILE2 /DIR/FILE\n");
	mv_("/FILE2", "/DIR/FILE");
	ls_("/");
	ls_("/DIR");
	
	printf("AFTER ln /DIR/FILE2 /DIR/FILE (note: the error is meant)\n");
	ln_("/DIR/FILE2", "/DIR/FILE");
	ls_("/");
	ls_("/DIR");
	
	printf("AFTER mv /DIR/SUBDIR /NEWDIR\n");
	mv_("/DIR/SUBDIR", "/NEWDIR");
	ls_("/");
	ls_("/DIR");
	ls_("/NEWDIR");

	int filefd2 = open_("/DIR/FILE2", 0, 0);

	lseek_(filefd2, 0);
	read_(filefd2, &value, sizeof(int));
	printf("The value written is %d\n", value);

	printf("AFTER rmdir /DIR\n");
	rmdir_("/DIR", 1);
	ls_("/");
	
	printf("note: this error is meant\n");
	ls_("/DIR");

	close_(filefd);
	close_(filefd1);
	close_(filefd2);
	closefs();
	return 0;
}
