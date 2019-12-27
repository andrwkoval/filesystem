#ifndef FFS_MKFS_H
#define FFS_MKFS_H

#include "ffs.h"

void ffs_write_superblock(int fd, uint64_t bgn, uint64_t bgdt_blocks);

void ffs_write_bgd_table(int fd, uint64_t bgn, uint64_t bgdt_blocks);

void ffs_write_block_groups(int fd, uint64_t bgn, uint64_t bgdt_blocks);

#endif //FFS_MKFS_H
