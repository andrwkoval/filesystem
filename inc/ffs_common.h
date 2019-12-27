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

ssize_t readbuff(int fd, void *buffer, size_t size);

void bitmap_set_bit(uint8_t *bitmap, uint16_t bit, uint8_t val);

ssize_t read_inode(uint64_t inodei, ffs_inode_t *inode, char *fs);

int64_t entry_inode_no(ffs_inode_t *inode, char *entry_name, char *fs);

int64_t path_to_inode(const char *path, char *fs);

#endif //FFS_COMMON_H
