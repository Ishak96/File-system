/**
 * @file ui.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak 
 * @brief The main functions to be used by the fs users
 * @details contains utility functions such as formatting the fs and
 * opening and writing to files and so on.
 */
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

/**
 * @brief initializes the system
 * @details fills the global variables fs and super
 * @param filename    the filename of the fs
 * @param size        the size of the fs
 * @param format      a boolean of wether to format the virtual partition
 * @return 0 in case of success or -1 in case of an error
 */
int initfs(char* filename, size_t size, int format) {
	srand(time(NULL));
	printf("Opening filesyst..\n");
	/* test creatfile */
	if(creatfile(filename, size, &fs) < 0) {
		fprintf(stderr, "initfs: can't create file %s\n", filename);
	}
	
	if(format) {
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
	} else {
		union fs_block blk;
		if(fs_read_block(fs, 0, &blk) < 0) {
			fprintf(stderr, "fs_format: fs_read_block\n");
			return FUNC_ERROR;
		}
		super = blk.super;
	}

	if(!fs_check_magicnum(fs.fd)) {
		fprintf(stderr, "Magic number of file doesn't match the FS's\n");
		return FUNC_ERROR;
	}

	return 0;
}

/**
 * @brief a utility function to get the inode of the parent
 * @param parentino the pointer to put the inode in
 * @return 0 in case of success or -1 in case of an error
 */
int getParentInode(const char* filepath, uint32_t* parentino) {
	char* filename_copy = strdup(filepath);
	
	char* filename_path = dirname(filename_copy);
	
	if(findpath(fs, super, parentino, filename_path) < 0) {
		return FUNC_ERROR;
	}
	
	free(filename_copy);
	return 0;
}

/**
 * @brief opens a directory
 * @details opens the directory with pathname *dirname*, or creates it 
 * if the *creat* parameter is set to not null, the *perms* are set to 
 * the created directory in that case.
 * @return the opened directory pointer, or NULL in case of an error
 */
