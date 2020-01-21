#ifndef FFS_FUSE_H
#define FFS_FUSE_H

#include <fcntl.h>
#include <fuse.h>

int ffs_statfs(const char *path, struct statvfs *statv);

int ffs_opendir(const char *path, struct fuse_file_info *fi);

int ffs_open(const char *path, struct fuse_file_info *fi);

int ffs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

int ffs_getattr(const char *path, struct stat *statbuf);

int ffs_chmod(const char *path, mode_t mode);

int ffs_chown(const char *path, uid_t uid, gid_t gid);

int ffs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

int ffs_access(const char *path, int mask);

int ffs_utimens(const char *path, const struct timespec tv[2]);

int ffs_getxattr(const char *path, const char *name, char *value, size_t size);

void *ffs_init(struct fuse_conn_info *conn);

void ffs_destroy(void *userdata);

#endif //FFS_FUSE_H
