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

#include <libgen.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

int formatdir(struct fs_filesyst fs, struct fs_super_block super) {
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
	if(*size > 0 && io_read(fs, super, dirfd, *files, sizeof(struct dirent) * (*size)) < 0) {
		fprintf(stderr, "getFile: io_read\n");
		free(*files);
		return FUNC_ERROR;
	}
	return 0;
}

int insertFile(struct fs_filesyst fs, struct fs_super_block super,
			   int dirfd, struct dirent file)
{
	/* todo: add test for name integrity */
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
	if(i >= 0 && strcmp(file.d_name, files[i].d_name) < 0) {
		struct dirent temp = file;
		file = files[i];
		files[i] = temp;
	}
	while(i-1 >= 0 && strcmp(files[i].d_name, files[i-1].d_name) < 0) {
		struct dirent temp = files[i];
		files[i] = files[i-1];
		files[i-1] = temp;
		i++;
	}
	io_lseek(fs, super, dirfd, sizeof(int));
	if(size > 0 && io_write(fs, super, dirfd, files, sizeof(struct dirent) * size) < 0) {
		fprintf(stderr, "insertFile: io_write\n");
		free(files);
		return FUNC_ERROR;
	}

	if(io_write(fs, super, dirfd, &file, sizeof(struct dirent)) < 0) {
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

int findFile(struct fs_filesyst fs, struct fs_super_block super,
			   int dirfd, char* filename, struct dirent *res, int* idx)
{
	if(dirfd < 0 && strlen(filename) < 256) {
		fprintf(stderr, "findFile: invalid arguments\n");
		return FUNC_ERROR;
	}
	
	struct dirent* files = NULL;
	int size = 0;
	if(getFiles(fs, super, dirfd, &files, &size) < 0) {
		fprintf(stderr, "findFile: invalid arguments\n");
		return FUNC_ERROR;
	}
	int i, j, m;
	int found = 0;
	for(i=0, j=size-1; i<=j;) {
		m = (i+j)/2;
		int cmp = strcmp(filename, files[m].d_name);
		
		if(cmp == 0) {
			found = 1;
			break;
		} else if(cmp < 0) {
			j = m-1;
		} else {
			i = m+1;
		}
	}
	if(!found) {
		struct dirent temp = {0};
		temp.d_ino = -1;
		*res = temp;
		*idx = -1;
	} else {
		*res = files[m];
		*idx = m;
	}
	free(files);
	return 0;
}

int delFile(struct fs_filesyst fs, struct fs_super_block super,
			   int dirfd, char* filename)
{
	int idx = 0;
	struct dirent res;
	if(findFile(fs, super, dirfd, filename, &res, &idx) < 0) {
		fprintf(stderr, "delFile: findFile\n");
		return FUNC_ERROR;
	}
	if(idx < 0) {
		return FUNC_ERROR;
	}
	struct dirent* files = NULL;
	int size = 0;
	if(getFiles(fs, super, dirfd, &files, &size) < 0) {
		fprintf(stderr, "findFile: invalid arguments\n");
		return FUNC_ERROR;
	}
	if(size > 0) {
		while(idx+1 < size) {
			files[idx] = files[idx+1];
			idx ++;
		}
		io_lseek(fs, super, dirfd, sizeof(int));
		if(size > 0 && io_write(fs, super, dirfd, files, sizeof(struct dirent) * (size-1)) < 0) {
			fprintf(stderr, "insertFile: io_write\n");
			free(files);
			return FUNC_ERROR;
		}
		io_lseek(fs, super, dirfd, 0);
		size --;
		
		if(io_write(fs, super, dirfd, &size, sizeof(int)) < 0) {
			fprintf(stderr, "insertFile: io_write\n");
			free(files);
			return FUNC_ERROR;
		}
	}

	free(files);
	return 0;
}

int findpath(struct fs_filesyst fs, struct fs_super_block super, char* filename) {
	if(!strcmp(filename, "/")) {
		return io_open_fd(0);
	}
	char delim[2] = "/";
	char* tok = strtok(filename, delim);

	uint32_t dir = 0; // the root directory always has the inodenum 0
	int dirfd = io_open_fd(dir);
	struct dirent filefound = {0};
	int idx;

	while(tok != NULL) {
		if(findFile(fs, super, dirfd, tok, &filefound, &idx) < 0) {
			fprintf(stderr, "findpath: findFile\n");
			return FUNC_ERROR;
		}
		if(filefound.d_ino == -1) {
			fprintf(stderr, "findpath: file not found\n");
			return FUNC_ERROR;
		}
		dir = filefound.d_ino;
		// io_close_fd(dirfd);
		dirfd = io_open_fd(dir);
		tok = strtok(NULL, delim);
	}

	return dirfd;
}

int opendir(struct fs_filesyst fs, struct fs_super_block super, const char* filepath) {
	int dirfd = formatdir(fs, super); // the created dir fd
	//~ printf("[%s] cur = %d\n", filepath, dirfd);
	if(dirfd < 0) {
		fprintf(stderr, "creatdir: formatdir\n");
		return FUNC_ERROR;
	}
	uint32_t curino = io_getino(dirfd);
	struct dirent cur = {0};
	
	// put the . in the created directory
	cur.d_ino = curino;
	cur.d_type = S_DIR;	
	strcpy(cur.d_name, ".");
	if(insertFile(fs, super, dirfd, cur) < 0) {
		fprintf(stderr, "creatdir: insertFile\n");
		return FUNC_ERROR;
	}

	// get parent path and file basename
	char* path_copy1 = strdup(filepath); // note: we need to copies because the
	char* path_copy2 = strdup(filepath); // basename and dirname funcs change the values of the strings
	char* child_name = basename(path_copy1);
	char* parent_path = dirname(path_copy2);
	//~ printf("[%s] %s -- %s\n", filepath, parent_path, child_name);
	
	int parent = findpath(fs, super, parent_path); // get the parent's fd
	//~ printf("[%s] parent = %d\n", filepath, parent);
	
	// make .. in the created dir as the parent
	cur.d_ino = io_getino(parent);
	strcpy(cur.d_name, "..");
	if(insertFile(fs, super, dirfd, cur) < 0) {
		fprintf(stderr, "creatdir: insertFile\n");
		return FUNC_ERROR;
	}
	
	// put the current dir in the parent
	if(strcmp("/", filepath)) {  // the root dir is a special case
		cur.d_ino = io_getino(dirfd);
		strcpy(cur.d_name, child_name);
		if(insertFile(fs, super, parent, cur) < 0) {
			fprintf(stderr, "creatdir: insertFile\n");
			return FUNC_ERROR;
		}
		// io_close_fd(parent); // close the parent fd
	}
	
	free(path_copy1);
	free(path_copy2);
	
	return dirfd;
}

