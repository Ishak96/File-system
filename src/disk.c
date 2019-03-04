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

/**
 * @brief creat a disk image for storing the disk data
 * @details if this function is called on a disk image
 * that already exists, the function will retun -1, 
 * otherwise it will initialize the fs_filesyst struct 
 * @see fs_filesyst
 * @param filename partition name
 * @param n the number of blocks
 * @param fs virtual filesystem structure
 */
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
	fs->mounts = 0;
	
	return 0;
}

/**
* @brief discover the number of blocks on the disk
* @param fs virtual filesystem structure
* @return the virtual filesystem number of blocks
*/
int disk_size(fs_filesyst fs){
	return fs.nblocks;
}

/**
* @brief release the file
* @details this function close the file descriptor using the virtual filesystem structure 
* @param fs virtual filesystem structure
*/
void disk_close(fs_filesyst* fs){
	if(fs->fd) {
		printf("%d disk block reads\n",fs->nreads);
		printf("%d disk block writes\n",fs->nwrites);
		close(fs->fd);
		fs->fd = 0;
	}
}

void disk_write(int blocknum, const char *data, fs_filesyst* fs){

}

void disk_read(int blocknum, char *data, fs_filesyst* fs){

}