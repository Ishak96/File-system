#ifndef UI_H
#define UI_H

int initfs(char* filename, size_t size);
int ls_(const char* dirpath);
int lseek_(int fd, uint32_t newoff);
int write_(int fd, void* data, int size);
int read_(int fd, void* data, int size);
int open_(const char* filename, uint16_t perms);
int close_(int fd);
void closefs();

#endif
