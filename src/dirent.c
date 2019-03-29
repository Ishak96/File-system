/**
 * @file dirent.c
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief main directory managment and naming functions
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

/**
 * @brief format and empty directory
 * @details allocate the inode for the directory and initialize
 * the size (to 0) in the first byte
 */
int formatdir(struct fs_filesyst fs, struct fs_super_block super, uint32_t* inodenum, uint16_t mode) {
	mode |= S_DIR;
	if(io_open_creat(fs, super, mode, inodenum) < 0) {
		fprintf(stderr, "formatdir: io_open_creat\n");
		return FUNC_ERROR;
	}
	int size = 0;
	if(io_write_ino(fs, super, *inodenum, &size, 0, sizeof(int)) < 0) {
		fprintf(stderr, "opendir: io_write\n");
		return FUNC_ERROR;
	}

	return 0;
}

/**
 * @brief get the files in a directory with inode *inodenum* 
 * @details allocates an array of struct dirent's and puts the files
 * and the number of files in files and size respectively
 */
int getFiles(struct fs_filesyst fs, struct fs_super_block super, 
		     uint32_t dirino, struct dirent** files, int* size)
{
	if(size == NULL) {
		fprintf(stderr, "getFile: invalid arguments\n");
		return FUNC_ERROR;
	}
	*size = 0;

	if(io_read_ino(fs, super, dirino, size, 0, sizeof(int)) < 0) {
		fprintf(stderr, "getFiles: io_read\n");
		return FUNC_ERROR;
	}

	*files = malloc(sizeof(struct dirent) * (*size));
	bzero(*files, sizeof(struct dirent) * (*size));
	if(files == NULL) {
		fprintf(stderr, "getFiles: malloc err!\n");
		return FUNC_ERROR;
	}
	if(*size > 0 && io_read_ino(fs, super, dirino, *files, sizeof(int),
								sizeof(struct dirent) * (*size)) < 0)
	{
		fprintf(stderr, "getFile: io_read\n");
		free(*files);
		return FUNC_ERROR;
	}
	return 0;
}


/**
 * @brief find a filename in a directory
 * @details gets the structure found in a directory of the corresponding
 * file with name *filename* in directory with inode number *dirino*.
 * this function uses a binary search because the file entries are sorted
 * in the directory
 */
