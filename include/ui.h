#ifndef UI_H
#define UI_H
#include <dirent.h>

int initfs(char* filename, size_t size);
int ls_(const char* dir);
int ln_(const char* src, const char* dest);
int lseek_(int fd, uint32_t newoff);
int write_(int fd, void* data, int size);
int read_(int fd, void* data, int size);
int open_(const char* filename, int creat, uint16_t perms);
DIR_* opendir_(const char* dirname, int creat, uint16_t perms);
struct dirent readdir_(DIR_* dir);
int rm_(const char* filename);
int rmdir_(const char* filename, int recursive);
int close_(int fd);
int closedir_(DIR_* dir);
int cp_(const char* src, const char* dest);
int mv_(const char* src, const char* dest);
void closefs();

#endif
