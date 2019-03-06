/**
 * @file fs.h
 * @author ABDELMOUMENE Djahid 
 * @author AYAD Ishak
 * @brief filesystem function header
 * @details main filesystem structs and prototypes
 */
#ifndef FS_H
#define FS_H
#include <stdint.h>

#include <disk.h>


#define FS_MAGIC 0xF0F03410 		   /* magic number for our filesystem */
#define FS_POINTERS_PER_BLOCK 1024     /* no of pointers (used by inodes) per block in bytes*/
#define FS_INODES_PER_BLOCK 64 		   /* no of inodes per block */
#define FS_DIRECT_POINTERS_PER_INODE 8 /* no of direct data pointers in each inode */
#define FS_INODE_RATIO 0.01 /* total ratio of inodes in the fs */
#define FS_MAX_INODE_COUNT (NO_BYTES_32 / (FS_BLOCK_SIZE * FS_INODES_PER_BLOCK))
					/* maximum no of inodes blocks that can be referenced
					 *  with 32 bits in the directory entries */

/**
 * @brief super block structure
 * @details the structure of the super block the first block stored
 * stored in memory contains general information about the filesystem
 * and other useful information, with a total size of 40 bytes.
 */
struct fs_super_block  {
	uint32_t magic; 		   /**< the filesystem magic number */
	
	uint32_t data_bitmap_loc;  /**< data bitmap location in block num */
	uint32_t data_bitmap_size; /**< data bitmap size in blocks */
	uint32_t inode_bitmap_loc; /**< inode bitmap location in block num */
	uint32_t inode_bitmap_size;/**< inode bitmap size in blocks */
	
	uint32_t inode_loc; 	   /**< location of inodes in block num */
	uint32_t inode_count; 	   /**< no of inodes in blocks */
	uint32_t data_loc; 		   /**< location of the data in blocks */
	uint32_t data_count; 	   /**< no of data blocks */
	
	uint32_t free_inode_count; /**< no of free inodes */
	uint32_t free_data_count;  /**< no of free blocks */
	
	uint32_t nreads;  		   /**< number of reads performed */
	uint32_t nwrites;		   /**< number of writes performed*/
	uint32_t mounts;  		   /**< number of mounts*/
	
	uint32_t mtime;			   /**< time of mount of the filesystem */
	uint32_t wtime; 		   /**< last write time */
};

/**
 * @brief inode structure
 * @details the structure of inodes contains information about one file
 * with a total size of 54 bytes.
 */
struct fs_inode {
	uint16_t mode; 								  /**< file type and permissions */
	uint16_t uid; 								  /**< id of owner */
	uint16_t gid; 								  /**< group id of owners */
	uint32_t atime; 							  /**< last access time in seconds since the epoch */
	uint32_t mtime; 							  /**< last modification time in seconds since the epoch*/
	uint32_t size; 								  /**< size of the file in bytes */
	uint32_t direct[FS_DIRECT_POINTERS_PER_INODE];/**< direct data blocks */
	uint32_t indirect; 							  /**< indirect data blocks */
};

/**
 * @brief union of a block structure
 * @details a block can either be a super block, or and array of inodes
 * or an array of pointers to other blocks, or an array of data  bytes.
 */
union fs_block {
	struct fs_super_block super; 				/**< super block */
	struct fs_inode inodes[FS_INODES_PER_BLOCK];/**< array of inodes */
	uint32_t pointers[FS_POINTERS_PER_BLOCK];   /**< array of pointers */
	uint8_t data[FS_BLOCK_SIZE]; 				/**< array of data bytes */
};

/* prototypes */
int fs_format_super(struct fs_filesyst fs);
int fs_dump_super(struct fs_filesyst fs);
int fs_format(struct fs_filesyst fs);
int fs_alloc_inode(struct fs_filesyst fs, struct fs_super_block* super, uint32_t *inodenum);
int fs_read_inode(struct fs_filesyst fs, struct fs_super_block super,
				   uint32_t indno, struct fs_inode *inode);
int fs_dump_inode(struct fs_filesyst fs, struct fs_super_block super, uint32_t inodenum);
int fs_alloc_data(struct fs_filesyst fs, struct fs_super_block* super, uint32_t data[], size_t size);
int fs_write_inode(struct fs_filesyst fs, struct fs_super_block super, uint32_t indno, struct fs_inode *inode);
int fs_free_inode(struct fs_filesyst fs, struct fs_super_block* super, uint32_t inodenum);
int fs_free_data(struct fs_filesyst fs, struct fs_super_block* super, uint32_t datanum);
int fs_is_block_allocated(struct fs_filesyst fs, struct fs_super_block super, uint32_t datanum);
int fs_is_inode_allocated(struct fs_filesyst fs, struct fs_super_block super, uint32_t inodenum); 
int fs_write_data(struct fs_filesyst fs, struct fs_super_block super,
				  union fs_block *data, uint32_t *blknums, size_t size);
int fs_read_data(struct fs_filesyst fs, struct fs_super_block super,
				 union fs_block *data, uint32_t *blknums, size_t size);
#endif
