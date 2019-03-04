/**
 * @file meminit.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief initializing the partition
 * @details initializing the partition files and utility functions to interact with the os
 */
#include <devutils.h>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int creatfile(const char* filename, size_t size) {
	if(size <= 0) {
		die("creatfile: null size");
	}
	
	int fd = open(filename, O_RDWR | O_CREAT, 0777);
	if(fd < 0) {
		die("creatfile: open");
	}
	
	lseek(fd, size-1, SEEK_SET);
	write(fd, "\0", 1);
	return fd;
}