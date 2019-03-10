#include <io.h>
#include <fs.h>
#include <devutils.h>
#include <disk.h>

#include <string.h>
#include <stdint.h>
#include <stdio.h>

struct io_filedesc_table filedesc_table = {0};

int io_open_fd(uint32_t inodenum) {
	int found = 0;
	for(int i=0; i<IO_MAX_FILEDESC && found==0; i++) {
		if(filedesc_table.fds[i].is_allocated == 0) {
			found = 1;
			filedesc_table.fds[i].is_allocated = 1;
			filedesc_table.fds[i].offset = 0;
			filedesc_table.fds[i].inodenum = inodenum;
			return i;
		}
	}
	if(!found) {
		fprintf(stderr, "io_alloc_fd: can't allocate a file descriptor!\n");
		return FUNC_ERROR;
	}
	return 0;
}

int io_close_fd(int fd) {
	if(fd < 0) {
		fprintf(stderr, "io_clode_fd: invalid file desciptor!\n");
		return FUNC_ERROR;
	}
	filedesc_table.fds[fd].is_allocated = 0;
	return 0;
}


int io_iopen(struct fs_filesyst fs, struct fs_super_block super, uint32_t inodenum) {
	/* get the inode */
	struct fs_inode ind;
	if(fs_read_inode(fs, super, inodenum, &ind) < 0) {
		fprintf(stderr, "io_open: fs_read_inode\n");
		return FUNC_ERROR;
	}
	/* to add modes and uid, gid, open modes*/
	int fd = io_open_fd(inodenum);

	return fd;
}

int io_open_creat(struct fs_filesyst fs, struct fs_super_block super) {
	uint32_t inodenum;
	if(fs_alloc_inode(fs, &super, &inodenum) < 0) {
		fprintf(stderr, "io_open: can't allocate inode!\n");
		return FUNC_ERROR;
	}
	struct fs_inode ind;
	memset(&ind, 0, sizeof(ind));
	if(fs_write_inode(fs, super, inodenum, &ind) < 0) {
		fprintf(stderr, "io_open_creat: fs_write_inode\n");
		return FUNC_ERROR;
	}
	int fd = io_open_fd(inodenum);
	return fd;
}