int findFile(struct fs_filesyst fs, struct fs_super_block super,
			   uint32_t dirino, char* filename, struct dirent *res, int* idx)
{
	if(strlen(filename) >= 256) {
		fprintf(stderr, "findFile: invalid arguments\n");
		return FUNC_ERROR;
	}

	struct dirent* files = NULL;
	int size = 0;
	if(getFiles(fs, super, dirino, &files, &size) < 0) {
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

/**
 * @brief insert a file into a directory
 * @details inserts the file structure *file* into the corresponding 
 * directory with inode number *dirino*. the insertion is in a sorted list.
 */
int insertFile(struct fs_filesyst fs, struct fs_super_block super,
			   uint32_t dirino, struct dirent file)
{
	int idx = 0;
	struct dirent res;

	if(findFile(fs, super, dirino, file.d_name, &res, &idx) < 0) {
		fprintf(stderr, "insertFile: findFile\n");
		return FUNC_ERROR;
	}
	if(idx >= 0) {
		fprintf(stderr, "insertFile: file exists already %s\n", res.d_name);
		return FUNC_ERROR;
	}

	struct dirent* files = NULL;
	int size = 0;
	if(getFiles(fs, super, dirino, &files, &size) < 0) {
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
	//io_lseek(fs, super, dirfd, sizeof(int));
	if(size > 0 && io_write_ino(fs, super, dirino, files, sizeof(int),
									sizeof(struct dirent) * size) < 0)
	{
		fprintf(stderr, "insertFile: io_write\n");
		free(files);
		return FUNC_ERROR;
	}

	if(io_write_ino(fs, super, dirino, &file, sizeof(struct dirent) * size + sizeof(int),
					sizeof(struct dirent)) < 0)
	{
		fprintf(stderr, "insertFile: io_write\n");
		free(files);
		return FUNC_ERROR;
	}
	//io_lseek(fs, super, dirfd, 0);
	size ++;
	
	if(io_write_ino(fs, super, dirino, &size, 0, sizeof(int)) < 0) {
		fprintf(stderr, "insertFile: io_write\n");
		free(files);
		return FUNC_ERROR;
	}

	free(files);
	return 0;
}

/**
 * @brief delete a file from a directory
 * @details deletes the file with filename *filename* into the directory
 * with inode number *dirino*. the deletion is also done as in a sorted
 * list.
 */
int delFile(struct fs_filesyst fs, struct fs_super_block super,
			   uint32_t dirino, char* filename)
{
	int idx = 0;
	struct dirent res;

	if(findFile(fs, super, dirino, filename, &res, &idx) < 0) {
		fprintf(stderr, "delFile: findFile\n");
		return FUNC_ERROR;
	}
	if(idx < 0) {
		return FUNC_ERROR;
	}
	struct dirent* files = NULL;
	int size = 0;
	if(getFiles(fs, super, dirino, &files, &size) < 0) {
		fprintf(stderr, "findFile: invalid arguments\n");
		return FUNC_ERROR;
	}
	if(size > 0) {
		while(idx+1 < size) {
			files[idx] = files[idx+1];
			idx ++;
		}
		if(size > 0 && io_write_ino(fs, super, dirino,
		 files, sizeof(int), sizeof(struct dirent) * (size-1)) < 0) {
			fprintf(stderr, "insertFile: io_write\n");
			free(files);
			return FUNC_ERROR;
		}
		size --;
		
		if(io_write_ino(fs, super, dirino, &size, 0, sizeof(int)) < 0) {
			fprintf(stderr, "insertFile: io_write\n");
			free(files);
			return FUNC_ERROR;
		}
	}

	free(files);
	
	struct fs_inode ind;
	if(fs_read_inode(fs, super, res.d_ino, &ind) < 0) {
		fprintf(stderr, "open_ino: fs_read_inode with inodenum=%u\n", dirino);
		return FUNC_ERROR;
	}
	ind.hcount --;
	if(ind.hcount <= 0) {
		if(io_rm_ino(fs, super, res.d_ino) < 0) {
			fprintf(stderr, "rm_: couldn't delete inode no %d\n", dirino);
			return FUNC_ERROR;
		}
	} else {
		if(fs_write_inode(fs, super, res.d_ino, &ind) < 0) {
			fprintf(stderr, "open_ino: fs_write_inode\n");
			return FUNC_ERROR;
		}
	}
	return 0;
}

/**
 * @brief find the inode number of a file from its absolute path
 * @details searches for the inode number of file with the absolute path
 * *filename* and puts the value found into pointer *ino*. note that 
 * the root directory "/" is a special case and always has inode number 0
 */
int findpath(struct fs_filesyst fs, struct fs_super_block super, uint32_t* ino, char* filename) {
	if(!strcmp(filename, "/")) {
		*ino = 0;
		return 0;
	}
	char delim[2] = "/";
	char* tok = strtok(filename, delim);

	uint32_t dir = 0; // the root directory always has the inodenum 0
	struct dirent filefound = {0};
	int idx;

	while(tok != NULL) {
		if(findFile(fs, super, dir, tok, &filefound, &idx) < 0) {
			fprintf(stderr, "findpath: findFile\n");
			return FUNC_ERROR;
		}
		if(filefound.d_ino == -1) {
			return FUNC_ERROR;
		}
		dir = filefound.d_ino;
		tok = strtok(NULL, delim);
	}
	*ino = dir;
	return 0;
}

/**
 * @brief structures the a directory using the given filepath
 * @details formats the main components of the directory. Meaning it creates
 * the subdirectories . and .. and also create an instance of its directory
 * entry in the parent directory. ex: with /DIR it puts . and .. in the 
 * /DIR directory and DIR in the / (root) directory
 * @param dirino   the inode number of the directory to be inserted
 * @param filepath the full path (absolute) of the directory to be inserted
 */
int opendir_ino(struct fs_filesyst fs, struct fs_super_block super, uint32_t dirino,
			const char* filepath)
{	
	struct fs_inode ind;
	if(fs_read_inode(fs, super, dirino, &ind) < 0) {
		fprintf(stderr, "opendir_ino: fs_read_inode with inodenum=%u\n", dirino);
		return FUNC_ERROR;
	}

	uint16_t mode = ind.mode;
	if((mode & S_DIR) == 0) {
		fprintf(stderr, "opendir_ino: %s is a regular file\n", filepath);
		return FUNC_ERROR;
	}
	/* todo: verify type and get mode*/
	struct dirent cur = {0};
	
	// put the . in the created directory
	cur.d_ino = dirino;
	cur.d_type = S_DIR;	
	strcpy(cur.d_name, ".");
	if(ind.hcount == 0 && insertFile(fs, super, dirino, cur) < 0) {
		fprintf(stderr, "opendir_ino: insertFile\n");
		return FUNC_ERROR;
	}

	// get parent path and file basename
	char* path_copy1 = strdup(filepath); // note: we need two copies because the
	char* path_copy2 = strdup(filepath); // basename and dirname funcs change the values of the strings
	char* child_name = basename(path_copy1);
	char* parent_path = dirname(path_copy2);

	uint32_t parent;
	findpath(fs, super, &parent, parent_path); // get the parent's fd
	
	// put .. in the created dir as the parent
	cur.d_ino = parent;
	strcpy(cur.d_name, "..");
	if(ind.hcount == 0 && insertFile(fs, super, dirino, cur) < 0) {
		fprintf(stderr, "opendir_ino: insertFile\n");
		return FUNC_ERROR;
	}
	
	// put the current dir in the parent dir
	if(strcmp("/", filepath)) {  // the root dir is a special case
		cur.d_ino = dirino;
		cur.d_type = mode;
		strcpy(cur.d_name, child_name);
		if(insertFile(fs, super, parent, cur) < 0) {
			fprintf(stderr, "opendir_ino: insertFile\n");
			return FUNC_ERROR;
		}
	}

	free(path_copy1);
	free(path_copy2);
	
	ind.hcount ++;
	if(fs_write_inode(fs, super, dirino, &ind) < 0) {
		fprintf(stderr, "opendir_ino: fs_write_inode\n");
		return FUNC_ERROR;
	}
	
	return 0;
}

/**
 * @brief creates and formats a directory
 * @details creates a new directory with a new inode number and then
 * calls *opendir_ino*
 */
int opendir_creat(struct fs_filesyst fs, struct fs_super_block super, uint32_t* dirino,
			uint16_t perms, const char* filepath)
{
	/* increment the inode hardlink count here */
	perms |= S_DIR;
	if(formatdir(fs, super, dirino, perms) < 0) {
		fprintf(stderr, "creatdir: formatdir\n");
		return FUNC_ERROR;
	}
	
	if(opendir_ino(fs, super, *dirino, filepath) < 0) {
		fprintf(stderr, "opendir_creat: opendir_ino\n");
		return FUNC_ERROR;
	}
	return 0;
}

/**
 * @brief inserts a file into a directory
 * @details insert the file with inode number *fileino* with the path 
 * *filepath*, meaning it creates a new directory entry and puts it in
 * the parent directory (we get this from the full path ex: /DIR/file 
 * the parent directory is /DIR)
 * @param fileino  the inode of the file to be inserted
 * @param filepath the full path of the file to be inserted
 */
int open_ino(struct fs_filesyst fs, struct fs_super_block super, uint32_t fileino,
			 const char* filepath)
{	
	struct fs_inode ind;
	if(fs_read_inode(fs, super, fileino, &ind) < 0) {
		fprintf(stderr, "open_ino: fs_read_inode with inodenum=%u\n", fileino);
		return FUNC_ERROR;
	}
	uint16_t mode = ind.mode;
	if(mode & S_DIR) {
		// fprintf(stderr, "open_ino: %ud is a directory\n", fileino);
		return FUNC_ERROR;
	}
	/* todo: verify type and mode*/
	struct dirent cur = {0};

	// get parent path and file basename
	char* path_copy1 = strdup(filepath); // note: we need two copies because the
	char* path_copy2 = strdup(filepath); // basename and dirname funcs change the values of the strings
	char* child_name = basename(path_copy1);
	char* parent_path = dirname(path_copy2);

	uint32_t parent;
	findpath(fs, super, &parent, parent_path); // get the parent's fd
	
	cur.d_type = mode;
	cur.d_ino = fileino;
	strcpy(cur.d_name, child_name);
	
	if(insertFile(fs, super, parent, cur) < 0) {
		fprintf(stderr, "open_ino: insertFile\n");
		return FUNC_ERROR;
	}
	ind.hcount ++;
	if(fs_write_inode(fs, super, fileino, &ind) < 0) {
		fprintf(stderr, "open_ino: fs_write_inode\n");
		return FUNC_ERROR;
	}
	
	return 0;
}

/**
 * @brief creates a new file
 * @details creates a new file with a new inode number and inserts its
 * corresponding directory entry into the appropriate directory (meaning
 * inserts to the parent directory)
 */
int open_creat(struct fs_filesyst fs, struct fs_super_block super, uint32_t* fileino,
			uint16_t mode, const char* filepath)
{
	mode &= (~S_DIR);
	
	if(io_open_creat(fs, super, mode, fileino) < 0) {
		fprintf(stderr, "open_creat: io_open_creat\n");
		return FUNC_ERROR;
	}

	if(open_ino(fs, super, *fileino, filepath) < 0) {
		fprintf(stderr, "open_creat: opendir_ino\n");
		return FUNC_ERROR;
	}
	return 0;
}
