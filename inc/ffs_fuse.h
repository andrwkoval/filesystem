#ifndef FFS_FUSE_H
#define FFS_FUSE_H

#include <fcntl.h>
#include <fuse.h>

int ffs_statfs(const char *path, struct statvfs *statv);

int ffs_opendir(const char *path, struct fuse_file_info *fi);

int ffs_open(const char *path, struct fuse_file_info *fi);

int ffs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

int ffs_getattr(const char *path, struct stat *statbuf);

void *ffs_init(struct fuse_conn_info *conn);

void ffs_destroy(void *userdata);

#endif //FFS_FUSE_H
