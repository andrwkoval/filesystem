#ifndef FFS_COMMON_H
#define FFS_COMMON_H

#define FUSE_USE_VERSION 26

#include <stdint.h>
#include <stdio.h>

ssize_t writebuff(int fd, void *buffer, size_t size);

ssize_t readbuff(int fd, void *buffer, size_t size);

void bitmap_set_bit(uint8_t *bitmap, uint16_t bit, uint8_t val);

#include <limits.h>

struct ffs_init_data {
    char *rootdir;
};

#define FFS_DATA ((struct ffs_init_data *) fuse_get_context()->private_data)

void ffs_resolve_path(char fpath[PATH_MAX], const char *path);

#endif //FFS_COMMON_H
