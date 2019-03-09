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
	
	//~ uint32_t level1range_s, level1range_e;
	//~ level1range_s = (level0range_e < direct_off)? -1: /* no indirect blocks needed */
					//~ (level0range_s == direct_off )? off: /* only indirect blocks needed */
					 //~ 0; /* direct and indirect needed */
	//~ level1range_e = (level0range_e < direct_off)? -1: /* no indirect blocks needed */
					//~ (level0range_s == direct_off)? off+size-1: /* only indirect blocks needed */
					//~ (off+size-(direct_off+off))-1; /* direct and indirect needed */
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
			allocs_needed += ind->indirect == 0;
			continue;
		}
		allocs_needed += ind->direct[i] == 0;
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
	//~ printf("all_need1 = %d\n", allocs_needed);
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
		allocs_needed += indirect_data.pointers[i] == 0;
	}
	//~ printf("all_need2 = %d\n", allocs_needed);
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
	
	/* direct writing */
	if(level0range_s == level0range_e) { /* no direct writing needed*/
		goto indirect_writing;
	}
	uint32_t start = level0range_s/FS_BLOCK_SIZE, end = level0range_e/FS_BLOCK_SIZE;
	printf("really %d %d %d %d\n", level0range_s, level0range_e, level1range_s, level1range_e);
	printf("start = %d; end = %d\n", start, end);
	int data_index = 0;
	if(start == end) {
		union fs_block datablk;
		if(fs_read_data(fs, super, &datablk, &start, 1) < 0) {
			fprintf(stderr, "io_write: fs_read_data!\n");
			return FUNC_ERROR;
		}
		for(int i=level0range_s%FS_BLOCK_SIZE; i<=level0range_e%FS_BLOCK_SIZE; i++) {
			//~ printf("[%d]%d %d\n", start, i, i-level0range_s % FS_BLOCK_SIZE);
			datablk.data[i] = ((uint8_t*) data) [i-level0range_s % FS_BLOCK_SIZE];
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
		//~ printf("blk = %d\n", ind.direct[start]);
		//~ printf("blk = %d\n", ind.direct[end]);

		//~ printf("%d %d\n", level0range_s, level0range_e);
		//~ printf("srt = %d %d\n",level0range_s%FS_BLOCK_SIZE, FS_BLOCK_SIZE);
		//~ printf("end = %d %d\n",0, level0range_e%FS_BLOCK_SIZE);
		for(int i=level0range_s%FS_BLOCK_SIZE; i<FS_BLOCK_SIZE; i++) {
			printf("[%d]%d %d\n", start, i, i-level0range_s%FS_BLOCK_SIZE);
			datablk_s.data[i] = ((uint8_t*) data) [i-level0range_s%FS_BLOCK_SIZE];
			//~ printf("[%d] = %c\n",i,  datablk_s.data[i]);
		}
		for(int i=0; i<=level0range_e%FS_BLOCK_SIZE; i++) {
			printf("[%d]%d %d\n", end, i, i+end*FS_BLOCK_SIZE);
			datablk_e.data[i] = ((uint8_t*) data) [i+(level0range_e-level0range_e%FS_BLOCK_SIZE)];
		}
		if(fs_write_data(fs, super, &datablk_s, &ind.direct[start], 1) < 0) {
			fprintf(stderr, "io_write: fs_write_data!\n");
			return FUNC_ERROR;
		} /* todo: combine the two ifs*/
		if(fs_write_data(fs, super, &datablk_e, &ind.direct[end], 1) < 0) {
			fprintf(stderr, "io_write: fs_write_data!\n");
			return FUNC_ERROR;
		}
		
		for(uint32_t i=start+1; i<=end-1 && start+1 <= end-1; i++) {
			printf("[%d]%d %d\n", i, i, (i+1)*FS_BLOCK_SIZE - (FS_BLOCK_SIZE-level0range_s%FS_BLOCK_SIZE));
			//~ printf("blk = %d\n", ind.direct[i]);
			union fs_block* datablk;
			datablk = data  + (i+1)*FS_BLOCK_SIZE - (FS_BLOCK_SIZE-level0range_s%FS_BLOCK_SIZE);
			//~ printf("d[%d]%d %d\n", i, i, (i+1)*FS_BLOCK_SIZE - (FS_BLOCK_SIZE-level0range_s%FS_BLOCK_SIZE));
			if(fs_write_data(fs, super, datablk, &ind.direct[i], 1) < 0) { /* todo: change write and read to do one block at a time*/
				fprintf(stderr, "io_write: fs_write_data!\n");
				return FUNC_ERROR;
			}
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
	uint32_t indirect_offset = level0range_e-level0range_s;
	//~ printf("%d %d\n", direct_off - off, level0range_e-level0range_s);
	//~ printf("start = %d; end = %d\n", start, end);

	if(start == end) {
		union fs_block datablk;
		if(fs_read_data(fs, super, &datablk, &indirect_data.pointers[start], 1) < 0) {
			fprintf(stderr, "io_write: fs_read_data!\n");
			return FUNC_ERROR;
		}
		//~ printf("%d %d\n",level1range_s % FS_BLOCK_SIZE, level1range_e % FS_BLOCK_SIZE);
		for(int i=level1range_s%FS_BLOCK_SIZE; i<=level1range_e%FS_BLOCK_SIZE; i++) {
			//~ printf("[%d]%d %d\n", start, i, i-level1range_s % FS_BLOCK_SIZE+indirect_offset);
			datablk.data[i] = ((uint8_t*) data) [i-level1range_s % FS_BLOCK_SIZE+indirect_offset];
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
		//~ printf("iblk = %d\n", indirect_data.pointers[start]);
		//~ printf("iblk = %d\n", indirect_data.pointers[end]);
		//~ printf("srt = %d %d\n",level1range_s%FS_BLOCK_SIZE, FS_BLOCK_SIZE);
		//~ printf("end = %d %d\n",0, level1range_e%FS_BLOCK_SIZE);
		for(int i=level1range_s%FS_BLOCK_SIZE; i<FS_BLOCK_SIZE; i++) {
			printf("[%d]%d %d\n", start, i, i-level1range_s%FS_BLOCK_SIZE+indirect_offset);
			datablk_s.data[i] = ((uint8_t*) data) [i-level1range_s%FS_BLOCK_SIZE+indirect_offset];
		}
		for(int i=0; i<=level1range_e%FS_BLOCK_SIZE; i++) {
			printf("[%d]%d %d\n", end, i, i+level1range_e-level1range_e%FS_BLOCK_SIZE+indirect_offset);
			datablk_e.data[i] = ((uint8_t*) data) [i+level1range_e-level1range_e%FS_BLOCK_SIZE+indirect_offset];
		}
		if(fs_write_data(fs, super, &datablk_s, &indirect_data.pointers[start], 1) < 0) {
			fprintf(stderr, "io_write: fs_write_data!\n");
			return FUNC_ERROR;
		} /* todo: combine the two ifs*/
		if(fs_write_data(fs, super, &datablk_e, &indirect_data.pointers[end], 1) < 0) {
			fprintf(stderr, "io_write: fs_write_data!\n");
			return FUNC_ERROR;
		}

		for(uint32_t i=start+1; i<=end-1 && start+1 <= end-1; i++) {
			printf("i[%d]%d %d\n", i, i, (i+1)*FS_BLOCK_SIZE - (FS_BLOCK_SIZE-level1range_s%FS_BLOCK_SIZE)+indirect_offset);
			union fs_block* datablk;
			datablk = data  + (i+1)*FS_BLOCK_SIZE - (FS_BLOCK_SIZE-level1range_s%FS_BLOCK_SIZE)+indirect_offset;
			//~ printf("iblk = %d\n", indirect_data.pointers[i]);
			//~ printf("i[%d]%d %d\n", i, i, (i+1)*FS_BLOCK_SIZE - (FS_BLOCK_SIZE-level0range_s%FS_BLOCK_SIZE));
			
			if(fs_write_data(fs, super, datablk, &indirect_data.pointers[i], 1) < 0) { /* todo: change write and read to do one block at a time*/
				fprintf(stderr, "io_write: fs_write_data!\n");
				return FUNC_ERROR;
			}
		}
	}
	filedesc_table.fds[fd].offset += size;
	//~ printf("%d\n", filedesc_table.fds[fd].offset);
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
	
	/* direct reading */
	if(level0range_s == level0range_e) { /* no direct writing needed*/
		goto indirect_writing;
	}
	//~ union fs_block null_block;
	//~ memset(null_block, 0, sizeof(null_block));
	uint32_t start = level0range_s/FS_BLOCK_SIZE, end = level0range_e/FS_BLOCK_SIZE;
	if(start == end) {
		//~ printf("start==end direct\n");
		/* if not allocated return null block*/
		if(!ind.direct[start] || !fs_is_block_allocated(fs, super, ind.direct[start])) {
			for(int i=level0range_s % FS_BLOCK_SIZE; i<level0range_e % FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [i-level0range_s % FS_BLOCK_SIZE] = 0;
			}
		} else {
			/* else return values */
			union fs_block datablk;
			if(fs_read_data(fs, super, &datablk, &ind.direct[start], 1) < 0) {
				fprintf(stderr, "io_read: fs_read_data!\n");
				return FUNC_ERROR;
			}
			for(int i=level0range_s % FS_BLOCK_SIZE; i<level0range_e % FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [i-level0range_s % FS_BLOCK_SIZE] = datablk.data[i];
			}
		}
	} else {
		//~ printf("start direct\n");

		union fs_block datablk_s, datablk_e;
		if(!ind.direct[start] || !fs_is_block_allocated(fs, super, ind.direct[start])) {
			for(int i=level0range_s%FS_BLOCK_SIZE; i<FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [i-level0range_s%FS_BLOCK_SIZE] = 0;
			}
		} else {
			if(fs_read_data(fs, super, &datablk_s, &ind.direct[start], 1) < 0) {
				fprintf(stderr, "io_read: fs_read_data!\n");
				return FUNC_ERROR;
			} /* todo: combine the two ifs*/
			//~ printf("read = %c\n", datablk_s.data[0]);
			for(int i=level0range_s%FS_BLOCK_SIZE; i<FS_BLOCK_SIZE; i++) {
				//~ printf("sd[%d]%d %d\n", start, i, i-level0range_s%FS_BLOCK_SIZE);
				((uint8_t*) data) [i-level0range_s%FS_BLOCK_SIZE] = datablk_s.data[i];
			}
		}
		//~ printf("end direct\n");

		if(!ind.direct[end] || !fs_is_block_allocated(fs, super, ind.direct[end])) {
			for(int i=0; i<=level0range_e%FS_BLOCK_SIZE; i++) {
				//~ printf("ed[%d]%d %d\n", end, i, i+level0range_e-level0range_e%FS_BLOCK_SIZE);
				((uint8_t*) data) [i+level0range_e-level0range_e%FS_BLOCK_SIZE] = 0;
			}			
		} else {
			if(fs_read_data(fs, super, &datablk_e, &ind.direct[start], 1) < 0) {
				fprintf(stderr, "io_read: fs_read_data!\n");
				return FUNC_ERROR;
			}
			for(int i=0; i<=level0range_e%FS_BLOCK_SIZE; i++) {
				//~ printf("ed[%d]%d %d\n", end, i, i+level0range_e-level0range_e%FS_BLOCK_SIZE);
				((uint8_t*) data) [i+level0range_e-level0range_e%FS_BLOCK_SIZE] = datablk_e.data[i];
			}
		}
		//~ printf("blk = %d\n", ind.direct[start]);
		//~ printf("blk = %d %d\n",end, ind.direct[end]);
		//~ printf("%d %d\n", start, end);
		//~ printf("%d %d\n", start, end);
		for(uint32_t i=start+1; i<=end-1 && start+1 <= end-1; i++) {
			//~ printf("d[%d]%d %d\n", i, i, (i+1)*FS_BLOCK_SIZE - (FS_BLOCK_SIZE-level0range_s%FS_BLOCK_SIZE));
			union fs_block datablk;
			if(!ind.direct[i] || !fs_is_block_allocated(fs, super, ind.direct[i])) {
				for(uint32_t j=0; j<FS_BLOCK_SIZE; j++) {
					//~ printf("[%d]%d %d\n", i, i*FS_BLOCK_SIZE + j - level0range_s%FS_BLOCK_SIZE, j);
					((uint8_t*) data)[j+(i+1)*FS_BLOCK_SIZE - (FS_BLOCK_SIZE-level0range_s%FS_BLOCK_SIZE)] = 0;
				}				
			} else {
				if(fs_read_data(fs, super, &datablk, &ind.direct[i], 1) < 0) { /* todo: change write and read to do one block at a time*/
					fprintf(stderr, "io_read: fs_write_data!\n");
					return FUNC_ERROR;
				}
				for(uint32_t j=0; j<FS_BLOCK_SIZE; j++) {
					//~ printf("[%d]%d %d\n", i, i*FS_BLOCK_SIZE + j - level0range_s%FS_BLOCK_SIZE, j);
					((uint8_t*) data)[j+(i+1)*FS_BLOCK_SIZE - (FS_BLOCK_SIZE-level0range_s%FS_BLOCK_SIZE)] = datablk.data[j];
				}
			}
		}
	}
	
	indirect_writing:
	/* indirect reading */
	if(level1range_s == level1range_e) { /* no indirect reading needed */
		return 0;
	}
	union fs_block indirect_data;
	if(fs_read_data(fs, super, &indirect_data, &ind.indirect, 1) < 0) {
		fprintf(stderr, "io_read: fs_read_data\n");
		return FUNC_ERROR;
	}

	start = level1range_s/FS_BLOCK_SIZE, end = level1range_e/FS_BLOCK_SIZE;
	uint32_t indirect_offset = level0range_e-level0range_s;
	//~ printf("%d %d\n", direct_off - off, level0range_e-level0range_s);
	if(start == end) {
		union fs_block datablk;
		//~ printf("start==end indirect\n");
		if(!indirect_data.pointers[start] || !fs_is_block_allocated(fs, super, indirect_data.pointers[start])) {
			for(int i=level0range_s % FS_BLOCK_SIZE; i<level0range_e % FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [i-level1range_s % FS_BLOCK_SIZE+indirect_offset] = 0;
			}		
		} else {
			if(fs_read_data(fs, super, &datablk, &indirect_data.pointers[start], 1) < 0) {
				fprintf(stderr, "io_read: fs_read_data!\n");
				return FUNC_ERROR;
			}
			for(int i=level0range_s % FS_BLOCK_SIZE; i<level0range_e % FS_BLOCK_SIZE; i++) {
				((uint8_t*) data) [i-level1range_s % FS_BLOCK_SIZE+indirect_offset] = datablk.data[i];
			}
		}
	} else {
		//~ printf("start indirect %d\n", indirect_data.pointers[start]);
		union fs_block datablk_s, datablk_e;
		if(!indirect_data.pointers[start] || !fs_is_block_allocated(fs, super, indirect_data.pointers[start])) {
			for(int i=level1range_s%FS_BLOCK_SIZE; i<FS_BLOCK_SIZE; i++) {
				//~ printf("[%d]%d %d\n", start, i-level0range_s%FS_BLOCK_SIZE, i);
				((uint8_t*) data) [i-level1range_s%FS_BLOCK_SIZE+indirect_offset] = 0;
			}			
		} else { 
			if(fs_read_data(fs, super, &datablk_s, &indirect_data.pointers[start], 1) < 0) {
				fprintf(stderr, "io_read: fs_read_data!\n");
				return FUNC_ERROR;
			} /* todo: combine the two ifs*/
			for(int i=level1range_s%FS_BLOCK_SIZE; i<FS_BLOCK_SIZE; i++) {
				//~ printf("si[%d]%d %d\n", start, i-level1range_s%FS_maLOCK_SIZE+indirect_offset, i);
				((uint8_t*) data) [i-level1range_s%FS_BLOCK_SIZE+indirect_offset] = datablk_s.data[i];
			}
		}
		//~ printf("end indirect\n");
		if(!indirect_data.pointers[end] || !fs_is_block_allocated(fs, super, indirect_data.pointers[end])) {
			for(int i=0; i<=level1range_e%FS_BLOCK_SIZE; i++) {
				//~ printf("ei[%d]%d %d\n", end, i+size-level0range_e%FS_BLOCK_SIZE-1, i);
				((uint8_t*) data) [i+size-level1range_e%FS_BLOCK_SIZE-1] = 0;
			}			
		} else {
			if(fs_read_data(fs, super, &datablk_e, &indirect_data.pointers[end], 1) < 0) {
				fprintf(stderr, "io_read: fs_read_data!\n");
				return FUNC_ERROR;
			}
			for(int i=0; i<=level1range_e%FS_BLOCK_SIZE; i++) {
				//~ printf("ei[%d]%d %d\n", end, i+size-level0range_e%FS_BLOCK_SIZE-1, i);
				((uint8_t*) data) [i+size-level1range_e%FS_BLOCK_SIZE-1] = datablk_e.data[i];
			}
		}
		//~ printf("iblk = %d\n", indirect_data.pointers[start]);
		//~ printf("iblk = %d\n", indirect_data.pointers[end]);
		for(uint32_t i=start+1; i<=end-1 && start+1 <= end-1; i++) {
			union fs_block datablk;
			if(!indirect_data.pointers[i] || !fs_is_block_allocated(fs, super, indirect_data.pointers[i])) {
				for(uint32_t j=0; j<FS_BLOCK_SIZE; j++) {
					//~ printf("[%d]%d %d\n", i, i*FS_BLOCK_SIZE + j - level0range_s%FS_BLOCK_SIZE, j);
					((uint8_t*) data)[j+(i+1)*FS_BLOCK_SIZE - (FS_BLOCK_SIZE-level1range_s%FS_BLOCK_SIZE)+indirect_offset] = 0;
				}
			} else {
				//~ printf("i[%d]%d %d\n", i, i, (i+1)*FS_BLOCK_SIZE - (FS_BLOCK_SIZE-level1range_s%FS_BLOCK_SIZE)+indirect_offset);
				if(fs_read_data(fs, super, &datablk, &indirect_data.pointers[i], 1) < 0) { /* todo: change write and read to do one block at a time*/
					fprintf(stderr, "io_read: fs_write_data!\n");
					return FUNC_ERROR;
				}
				//~ printf("iblk = %d\n", indirect_data.pointers[i]);
				for(uint32_t j=0; j<FS_BLOCK_SIZE; j++) {
					//~ printf("[%d]%d %d\n", i, i*FS_BLOCK_SIZE + j - level0range_s%FS_BLOCK_SIZE, j);
					((uint8_t*) data)[j+(i+1)*FS_BLOCK_SIZE - (FS_BLOCK_SIZE-level1range_s%FS_BLOCK_SIZE)+indirect_offset] = datablk.data[j];
				}
			}
		}
	}
	
	return 0;
}