DIR_* opendir_(const char* dirname, int creat, uint16_t perms) {
	DIR_* dir = malloc(sizeof(DIR_));
	dir->size = 2;
	dir->idx = 0;
	uint32_t dirino;
	char* tempstr = strdup(dirname);
	if(findpath(fs, super, &dirino, tempstr) < 0) {
		// check perms here
		if(!creat) {
			fprintf(stderr, "opendir_: directory does not exist.\n");
			return NULL;
		}
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
	if(fs_read_inode(fs, super, dirino, &ind) < 0) {
		fprintf(stderr, "opendir_: cannot open the inode\n");
		return NULL;
	}
	uint16_t mode = ind.mode;
	if((mode & S_DIR) == 0) {
		fprintf(stderr, "opendir_: %s is a regular file (use open_)\n", dirname);
		return NULL;
	}

	dir->fd = io_open_fd(dirino);
	if(dir->fd < 0) {
		fprintf(stderr, "opendir_: canot create a file descriptor\n");
		return NULL;
	}
	if(getFiles(fs, super, dirino, &(dir->files), &(dir->size)) < 0) {
		fprintf(stderr, "opendir_: cannot read files\n");
		return NULL;
	}

	free(tempstr);
	// check perms here
	return dir;
}

/**
 * @brief read an entry from a DIR_* pointer
 * @details reads an entry from a directory (after opening it with opendir_
 * which returns the DIR_* pointer)
 * @return a pointer to the current directory entry, or NULL in case the 
 * end is reached 
 */
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

/**
 * @brief get the inode structure from the path
 */
struct fs_inode getInode(const char* path){
	struct fs_inode ind = {0};
	uint32_t fileino;
	char* tmp = strdup(path);
	if(findpath(fs, super, &fileino, tmp) < 0) {
		fprintf(stderr, "cannot get inode of \"%s\"\n", path);
		return ind;
	}
	free(tmp);
	if(fs_read_inode(fs, super, fileino, &ind) < 0) {
		fprintf(stderr, "rmdir_: fs_read_inode\n");
		return ind;
	}
	return ind;
}

/**
 * @brief close a directory pointer
 * @details called after opendir_ to free the DIR_* pointer and 
 * its components
 * @return 0 in case of success or -1 in case of an error
 */
int closedir_(DIR_* dir) {
	if(dir == NULL) {
		fprintf(stderr, "closedir_: null dir\n");
		return FUNC_ERROR;
	}
	if(io_close_fd(dir->fd) < 0) {
		fprintf(stderr, "close_: can't close %d\n", dir->fd);
		return FUNC_ERROR;
	}
	free(dir->files);
	free(dir);
	return 0;
}

/**
 * @brief list the files in a directory in a long format
 * @details lists all the files in a directory in a long format
 * @param direct the *absolute* path from the root to the directory
 * @return 0 in case of success or -1 in case of an error
 */
int lsl_(const char* direct) {
	DIR_* dir = opendir_(direct, 0, 0);
	if(dir == NULL) {
		fprintf(stderr, "ls_: directory does not exist\n");
		return FUNC_ERROR;
	}
	printf("%s\n", direct);
	printf("ino:type:name\n");
	struct dirent* d;
	while((d = readdir_(dir)) != NULL){
		if(d->d_type){
			printf("%d   %s  ", d->d_ino, (d->d_type!=0)?"DIR":"REG");
			printf("\033[32m");
			printf("%s\n", d->d_name);
			printf("\033[37m");

		}
		else{
			printf("%d   %s  %s\n", d->d_ino, (d->d_type!=0)?"DIR":"REG", d->d_name);
		}
	}
	
	closedir_(dir);
	return 0;
}

/**
 * @brief list the files in a directory
 * @details lists all the files in a directory
 * @param direct the *absolute* path from the root to the directory
 * @return 0 in case of success or -1 in case of an error
 */
int ls_(const char* direct) {
	DIR_* dir = opendir_(direct, 0, 0);
	if(dir == NULL) {
		fprintf(stderr, "ls_: directory does not exist\n");
		return FUNC_ERROR;
	}
	struct dirent* d;
	while((d = readdir_(dir)) != NULL){
		if(d->d_type){
			printf("\033[32m");
			printf("%s\t",d->d_name);
			printf("\033[37m");
		}
		else{
			printf("%s\t",d->d_name);
		}
	}
	printf("\n");
	closedir_(dir);
	return 0;
}

/**
 * @brief opens a file
 * @details opens the file with pathname *filename*, or creates it 
 * if the *creat* parameter is set to not null, the *perms* are set to 
 * the created file in that case.
 * @return the opened file's descriptor fd, or -1 in case of an error
 */
int open_(const char* filename, int creat, uint16_t perms) {
	uint32_t fileino;
	char* tempstr = strdup(filename);
	if(findpath(fs, super, &fileino, tempstr) < 0) {
		// check perms here
		if(!creat) {
			fprintf(stderr, "open_: file does not exist.\n");
			return FUNC_ERROR;
		}
		if(open_creat(fs, super, &fileino, perms, filename) < 0) {
			fprintf(stderr, "open_: open_creat\n");
			return FUNC_ERROR;
		}
		struct fs_inode ind;
		fs_read_inode(fs, super, fileino, &ind);
		free(tempstr);
		return io_open_fd(fileino);
	}
	/* verify type */
	struct fs_inode ind;
	if(fs_read_inode(fs, super, fileino, &ind) < 0) {
		fprintf(stderr, "open_: fs_read_inode\n");
		return FUNC_ERROR;
	}
	uint16_t mode = ind.mode;
	if((mode & S_DIR) != 0) {
		fprintf(stderr, "open_: %s is a directory (use opendir_)\n", filename);
		return FUNC_ERROR;
	}

	free(tempstr);
	// check perms here
	return io_open_fd(fileino);
}

/**
 * @brief closes an open file descritor
 * @return 0 in case of success or -1 in case of an error
 */
int close_(int fd) {
	if(io_close_fd(fd) < 0) {
		fprintf(stderr, "close_: can't close %d\n", fd);
		return FUNC_ERROR;
	}
	return 0;
}

/**
 * @brief changes the current pointer for a file
 * @details changes the offset of the file descriptor to newoff
 * @return 0 in case of success or -1 in case of an error
 */
int lseek_(int fd, uint32_t newoff) {
	if(io_lseek(fs, super, fd, newoff) < 0) {
		fprintf(stderr, "lseek_: io_lseek\n");
		return FUNC_ERROR;
	}
	return 0;
}
/**
 * @brief write data to a file
 * @details writes *size* bytes from the *data* pointer into the corresponding file
 * for the *fd*.
 * @return 0 in case of success or -1 in case of an error
 */
int write_(int fd, void* data, int size) {
	if(io_write(fs, super, fd, data, size) < 0) {
		fprintf(stderr, "write_: io_write\n");
		return FUNC_ERROR;
	}
	return 0;
}
/**
 * @brief reads data from file
 * @details reads *size* bytes from the corresponding file for the 
 * *fd* and puts the result in the *data* pointer
 * @return 0 in case of success or -1 in case of an error
 */
int read_(int fd, void* data, int size) {
	if(io_read(fs, super, fd, data, size) < 0) {
		fprintf(stderr, "read_: io_read\n");
		return FUNC_ERROR;
	}
	return 0;
}

/**
 * @brief removes a file
 * @details removes files from their path, note that the inode may not
 * get deleted until all hard links to the inode number have been deleted
 * @return 0 in case of success or -1 in case of an error
 */
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
		fprintf(stderr, "rm_: %s is a directory (use rmdir_)\n", filename);
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

/**
 * @brief attempts to remove a directory
 * @details attempts to remove the directory from its path, if it doesn't contain
 * it gets deleted, if it does, it gets deleted if the recursive boolean is set
 * to no null else it doesn't.
 * Note that the inode may not
 * get deleted until all hard links to the inode number have been deleted
 * @return 0 in case of success or -1 in case of an error
 */
int rmdir_(const char* filename, int recursive) {
	uint32_t fileino;
	char* tempstr = strdup(filename);
	if(findpath(fs, super, &fileino, tempstr) < 0) {
		fprintf(stderr, "rmdir_: directory doesn't exist\n");
		return FUNC_ERROR;
	}
	struct fs_inode ind;
	if(fs_read_inode(fs, super, fileino, &ind) < 0) {
		fprintf(stderr, "rmdir_: fs_read_inode\n");
		return FUNC_ERROR;
	}
	uint16_t mode = ind.mode;
	if((mode & S_DIR) == 0) {
		fprintf(stderr, "rmdir_: %s is a regular file (use rm_)\n", filename);
		return FUNC_ERROR;
	}
	
	DIR_* dir = opendir_(filename, 0, 0);
	if(dir == NULL) {
		fprintf(stderr, "rmdir_: opendir_\n");
		return FUNC_ERROR;
	}
	if(dir->size > 2) {
		if(!recursive) {
			fprintf(stderr, "rmdir_: %s is not empty\n", filename);
			free(tempstr);
		} else {
			struct dirent* dire = NULL;
			while((dire = readdir_(dir)) != NULL) {
				if(!strcmp(dire->d_name, ".") || !strcmp(dire->d_name, "..")) {
					continue;
				}
				char* sub = malloc(sizeof(char) * (strlen(filename) + strlen(dire->d_name) + 1));
				strcpy(sub, filename);
				if(filename[strlen(filename) - 1] != '/') {
					strcat(sub, "/");
				}
				strcat(sub, dire->d_name);
				if(dire->d_type & S_DIR) {
					if(rmdir_(sub, 1) < 0) {
						fprintf(stderr, "rmdir_: can't remove %s\n", sub);
						free(sub);
						return FUNC_ERROR;
					}
				} else {
					if(rm_(sub) < 0) {
						fprintf(stderr, "rmdir_: can't remove %s\n", sub);
						free(sub);
						return FUNC_ERROR;
					}
				}
				free(sub);
			}
		}
	}
	uint32_t ino;
	if(getParentInode(filename, &ino) < 0) {
		fprintf(stderr, "rmdir_: invalid directory path %s\n", filename);
		return FUNC_ERROR;
	}
	free(tempstr);
	tempstr = strdup(filename);
	char* base = basename(tempstr);

	if(delFile(fs, super, ino, base) < 0) {
		fprintf(stderr, "rm_: can't remove file or directory\n");
		return FUNC_ERROR;
	}
	free(tempstr);
	closedir_(dir);
	return 0;
}

/**
 * @brief copies a file from src to dest
 * @details copies any file or directory from src to dest
 * Note that you have to specify the file name of the destination
 * e.g. cp_("/dir/file", "/") won't work because the destination doesn't 
 * have a specified name like cp_("/dir/file", "/file")
 * @return 0 in case of success or -1 in case of an error
 */
int cp_(const char* src, const char* dest) {
	int srcfd = open_(src, 0, 0);
	int destfd = open_(dest, 1, 0);
	if(srcfd < 0 || destfd < 0) {
		fprintf(stderr, "cp_: cannot open src or dest\n");
		return FUNC_ERROR;
	}
	
	struct fs_inode ind = {0};
	if(fs_read_inode(fs, super, io_getino(srcfd), &ind)) {
		fprintf(stderr, "cp_: cannot read inode\n");
		return FUNC_ERROR;
	}

	size_t size = ind.size;
	
	lseek_(srcfd, 0);
	void* data = malloc(size);
	if(io_read(fs, super, srcfd, data, size) < 0) {
		fprintf(stderr, "cp_: cannot read from the source\n");
		return FUNC_ERROR;
	}
	
	lseek_(destfd, 0);
	if(io_write(fs, super, destfd, data, size) < 0) {
		fprintf(stderr, "cp_: cannot write to the destination\n");
		return FUNC_ERROR;
	}
	
	close_(srcfd);
	close_(destfd);
	return 0;
}

/**
 * @brief creates a hard link of src in dest
 * @details creates a hard link of the correspoding inode of
 * any file or directory from src in dest.
 * Note that you have to specify the file name of the destination
 * e.g. ln_("/dir/file", "/") won't work because the destination doesn't 
 * have a specified name like ln_("/dir/file", "/file")
 * @return 0 in case of success or -1 in case of an error
 */
int ln_(const char* src, const char* dest) {
	uint32_t ino;
	char* tmpstr = strdup(src);
	if(findpath(fs, super, &ino, tmpstr) < 0) {
		fprintf(stderr, "ln_: %s doesn't exist\n", src);
		free(tmpstr);
		return FUNC_ERROR;
	}
	struct fs_inode ind;
	if(fs_read_inode(fs, super, ino, &ind) < 0) {
		fprintf(stderr, "open_ino: fs_read_inode with inodenum=%u\n", ino);
		free(tmpstr);
		return FUNC_ERROR;
	}
	uint16_t mode = ind.mode;
	if((mode & S_DIR) == 0) {
		if(open_ino(fs, super, ino, dest) < 0) {
			fprintf(stderr, "ln_: cannot open %s\n", dest);
			free(tmpstr);
			return FUNC_ERROR;
		}
	} else {
		if(opendir_ino(fs, super, ino, dest) < 0) {
			fprintf(stderr, "ln_: cannot open %s\n", dest);
			free(tmpstr);
			return FUNC_ERROR;
		}
	}
	if(fs_read_inode(fs, super, ino, &ind) < 0) {
		fprintf(stderr, "open_ino: fs_read_inode with inodenum=%u\n", ino);
		free(tmpstr);
		return FUNC_ERROR;
	}
	free(tmpstr);
	return 0;
}

/**
 * @brief move a file from src to dest
 * @details moves any file or directory from src to dest
 * Note that you have to specify the file name of the destination
 * e.g. mv_("/dir/file", "/") won't work because the destination doesn't 
 * have a specified name like mv_("/dir/file", "/file")
 * @return 0 in case of success or -1 in case of an error
 */
int mv_(const char* src, const char* dest) {
	if(ln_(src, dest) < 0) {
		fprintf(stderr, "mv_: cannot place the destination link\n");
		return FUNC_ERROR;
	}
	uint32_t srcino;
	char* tempstr = strdup(src);
	if(findpath(fs, super, &srcino, tempstr) < 0) {
		fprintf(stderr, "mv_: cannot find inode number\n");
		return FUNC_ERROR;
	}
	free(tempstr);
	struct fs_inode ind = {0};
	if(fs_read_inode(fs, super, srcino, &ind)) {
		fprintf(stderr, "mv_: cannot read inode\n");
		return FUNC_ERROR;
	}
	if(ind.mode & S_DIR) {
		DIR_* dir = opendir_(src, 0, 0);
		if(dir == NULL) {
			fprintf(stderr, "mv_: can't remove file or directory\n");
			return FUNC_ERROR;
		}
		uint32_t ino;
		if(getParentInode(src, &ino) < 0) {
			fprintf(stderr, "mv_: invalid directory path %s\n", src);
			return FUNC_ERROR;
		}
		char* tempstr = strdup(src);
		char* base = basename(tempstr);

		if(delFile(fs, super, ino, base) < 0) {
			fprintf(stderr, "mv_: can't remove file or directory\n");
			return FUNC_ERROR;
		}
		free(tempstr);
		closedir_(dir);
	} else {
		if(rm_(src) < 0) {
			fprintf(stderr, "mv_: cannot delete the source link\n");
			return FUNC_ERROR;
		}
	}
	return 0;
}

/**
 * @brief closes the virtual filesystem
 */
void closefs() {
	printf("Closing the filesystem..\n");
	disk_close(&fs);
}
