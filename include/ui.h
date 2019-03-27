#ifndef UI_H
#define UI_H
#include <dirent.h>

int initfs(char* filename, size_t size);
int ls_(DIR_* dir);
int lseek_(int fd, uint32_t newoff);
int write_(int fd, void* data, int size);
int read_(int fd, void* data, int size);
int open_(const char* filename, uint16_t perms);
DIR_* opendir_(const char* dirname, uint16_t perms);
struct dirent readdir_(DIR_* dir);
int rm_(const char* filename);
int close_(int fd);
int closedir_(DIR_* dir);
void closefs();

#endif