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
int io_open_fd(uint32_t inodenum);
int io_close_fd(int fd);
int io_iopen(struct fs_filesyst fs, struct fs_super_block super, uint32_t inodenum);
int io_open_creat(struct fs_filesyst fs, struct fs_super_block super, uint16_t mode,
					uint32_t* inodenum);
int io_write_ino(struct fs_filesyst fs, struct fs_super_block super, uint32_t inodenum,
			 void* data, uint32_t off, size_t size);
int io_write(struct fs_filesyst fs, struct fs_super_block super, int fd,
			 void* data, size_t size);
int io_read_ino(struct fs_filesyst fs, struct fs_super_block super, uint32_t inodenum,
			 void* data, uint32_t off, size_t size);
int io_read(struct fs_filesyst fs, struct fs_super_block super, int fd,
			 void* data, size_t size);
int io_lseek(struct fs_filesyst fs, struct fs_super_block super, int fd,
			  size_t new_off);
int io_rm(struct fs_filesyst fs, struct fs_super_block super, int fd);
uint32_t io_getino(int fd);
size_t io_getoff(int fd);
#endif
