#ifndef IO_H
#define IO_H

#include <disk.h>
#include <fs.h>

#include <stdint.h>
#define IO_MAX_FILEDESC 100

struct io_filedesc {
	int is_allocated;
	uint32_t offset;
	uint32_t mode;
	uint32_t inodenum;
};

struct io_filedesc_table {
	struct io_filedesc fds[IO_MAX_FILEDESC];
};

int io_iopen(struct fs_filesyst fs, struct fs_super_block super, uint32_t inodenum);
int io_open_creat(struct fs_filesyst fs, struct fs_super_block super);
int io_write(struct fs_filesyst fs, struct fs_super_block super, int fd,
			 void* data, size_t size);
int io_read(struct fs_filesyst fs, struct fs_super_block super, int fd,
			 void* data, size_t size);
int io_lseek(struct fs_filesyst fs, struct fs_super_block super, int fd,
			  size_t new_off);
int io_rm(struct fs_filesyst fs, struct fs_super_block super, int fd);
#endif
