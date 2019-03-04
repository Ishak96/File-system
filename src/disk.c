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
#include <disk.h>

int creatfile(const char* filename, size_t n, fs_filesyst* fs) {
	if(n <= 0) {
		die("creatfile: null size");
	}
	
	fs->fd = open(filename, O_RDWR | O_CREAT, 0777);
	if(fs->fd < 0) {
		die("creatfile: open");
	}
	
	lseek(fs->fd, (n-1)*BLOCK_SIZE, SEEK_SET);
	write(fs->fd, "\0", 1);

	fs->tot_size = n*BLOCK_SIZE;
	fs->nblocks = n;
	fs->nreads = 0;
	fs->nwrites = 0;
	
	return 1;
}

int disk_size(fs_filesyst fs){
	return fs.nblocks;
}

void disk_close(fs_filesyst* fs){
	if(fs->fd) {
		printf("%d disk block reads\n",fs->nreads);
		printf("%d disk block writes\n",fs->nwrites);
		close(fs->fd);
		fs->fd = 0;
	}
}