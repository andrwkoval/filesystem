#ifndef FFS_H
#define FFS_H

#include <stdint.h>

#define FFS_BLOCKSIZE 2048
#define FFS_LOG_BLOCK_SIZE 1
#define FFS_BLOCKS_PER_GROUP 2048
#define FFS_INODES_PER_GROUP 2048
#define FFS_INODE_TABLE_BLOCKS (sizeof(ffs_inode_t) * FFS_INODES_PER_GROUP / FFS_BLOCKSIZE)
#define FFS_RESERVED_INODES 11
#define FFS_MAGIC 0xef53
#define FFS_FILESYSTEM_STATE 1
#define FFS_ERROR_HANDLER 1

typedef struct ffs_superblock {
    uint32_t sb_inodes_count;
    uint32_t sb_blocks_count;
    uint32_t sb_pad1;
    uint32_t sb_free_blocks_count;
    uint32_t sb_free_inodes_count;
    uint32_t sb_pad2;
    uint32_t sb_log_block_size;
    int32_t sb_log_frag_size;
    uint32_t sb_blocks_per_group;
    uint32_t sb_frags_per_group;
    uint32_t sb_inodes_per_group;
    uint16_t sb_pad3[5];
    uint16_t sb_max_mnt_count;
    uint16_t sb_magic;
    uint16_t sb_state;
    uint16_t sb_errors;
    uint16_t sb_minor_rev_level;
    uint32_t sb_pad4;
    uint32_t sb_checkinterval;
    uint32_t sb_pad5;
    uint32_t sb_rev_level;
    uint64_t pad6[118];
} ffs_sb_t;

typedef struct ffs_block_group_descriptor {
    uint32_t bgd_block_bitmap;
    uint32_t bgd_inode_bitmap;
    uint32_t bgd_inode_table;
    uint16_t bgd_free_blocks_count;
    uint16_t bgd_free_inodes_count;
    uint16_t bgd_used_dirs_count;
    uint16_t bgd_pad[7];
} ffs_bgd_t;

typedef struct ffs_inode {
    uint16_t i_mode;
    uint16_t i_uid;
    int32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint64_t i_pad1;
    uint32_t i_block[15];
    uint32_t i_pad2[5];
    uint16_t i_uid_high;
    uint16_t i_gid_high;
    uint32_t i_pad3;
} ffs_inode_t;

typedef struct ffs_block {
    uint8_t b_data[2048];
} ffs_block_t;

typedef struct ffs_block_group {
    uint8_t bg_block_bitmap[2048];
    uint8_t bg_inode_bitmap[2048];
    ffs_inode_t bg_inode_table[2048];
    ffs_block_t bg_data_blocks[2048];
} ffs_bg_t;

#define FFS_DIR_ENTRY_RECORD_LENGTH 256
#define FFS_FILENAME_MAX_LENGTH 247

typedef struct ffs_dir_entry {
    uint32_t de_inode;
    uint16_t de_rec_len;
    uint16_t de_name_len;
    uint8_t de_name[248];
} ffs_de_t;

#endif //FFS_H
