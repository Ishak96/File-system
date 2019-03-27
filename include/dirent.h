/**
 * @file dirent.h
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief dirent.h - format of directory entries
 * @details main filesystem structs and prototypes
 */
#ifndef DIRENT_H
#define DIRENT_H

#define S_DIR 01000

struct dirent {
    uint32_t d_ino;
    int d_type;                            /* File type  */
    char d_name[256];                      /* File name  */
};

typedef struct {
	int fd;
	size_t size;
	struct dirent* files;
} DIR_;

int formatdir(struct fs_filesyst fs, struct fs_super_block super, uint32_t* inodenum, uint16_t mode);
int insertFile(struct fs_filesyst fs, struct fs_super_block super,
			   uint32_t dirino, struct dirent file);
int findFile(struct fs_filesyst fs, struct fs_super_block super,
			   uint32_t dirino, char* filename, struct dirent *res, int* idx);
int getFiles(struct fs_filesyst fs, struct fs_super_block super, 
		     uint32_t dirino, struct dirent** files, int* size);
int delFile(struct fs_filesyst fs, struct fs_super_block super,
			   uint32_t dirino, char* filename);
int findpath(struct fs_filesyst fs, struct fs_super_block super, uint32_t* ino, char* filename);
int opendir_creat(struct fs_filesyst fs, struct fs_super_block super, uint32_t* dirino,
			uint16_t mode, const char* filepath);
int opendir_ino(struct fs_filesyst fs, struct fs_super_block super, uint32_t dirino, const char* filepath);
int open_ino(struct fs_filesyst fs, struct fs_super_block super, uint32_t fileino,
			 const char* filepath);
int open_creat(struct fs_filesyst fs, struct fs_super_block super, uint32_t* fileino,
			uint16_t mode, const char* filepath);
#endif