int io_lazy_alloc(struct fs_filesyst fs, struct fs_super_block super,
				uint32_t inodenum, struct fs_inode *ind, size_t off, size_t size)
{
	uint32_t direct_off = FS_DIRECT_POINTERS_PER_INODE * FS_BLOCK_SIZE;
	
	uint32_t level0range_s, level0range_e;
	level0range_s = (off < direct_off)? off: direct_off;
	level0range_e = (off + size < direct_off)? off+size-1: direct_off+FS_BLOCK_SIZE-1;

	uint32_t level1range_s, level1range_e;
	level1range_s = (off < direct_off)? 0: off-direct_off;
	level1range_e = (off + size < direct_off)? 0: off+size-direct_off-1;
	
	/* sanity check */
	if(level0range_e < level0range_s || level1range_e < level1range_s) {
		fprintf(stderr, "fs_lazy_alloc: end smaller than start!\n");
		return FUNC_ERROR;
	}

	/* level 0 (eg. direct) */
	uint32_t start = level0range_s / FS_BLOCK_SIZE;
	uint32_t end = level0range_e / FS_BLOCK_SIZE;
	int allocs_needed = 0;
	for(int i=start; i<=end; i++) {
		if(i == FS_DIRECT_POINTERS_PER_INODE) {
			allocs_needed += (ind->indirect == 0);
			continue;
		}
		allocs_needed += (ind->direct[i] == 0);
	}

	if(allocs_needed) {
		uint32_t *dt = malloc(sizeof(uint32_t) * allocs_needed);
		if(dt == NULL) {
			fprintf(stderr, "fs_lazy_alloc: malloc\n");
			return FUNC_ERROR;
		}
		if(fs_alloc_data(fs, &super, dt, allocs_needed) < 0) {
			fprintf(stderr, "fs_lazy_alloc: fs_alloc_data\n");
			return FUNC_ERROR;
		}
		for(int i=start, j=0; i<=end && j<allocs_needed; i++) {
			if(i == FS_DIRECT_POINTERS_PER_INODE) {
				if(ind->indirect == 0) {
					ind->indirect = dt[j++];
					union fs_block temp;
					memset(&temp, 0, sizeof(temp));
					if(fs_write_data(fs, super, &temp, &ind->indirect, 1) < 0) {
						fprintf(stderr, "fs_lazy_alloc: fs_write_block\n");
					}
				}
				break;
			}
			if(ind->direct[i] == 0) {
				ind->direct[i] = dt[j++];
			}
		}
		/* write changes to disk */
		if(fs_write_inode(fs, super, inodenum, ind) < 0) {
			fprintf(stderr, "fs_lazy_alloc: fs_write_inode\n"); 
			return FUNC_ERROR;
		}
		free(dt);
	}

	/* level 1 (eg. indirect) */
	/* read the indirect block */
	if(ind->indirect == 0 || level1range_e == -1) {
		return 0; /* no more allocation needed */
	}
	
	union fs_block indirect_data;
	if(fs_read_data(fs, super, &indirect_data, &ind->indirect, 1) < 0) {
		fprintf(stderr, "fs_lazy_alloc: fs_read_data\n");
		return FUNC_ERROR;
	}
	/* allocs */
	start = level1range_s / FS_BLOCK_SIZE;
	end = level1range_e / FS_BLOCK_SIZE;
	allocs_needed = 0;
	for(int i=start; i<=end; i++) {
		allocs_needed += (indirect_data.pointers[i] == 0);
	}
	if(allocs_needed) {
		uint32_t *dt = malloc(sizeof(uint32_t) * allocs_needed);
		if(dt == NULL) {
			fprintf(stderr, "fs_lazy_alloc: malloc\n");
			return FUNC_ERROR;
		}
		if(fs_alloc_data(fs, &super, dt, allocs_needed) < 0) {
			fprintf(stderr, "fs_lazy_alloc: fs_alloc_data\n");
			return FUNC_ERROR;
		}
		for(int i=start, j=0; i<=end && j<allocs_needed; i++) {
			if(indirect_data.pointers[i] == 0) {
				indirect_data.pointers[i] = dt[j++];
			}
		}
		/* write changes to disk */
		if(fs_write_data(fs, super, &indirect_data, &ind->indirect, 1) < 0) {
			fprintf(stderr, "fs_lazy_alloc: fs_write_inode\n"); 
			return FUNC_ERROR;
		}
		free(dt);
	}
	return 0;
}

int io_lseek(struct fs_filesyst fs, struct fs_super_block super, int fd,
			  size_t new_off)
{
	filedesc_table.fds[fd].offset = new_off;
	return 0;
}

