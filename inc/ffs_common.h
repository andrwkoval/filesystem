#ifndef FFS_COMMON_H
#define FFS_COMMON_H

#define FUSE_USE_VERSION 26

#include <ffs.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

struct ffs_init_data {
    char *source;
};

#define FFS_DATA ((struct ffs_init_data *) fuse_get_context()->private_data)

ssize_t writebuff(int fd, void *buffer, size_t size);

void bitmap_set_bit(uint8_t *bitmap, uint16_t bit, uint8_t val);

uint8_t read_superblock(FILE *fs, ffs_sb_t *sb);

uint8_t read_inode(FILE *fs, uint64_t inodei, ffs_inode_t *inode);

int64_t entry_inode_no(FILE *fs, ffs_inode_t *inode, char *entry_name);

int64_t path_to_inode(FILE *fs, const char *path);

uint8_t write_inode(FILE *fs, uint64_t inodei, ffs_inode_t *inode);

#endif //FFS_COMMON_H
