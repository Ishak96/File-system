#include <io.h>
#include <fs.h>
#include <devutils.h>
#include <disk.h>
#include <dirent.h>

#include <libgen.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>

struct fs_filesyst fs;
struct fs_super_block super;

int cur_user = 0;

int initfs(char* filename, size_t size) {
	srand(time(NULL));
	printf("Creating filesyst..\n");
	/* test creatfile */
	if(creatfile(filename, size, &fs) < 0) {
		fprintf(stderr, "initfs: can't create file %s\n", filename);
	}
	printf("formatting..\n");
	if(fs_format(fs) < 0) {
		fprintf(stderr, "initfs: can't fomat partition to file %s\n", filename);
	}
	
	union fs_block blk;
	if(fs_read_block(fs, 0, &blk) < 0) {
		fprintf(stderr, "fs_format: fs_read_block\n");
		return FUNC_ERROR;
	}
	super = blk.super;

	uint32_t dirino;
	if(opendir_creat(fs, super, &dirino, S_DIR, "/") < 0) {
		fprintf(stderr, "initfs: opendir_creat\n");
		return FUNC_ERROR;
	}
	printf("Creating the root directory.. %u\n", dirino);
	
	return 0;
}


int getParentInode(const char* filepath, uint32_t* parentino) {
	char* filename_copy = strdup(filepath);
	
	char* filename_path = dirname(filename_copy);
	
	if(findpath(fs, super, parentino, filename_path) < 0) {
		return FUNC_ERROR;
	}
	
	free(filename_copy);
	return 0;
}

DIR_* opendir_(const char* dirname, uint16_t perms) {
	DIR_* dir = malloc(sizeof(DIR_));
	dir->size = 2;
	dir->idx = 0;
	uint32_t dirino;
	char* tempstr = strdup(dirname);
	if(findpath(fs, super, &dirino, tempstr) < 0) {
		// check perms here
		if(opendir_creat(fs, super, &dirino, perms, dirname) < 0) {
			fprintf(stderr, "opendir_: opendir_creat\n");
			return NULL;
		}
		free(tempstr);
		dir->fd = io_open_fd(dirino);
		if(getFiles(fs, super, dirino, &(dir->files), &(dir->size)) < 0) {
			fprintf(stderr, "opendir_: cannot read files\n");
			return NULL;
		}
		return dir;
	}
	/* verify type */
	struct fs_inode ind;
	fs_read_inode(fs, super, dirino, &ind);
	uint16_t mode = ind.mode;
	if((mode & S_DIR) == 0) {
		fprintf(stderr, "opendir_: %s is a regular file (use open_)\n", dirname);
		return NULL;
	}

	dir->fd = io_open_fd(dirino);
	if(getFiles(fs, super, dirino, &(dir->files), &(dir->size)) < 0) {
		fprintf(stderr, "opendir_: cannot read files\n");
		return NULL;
	}

	free(tempstr);
	// check perms here
	return dir;
}

struct dirent* readdir_(DIR_* dir) {
	free(dir->files);
	uint32_t ino = io_getino(dir->fd);
	if(getFiles(fs, super, ino, &(dir->files), &(dir->size)) < 0) {
		fprintf(stderr, "readdir_: cannot update files\n");
		return NULL;
	}
	if(dir->idx >= dir->size) {
		return NULL;
	}
	return dir->files + (dir->idx++);
}

int closedir_(DIR_* dir) {
	if(io_close_fd(dir->fd) < 0) {
		fprintf(stderr, "close_: can't close %d\n", dir->fd);
		return FUNC_ERROR;
	}
	free(dir->files);
	free(dir);
	return 0;
}
int ls_(DIR_* dir) {
	struct dirent* d;
	printf("len=%d\n", dir->size);
	while((d = readdir_(dir)) != NULL){
		printf("%d %d %s\n", d->d_ino, d->d_type, d->d_name);
	}
	return 0;
}

