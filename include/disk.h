#ifndef DISK_H
#define DISK_H
#include <stdint.h>
/**
 * @brief virtual filesystem structure
 * @details contains information about the file used to simulate a disk
 * partition
 */
struct fs_filesyst {
	uint32_t fd; /* file descriptor */
	uint32_t tot_size; /* total size of our file (partition) */
};

/* prototypes */
int creatfile(const char* filename, size_t size);

#endif