int io_write(struct fs_filesyst fs, struct fs_super_block super, int fd,
			 void* data, size_t size)
{
	if(filedesc_table.fds[fd].is_allocated == 0) {
		fprintf(stderr, "io_write: fd closed!\n");
		return FUNC_ERROR;
	}
	uint32_t inodenum = filedesc_table.fds[fd].inodenum;

	struct fs_inode ind;
	if(fs_read_inode(fs, super, inodenum, &ind) < 0) {
		fprintf(stderr, "io_write: fs_read_inode\n");
		return FUNC_ERROR;
	}
	/* current offset*/
	uint32_t off = filedesc_table.fds[fd].offset;
	/* lazy allocation */
	if(io_lazy_alloc(fs, super, inodenum, &ind, off, size) < 0) {
		fprintf(stderr, "io_write: lazy allocation failed\n");
		return FUNC_ERROR;
	}
	/* actual writing */
	uint32_t direct_off = FS_DIRECT_POINTERS_PER_INODE * FS_BLOCK_SIZE;
	
	uint32_t level0range_s, level0range_e;
	level0range_s = (off < direct_off)? off: direct_off-1;
	level0range_e = (off + size < direct_off)? off+size-1: direct_off-1;
	
	uint32_t level1range_s, level1range_e;
	level1range_s = (off < direct_off)? 0: off-direct_off;
	level1range_e = (off + size < direct_off)? 0: off+size-1-direct_off;

	int data_index = 0;
	
	/* direct writing */
	if(level0range_s == level0range_e) { /* no direct writing needed*/
		goto indirect_writing;
	}
	uint32_t start = level0range_s/FS_BLOCK_SIZE, end = level0range_e/FS_BLOCK_SIZE;
	
	if(start == end) {
		union fs_block datablk;
		if(fs_read_data(fs, super, &datablk, &start, 1) < 0) {
			fprintf(stderr, "io_write: fs_read_data!\n");
			return FUNC_ERROR;
		}
		for(int i=level0range_s%FS_BLOCK_SIZE; i<=level0range_e%FS_BLOCK_SIZE; i++) {
			datablk.data[i] = ((uint8_t*) data) [data_index++];
		}
		if(fs_write_data(fs, super, &datablk, &ind.direct[start], 1) < 0) {
			fprintf(stderr, "io_write: fs_write_data!\n");
			return FUNC_ERROR;
		}
	} else {
		union fs_block datablk_s, datablk_e;
		if(fs_read_data(fs, super, &datablk_s, &ind.direct[start], 1) < 0) {
			fprintf(stderr, "io_write: fs_read_data!\n");
			return FUNC_ERROR;
		} /* todo: combine the two ifs*/
		if(fs_read_data(fs, super, &datablk_e, &ind.direct[end], 1) < 0) {
			fprintf(stderr, "io_write: fs_read_data!\n");
			return FUNC_ERROR;
		}
		/* start */
		for(int i=level0range_s%FS_BLOCK_SIZE; i<FS_BLOCK_SIZE; i++) {
			datablk_s.data[i] = ((uint8_t*) data) [data_index++];
		}
		if(fs_write_data(fs, super, &datablk_s, &ind.direct[start], 1) < 0) {
			fprintf(stderr, "io_write: fs_write_data!\n");
			return FUNC_ERROR;
		}
		
		/* middle */
		for(uint32_t i=start+1; i<=end-1 && start+1 <= end-1; i++) {
			union fs_block* datablk;
			datablk = data  + data_index;
			data_index += FS_BLOCK_SIZE;
			if(fs_write_data(fs, super, datablk, &ind.direct[i], 1) < 0) { /* todo: change write and read to do one block at a time*/
				fprintf(stderr, "io_write: fs_write_data!\n");
				return FUNC_ERROR;
			}
		}
		/* end */
		for(int i=0; i<=level0range_e%FS_BLOCK_SIZE; i++) {
			datablk_e.data[i] = ((uint8_t*) data) [data_index++];
		}
		if(fs_write_data(fs, super, &datablk_e, &ind.direct[end], 1) < 0) {
			fprintf(stderr, "io_write: fs_write_data!\n");
			return FUNC_ERROR;
		}

	}
	
	indirect_writing:
	/* indirect writing */
	if(level1range_s == level1range_e) { /* no indirect writing needed */
		return 0;
	}
	union fs_block indirect_data;
	if(fs_read_data(fs, super, &indirect_data, &ind.indirect, 1) < 0) {
		fprintf(stderr, "io_write: fs_read_data\n");
		return FUNC_ERROR;
	}

	start = level1range_s/FS_BLOCK_SIZE, end = level1range_e/FS_BLOCK_SIZE;
	if(start == end) {
		union fs_block datablk;
		if(fs_read_data(fs, super, &datablk, &indirect_data.pointers[start], 1) < 0) {
			fprintf(stderr, "io_write: fs_read_data!\n");
			return FUNC_ERROR;
		}
		for(int i=level1range_s%FS_BLOCK_SIZE; i<=level1range_e%FS_BLOCK_SIZE; i++) {
			datablk.data[i] = ((uint8_t*) data) [data_index++];
		}
		if(fs_write_data(fs, super, &datablk, &indirect_data.pointers[start], 1) < 0) {
			fprintf(stderr, "io_write: fs_write_data!\n");
			return FUNC_ERROR;
		}
	} else {
		union fs_block datablk_s, datablk_e;
		if(fs_read_data(fs, super, &datablk_s, &indirect_data.pointers[start], 1) < 0) {
			fprintf(stderr, "io_write: fs_read_data!\n");
			return FUNC_ERROR;
		} /* todo: combine the two ifs*/
		if(fs_read_data(fs, super, &datablk_e, &indirect_data.pointers[end], 1) < 0) {
			fprintf(stderr, "io_write: fs_read_data!\n");
			return FUNC_ERROR;
		}
		for(int i=level1range_s%FS_BLOCK_SIZE; i<FS_BLOCK_SIZE; i++) {
			datablk_s.data[i] = ((uint8_t*) data) [data_index++];
		}
		if(fs_write_data(fs, super, &datablk_s, &indirect_data.pointers[start], 1) < 0) {
			fprintf(stderr, "io_write: fs_write_data!\n");
			return FUNC_ERROR;
		} /* todo: combine the two ifs*/

		for(uint32_t i=start+1; i<=end-1 && start+1 <= end-1; i++) {
			union fs_block* datablk;
			datablk = data  + data_index;
			data_index += FS_BLOCK_SIZE;
			if(fs_write_data(fs, super, datablk, &indirect_data.pointers[i], 1) < 0) { /* todo: change write and read to do one block at a time*/
				fprintf(stderr, "io_write: fs_write_data!\n");
				return FUNC_ERROR;
			}
		}
		for(int i=0; i<=level1range_e%FS_BLOCK_SIZE; i++) {
			datablk_e.data[i] = ((uint8_t*) data) [data_index++];
		}
		if(fs_write_data(fs, super, &datablk_e, &indirect_data.pointers[end], 1) < 0) {
			fprintf(stderr, "io_write: fs_write_data!\n");
			return FUNC_ERROR;
		}
	}
	filedesc_table.fds[fd].offset += size;
	return 0;
}
int io_read(struct fs_filesyst fs, struct fs_super_block super, int fd,
			 void* data, size_t size)
{
	if(filedesc_table.fds[fd].is_allocated == 0) {
		fprintf(stderr, "io_read: fd closed!\n");
		return FUNC_ERROR;
	}
	uint32_t inodenum = filedesc_table.fds[fd].inodenum;

	struct fs_inode ind;
	if(fs_read_inode(fs, super, inodenum, &ind) < 0) {
		fprintf(stderr, "io_read: fs_read_inode\n");
		return FUNC_ERROR;
	}
	uint32_t off = filedesc_table.fds[fd].offset;

	uint32_t direct_off = FS_DIRECT_POINTERS_PER_INODE * FS_BLOCK_SIZE;
	
	uint32_t level0range_s, level0range_e;
	level0range_s = (off < direct_off)? off: direct_off-1;
	level0range_e = (off + size < direct_off)? off+size-1: direct_off-1;
	
	uint32_t level1range_s, level1range_e;
	level1range_s = (off < direct_off)? 0: off-direct_off;
	level1range_e = (off + size < direct_off)? 0: off+size-1-direct_off;

	int data_index = 0;
	
	/* direct reading */
	if(level0range_s == level0range_e) { /* no direct writing needed*/
		goto indirect_reading;
	}
	uint32_t start = level0range_s/FS_BLOCK_SIZE, end = level0range_e/FS_BLOCK_SIZE;
	if(start == end) {
		if(!ind.direct[start] || !fs_is_block_allocated(fs, super, ind.direct[start])) {
			for(int i=level0range_s % FS_BLOCK_SIZE; i<level0range_e % FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [data_index++] = 0;
			}
		} else {
			/* else return values */
			union fs_block datablk;
			if(fs_read_data(fs, super, &datablk, &ind.direct[start], 1) < 0) {
				fprintf(stderr, "io_read: fs_read_data!\n");
				return FUNC_ERROR;
			}
			for(int i=level0range_s % FS_BLOCK_SIZE; i<level0range_e % FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [data_index++] = datablk.data[i];
			}
		}
	} else {		
		union fs_block datablk_s, datablk_e;
		/* start */
		if(!ind.direct[start] || !fs_is_block_allocated(fs, super, ind.direct[start])) {
			for(int i=level0range_s%FS_BLOCK_SIZE; i<FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [data_index++] = 0;
			}
		} else {
			if(fs_read_data(fs, super, &datablk_s, &ind.direct[start], 1) < 0) {
				fprintf(stderr, "io_read: fs_read_data!\n");
				return FUNC_ERROR;
			}
			for(int i=level0range_s%FS_BLOCK_SIZE; i<FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [data_index++] = datablk_s.data[i];
			}
		}
		for(uint32_t i=start+1; i<=end-1 && start+1 <= end-1; i++) {
			union fs_block datablk;
			if(!ind.direct[i] || !fs_is_block_allocated(fs, super, ind.direct[i])) {
				for(uint32_t j=0; j<FS_BLOCK_SIZE; j++) {
					((uint8_t*) data)[data_index++] = 0;
				}
			} else {
				if(fs_read_data(fs, super, &datablk, &ind.direct[i], 1) < 0) { /* todo: change write and read to do one block at a time*/
					fprintf(stderr, "io_read: fs_write_data!\n");
					return FUNC_ERROR;
				}
				for(uint32_t j=0; j<FS_BLOCK_SIZE; j++) {
					((uint8_t*) data)[data_index++] = datablk.data[j];
				}
			}
		}
		/* end */
		if(!ind.direct[end] || !fs_is_block_allocated(fs, super, ind.direct[end])) {
			for(int i=0; i<=level0range_e%FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [data_index++] = 0;
			}			
		} else {
			if(fs_read_data(fs, super, &datablk_e, &ind.direct[end], 1) < 0) {
				fprintf(stderr, "io_read: fs_read_data!\n");
				return FUNC_ERROR;
			}
			for(int i=0; i<=level0range_e%FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [data_index++] = datablk_e.data[i];
			}
		}
	}
	
	indirect_reading:
	/* indirect reading */
	if(level1range_s == level1range_e || ind.indirect == 0) { /* no indirect reading needed */
		return 0;
	}
	union fs_block indirect_data;
	if(fs_read_data(fs, super, &indirect_data, &ind.indirect, 1) < 0) {
		fprintf(stderr, "io_read: fs_read_data\n");
		return FUNC_ERROR;
	}

	start = level1range_s/FS_BLOCK_SIZE, end = level1range_e/FS_BLOCK_SIZE;
	if(start == end) {
		union fs_block datablk;
		if(!indirect_data.pointers[start] || !fs_is_block_allocated(fs, super, indirect_data.pointers[start])) {
			for(int i=level0range_s % FS_BLOCK_SIZE; i<level0range_e % FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [data_index++] = 0;
			}		
		} else {
			if(fs_read_data(fs, super, &datablk, &indirect_data.pointers[start], 1) < 0) {
				fprintf(stderr, "io_read: fs_read_data!\n");
				return FUNC_ERROR;
			}
			for(int i=level0range_s % FS_BLOCK_SIZE; i<level0range_e % FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [data_index++] = datablk.data[i];
			}
		}
	} else {
		union fs_block datablk_s, datablk_e;
		/* start */
		if(!indirect_data.pointers[start] || !fs_is_block_allocated(fs, super, indirect_data.pointers[start])) {
			for(int i=level1range_s%FS_BLOCK_SIZE; i<FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [data_index++] = 0;
			}			
		} else { 
			if(fs_read_data(fs, super, &datablk_s, &indirect_data.pointers[start], 1) < 0) {
				fprintf(stderr, "io_read: fs_read_data!\n");
				return FUNC_ERROR;
			}
			for(int i=level1range_s%FS_BLOCK_SIZE; i<FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [data_index++] = datablk_s.data[i];
			}
		}
		
		/* middle */
		for(uint32_t i=start+1; i<=end-1 && start+1 <= end-1; i++) {
			union fs_block datablk;
			if(!indirect_data.pointers[i] || !fs_is_block_allocated(fs, super, indirect_data.pointers[i])) {
				for(uint32_t j=0; j<FS_BLOCK_SIZE; j++) {
					((uint8_t*) data)[data_index++] = 0;
				}
			} else {
				if(fs_read_data(fs, super, &datablk, &indirect_data.pointers[i], 1) < 0) { /* todo: change write and read to do one block at a time*/
					fprintf(stderr, "io_read: fs_write_data!\n");
					return FUNC_ERROR;
				}
				for(uint32_t j=0; j<FS_BLOCK_SIZE; j++) {
					((uint8_t*) data)[data_index++] = datablk.data[j];
				}
			}
		}
		/* end */
		if(!indirect_data.pointers[end] || !fs_is_block_allocated(fs, super, indirect_data.pointers[end])) {
			for(int i=0; i<=level1range_e%FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [data_index++] = 0;
			}			
		} else {
			if(fs_read_data(fs, super, &datablk_e, &indirect_data.pointers[end], 1) < 0) {
				fprintf(stderr, "io_read: fs_read_data!\n");
				return FUNC_ERROR;
			}
			for(int i=0; i<=level1range_e%FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [data_index++] = datablk_e.data[i];
			}
		}
	}
	return 0;
}