int open_(const char* filename, uint16_t perms) {

	uint32_t fileino;
	char* tempstr = strdup(filename);
	if(findpath(fs, super, &fileino, tempstr) < 0) {
		// check perms here
		if(open_creat(fs, super, &fileino, perms, filename) < 0) {
			fprintf(stderr, "open_: open_creat\n");
			return FUNC_ERROR;
		}
		free(tempstr);
		return io_open_fd(fileino);
	}
	/* verify type */
	struct fs_inode ind;
	fs_read_inode(fs, super, fileino, &ind);
	uint16_t mode = ind.mode;
	if((mode & S_DIR) != 0) {
		fprintf(stderr, "open_: %s is a directory (use opendir_)\n", filename);
		return FUNC_ERROR;
	}

	free(tempstr);
	// check perms here
	return io_open_fd(fileino);
}

int close_(int fd) {
	if(io_close_fd(fd) < 0) {
		fprintf(stderr, "close_: can't close %d\n", fd);
		return FUNC_ERROR;
	}
	return 0;
}

int lseek_(int fd, uint32_t newoff) {
	if(io_lseek(fs, super, fd, newoff) < 0) {
		fprintf(stderr, "lseek_: io_lseek\n");
		return FUNC_ERROR;
	}
	return 0;
}

int write_(int fd, void* data, int size) {
	if(io_write(fs, super, fd, data, size) < 0) {
		fprintf(stderr, "write_: io_write\n");
		return FUNC_ERROR;
	}
	return 0;
}

int read_(int fd, void* data, int size) {
	if(io_read(fs, super, fd, data, size) < 0) {
		fprintf(stderr, "read_: io_read\n");
		return FUNC_ERROR;
	}
	return 0;
}


int rm_(const char* filename) {
	uint32_t fileino;
	char* tempstr = strdup(filename);
	if(findpath(fs, super, &fileino, tempstr) < 0) {
		fprintf(stderr, "rm_: findpath\n");
		return FUNC_ERROR;
	}
	struct fs_inode ind;
	fs_read_inode(fs, super, fileino, &ind);
	uint16_t mode = ind.mode;
	if((mode & S_DIR) != 0) {
		fprintf(stderr, "rm_: %s is a directory (use opendir_)\n", filename);
		return FUNC_ERROR;
	}

	uint32_t ino;
	if(getParentInode(filename, &ino) < 0) {
		fprintf(stderr, "rm_: invalid file path %s\n", filename);
		return FUNC_ERROR;
	}
	free(tempstr);
	tempstr = strdup(filename);
	char* base = basename(tempstr);

	if(delFile(fs, super, ino, base) < 0) {
		fprintf(stderr, "rm_: can't remove file\n");
		return FUNC_ERROR;
	}
		
	free(tempstr);
	return 0;
}

int cp(const char* src, const char* dest) {
	return 0;
}

//~ int mv(const char* src, const char* dest)
//~ {
	//~ uint32_t src_ino;
	//~ findpath(fs, super, &src_ino, src);

	//~ char* src_copy1 = strdup(src);
	//~ char* src_copy2 = strdup(src);
	//~ char* src_base = dirname(src_copy1);
	//~ char* src_path = dirname(src_copy2);
	//~ uint32_t src_parent;
	//~ findpath(fs, super, &src_parent, src_path);

	//~ char* dest_copy1 = strdup(dest);
	//~ char* dest_copy2 = strdup(dest);
	//~ char* dest_base = basename(dest_copy1);
	//~ char* dest_path = dirname(dest_copy2);
	//~ uint32_t dest_parent;
	//~ findpath(fs, super, &dest_parent, dest_path); // get the parent's fd
	
	//~ delFile(fs, super, src_ino, src_copy);
	
	//~ return 0;
//~ }

void closefs() {
	printf("Closing the filesystem..\n");
	disk_close(&fs);
}
