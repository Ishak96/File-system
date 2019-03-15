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

int opendir(struct fs_filesyst fs, struct fs_super_block super);
int insertFile(struct fs_filesyst fs, struct fs_super_block super,
			   int dirfd, struct dirent file);
int findFile(struct fs_filesyst fs, struct fs_super_block super,
			   int dirfd, char* filename, struct dirent *res, int* idx);
int delFile(struct fs_filesyst fs, struct fs_super_block super,
			   int dirfd, char* filename);
#endif
