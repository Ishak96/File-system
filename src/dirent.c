/**
 * @file dirent.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief main dirent functions
 */
#include <io.h>
#include <fs.h>
#include <devutils.h>
#include <disk.h>
#include <dirent.h>

#include <string.h>
#include <stdint.h>
#include <stdio.h>

int opendir(struct fs_filesyst fs, struct fs_super_block super) {
	uint16_t mode = 0;
	mode |= S_DIR;

	int fd = io_open_creat(fs, super, mode);
	if(fd < 0) {
		fprintf(stderr, "opendir: io_open_creat\n");
		return FUNC_ERROR;
	}
	int size = 0;
	if(io_write(fs, super, fd, &size, sizeof(int)) < 0) {
		fprintf(stderr, "opendir: io_write\n");
		return FUNC_ERROR;
	}

	return fd;
}

int getFiles(struct fs_filesyst fs, struct fs_super_block super, 
		     int dirfd, struct dirent** files, int* size)
{
	if(size == NULL || dirfd < 0) {
		fprintf(stderr, "getFile: invalid arguments\n");
		return FUNC_ERROR;
	}
	
	*size = 0;
	io_lseek(fs, super, dirfd, 0);
	if(io_read(fs, super, dirfd, size, sizeof(int)) < 0) {
		fprintf(stderr, "getFiles: io_read\n");
		return FUNC_ERROR;
	}
	*files = malloc(sizeof(struct dirent) * (*size));
	if(files == NULL) {
		fprintf(stderr, "getFiles: malloc err!\n");
		return FUNC_ERROR;
	}
	if(*size > 0 && io_read(fs, super, dirfd, *files, sizeof(struct dirent) * (*size))) {
		fprintf(stderr, "getFile: io_read\n");
		free(*files);
		return FUNC_ERROR;
	}
	return 0;
}

int insertFile(struct fs_filesyst fs, struct fs_super_block super,
			   int dirfd, struct dirent file)
{
	if(dirfd < 0) {
		fprintf(stderr, "insertFile: invalid arguments\n");
		return FUNC_ERROR;
	}
	
	struct dirent* files = NULL;
	int size = 0;
	if(getFiles(fs, super, dirfd, &files, &size) < 0) {
		fprintf(stderr, "insertFile: invalid arguments\n");
		return FUNC_ERROR;
	}

	int i = size-1;
	if(i >= 0 && strcmp(file.d_name, files[i].d_name) > 0) {
		struct dirent temp = file;
		file = files[i];
		files[i] = temp;
	}
	while(i-1 >= 0 && strcmp(files[i].d_name, files[i-1].d_name) > 0) {
		struct dirent temp = files[i];
		files[i] = files[i-1];
		files[i-1] = temp;
		i++;
	}

	if(size != 0 && io_write(fs, super, dirfd, files, sizeof(struct dirent) * size)) {
		fprintf(stderr, "insertFile: io_write\n");
		free(files);
		return FUNC_ERROR;
	}
	if(io_write(fs, super, dirfd, &file, sizeof(struct dirent))) {
		fprintf(stderr, "insertFile: io_write\n");
		free(files);
		return FUNC_ERROR;
	}

	io_lseek(fs, super, dirfd, 0);
	size ++;
	
	if(io_write(fs, super, dirfd, &size, sizeof(int)) < 0) {
		fprintf(stderr, "insertFile: io_write\n");
		free(files);
		return FUNC_ERROR;
	}

	free(files);
	return 0;
}


