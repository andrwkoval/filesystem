#include "ffs_common.h"
#include "ffs_fuse.h"

#include <fuse.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int ffs_statfs(const char *path, struct statvfs *statv) {
    int fd;
    if ((fd = open(FFS_DATA->source, O_RDONLY)) < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (lseek(fd, 1024, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        return EXIT_FAILURE;
    }

    ffs_sb_t sb;
    if (readbuff(fd, &sb, sizeof(sb)) == -1) {
        perror("write");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);

    statv->f_bsize = 1024 << sb.sb_log_block_size;
    statv->f_blocks = sb.sb_blocks_count;
    statv->f_bfree = sb.sb_free_blocks_count;
    statv->f_bavail = sb.sb_free_blocks_count;
    statv->f_files = sb.sb_inodes_count;
    statv->f_ffree = sb.sb_free_inodes_count;
    statv->f_favail = sb.sb_free_inodes_count;
    statv->f_namemax = FFS_FILENAME_MAX_LENGTH;

    return EXIT_SUCCESS;
}

int ffs_opendir(const char *path, struct fuse_file_info *fi) {
    return EXIT_SUCCESS;
}

int ffs_open(const char *path, struct fuse_file_info *fi) {
    return EXIT_SUCCESS;
}

int ffs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    uint64_t inum;
    if ((inum = path_to_inode(path, FFS_DATA->source)) == EXIT_FAILURE) {
        return -EXIT_FAILURE;
    }

    ffs_inode_t inode;
    if (read_inode(inum, &inode, FFS_DATA->source) == EXIT_FAILURE) {
        return -EXIT_FAILURE;
    }

    uint64_t blocks = inode.i_size / FFS_BLOCKSIZE;
    if (blocks > 12) {
        blocks = 12;
    }

    ffs_de_t *des = (ffs_de_t *) calloc(sizeof(ffs_de_t), FFS_BLOCKSIZE / sizeof(ffs_de_t) * blocks);
    if (des == NULL) {
        perror("calloc");
        return -EXIT_FAILURE;
    }

    int fd;
    if ((fd = open(FFS_DATA->source, O_RDONLY)) < 0) {
        perror("open");
        return -EXIT_FAILURE;
    }

    for (size_t i = 0; i < FFS_BLOCKSIZE / sizeof(ffs_de_t) * blocks; ++i) {
        if (lseek(fd, FFS_BLOCKSIZE * inode.i_block[i], SEEK_SET) == -1) {
            perror("lseek");
            close(fd);
            return EXIT_FAILURE;
        }
        if (readbuff(fd, des + i, sizeof(ffs_de_t)) == -1) {
            perror("read");
            close(fd);
            return EXIT_FAILURE;
        }
    }

    close(fd);

    for (uint64_t i = 0; i < FFS_BLOCKSIZE / sizeof(ffs_de_t) * blocks; ++i) {
        if (des[i].de_name_len != 0) {
            char *name = (char *) malloc(des[i].de_name_len + 1);
            if (name == NULL) {
                perror("malloc");
                return -EXIT_FAILURE;
            }
            for (uint64_t j = 0; j < des[i].de_name_len; ++j) {
                name[j] = des[i].de_name[j];
            }
            name[des[i].de_name_len] = 0;
            if (filler(buf, name, NULL, 0) != 0) {
                free(name);
                return -EXIT_FAILURE;
            }
            free(name);
        }
    }

    return EXIT_SUCCESS;
}

int ffs_getattr(const char *path, struct stat *statbuf) {

}

void *ffs_init(struct fuse_conn_info *conn) {
    return FFS_DATA;
}

void ffs_destroy(void *userdata) {}
