#include "ffs_common.h"
#include "ffs_fuse.h"

#include <errno.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ffs_statfs(const char *path, struct statvfs *statv) {
    // open file
    FILE *fs;
    if ((fs = fopen(FFS_DATA->source, "rb")) == NULL) {
        return -EXIT_FAILURE;
    }

    // read superblock
    ffs_sb_t sb;
    if (read_superblock(fs, &sb) == EXIT_FAILURE) {
        fclose(fs);
        return -EXIT_FAILURE;
    }

    fclose(fs);

    statv->f_bsize = 1024 << sb.sb_log_block_size;
    statv->f_blocks = sb.sb_blocks_count;
    statv->f_bfree = sb.sb_free_blocks_count;
    statv->f_bavail = sb.sb_free_blocks_count;
    statv->f_files = sb.sb_inodes_count;
    statv->f_ffree = sb.sb_free_inodes_count;
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
    // open file
    FILE *fs;
    if ((fs = fopen(FFS_DATA->source, "rb")) == NULL) {
        return -EXIT_FAILURE;
    }

    // find inode number
    uint64_t inum;
    if ((inum = path_to_inode(fs, path)) == EXIT_FAILURE) {
        fclose(fs);
        return -EXIT_FAILURE;
    }

    // read inode
    ffs_inode_t inode;
    if (read_inode(fs, inum, &inode) == EXIT_FAILURE) {
        fclose(fs);
        return -EXIT_FAILURE;
    }

    // number of blocks used by inode
    uint64_t blocks = inode.i_size / sizeof(ffs_block_t);
    if (blocks > 12) {
        blocks = 12;
    }

    for (size_t block = 0; block < blocks; ++block) {
        // move file pointer to record length of the first directory entry on the block
        if (fseek(fs, sizeof(ffs_block_t) * inode.i_block[block] + 4, SEEK_SET) == -1) {
            fclose(fs);
            return -EXIT_FAILURE;
        }

        uint16_t rl;
        do {
            // read entry length
            if (fread(&rl, 2, 1, fs) != 1) {
                fclose(fs);
                return -EXIT_FAILURE;
            }
            if (rl != 0) {
                // read entry name length
                uint16_t namelen;
                if (fread(&namelen, 2, 1, fs) != 1) {
                    fclose(fs);
                    return -EXIT_FAILURE;
                }
                // allocate memory for entry name
                char name[248];
                // read entry name
                if (fread(name, namelen, 1, fs) != 1) {
                    fclose(fs);
                    return -EXIT_FAILURE;
                }
                name[namelen] = '\0';
                if (filler(buf, name, NULL, 0) != 0) {
                    fclose(fs);
                    return -EXIT_FAILURE;
                }
                // move file pointer to next entry's record length
                if (fseek(fs, rl - namelen - 4, SEEK_CUR) == -1) {
                    fclose(fs);
                    return -EXIT_FAILURE;
                }
            }
        } while (rl != 0);
    }

    fclose(fs);

    return EXIT_SUCCESS;
}

int ffs_getattr(const char *path, struct stat *statbuf) {
    // open file
    FILE *fs;
    if ((fs = fopen(FFS_DATA->source, "rb")) == NULL) {
        return -EXIT_FAILURE;
    }

    // find inode number
    uint64_t inum;
    if ((inum = path_to_inode(fs, path)) == EXIT_FAILURE) {
        fclose(fs);
        return -EXIT_FAILURE;
    }

    // read inode
    ffs_inode_t inode;
    if (read_inode(fs, inum, &inode) == EXIT_FAILURE) {
        fclose(fs);
        return -EXIT_FAILURE;
    }

    fclose(fs);

    memset(statbuf, 0, sizeof(struct stat));

    statbuf->st_mode = inode.i_mode;
    statbuf->st_ino = inum;
    statbuf->st_nlink = inode.i_links_count;
    statbuf->st_uid = inode.i_uid;
    statbuf->st_gid = inode.i_gid;
    statbuf->st_size = inode.i_size;
    statbuf->st_blksize = inode.i_blocks;
    return EXIT_SUCCESS;
}

int ffs_chmod(const char *path, mode_t mode) {
    FILE *fs;
    if ((fs = fopen(FFS_DATA->source, "r+b")) == NULL) {
        return -EXIT_FAILURE;
    }

    // find inode number
    uint64_t inum;
    if ((inum = path_to_inode(fs, path)) == EXIT_FAILURE) {
        fclose(fs);
        return -EXIT_FAILURE;
    }

    // read inode
    ffs_inode_t inode;
    if (read_inode(fs, inum, &inode) == EXIT_FAILURE) {
        fclose(fs);
        return -EXIT_FAILURE;
    }

    inode.i_mode = mode;

    if (write_inode(fs, inum, &inode) == EXIT_FAILURE) {
        fclose(fs);
        return -EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int ffs_chown(const char *path, uid_t uid, gid_t gid) {
    FILE *fs;
    if ((fs = fopen(FFS_DATA->source, "r+b")) == NULL) {
        return -EXIT_FAILURE;
    }

    // find inode number
    uint64_t inum;
    if ((inum = path_to_inode(fs, path)) == EXIT_FAILURE) {
        fclose(fs);
        return -EXIT_FAILURE;
    }

    // read inode
    ffs_inode_t inode;
    if (read_inode(fs, inum, &inode) == EXIT_FAILURE) {
        fclose(fs);
        return -EXIT_FAILURE;
    }

    inode.i_gid = gid;
    inode.i_uid = uid;

    if (write_inode(fs, inum, &inode) == EXIT_FAILURE) {
        fclose(fs);
        return -EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int ffs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    FILE *fs;
    if ((fs = fopen(FFS_DATA->source, "r")) == NULL) {
        return -EXIT_FAILURE;
    }

    // find inode number
    uint64_t inum;
    if ((inum = path_to_inode(fs, path)) == EXIT_FAILURE) {
        fclose(fs);
        return -EXIT_FAILURE;
    }

    // read inode
    ffs_inode_t inode;
    if (read_inode(fs, inum, &inode) == EXIT_FAILURE) {
        fclose(fs);
        return -EXIT_FAILURE;
    }

    // number of blocks used by inode
    uint64_t blocks = inode.i_size / sizeof(ffs_block_t);
    if (blocks > 12) {
        blocks = 12;
    }

    // allocate memory
    if ((buf = calloc(sizeof(char), size)) == NULL) {
        fclose(fs);
        return -EXIT_FAILURE;
    }

    for (size_t block = 0; block < blocks; ++block) {
        // move file pointer to a block
        if (fseek(fs, sizeof(ffs_block_t) * inode.i_block[block], SEEK_SET) == -1) {
            fclose(fs);
            free(buf);
            return -EXIT_FAILURE;
        }
        // read all chars from the block
        if (fread(buf + block * 2048, 1, 2048, fs) != 2048) {
            fclose(fs);
            free(buf);
            return -EXIT_FAILURE;
        }
    }

    fclose(fs);

    return EXIT_SUCCESS;
}

int ffs_access(const char *path, int mask) {
    return EXIT_SUCCESS;
}

int ffs_utimens(const char *path, const struct timespec tv[2]) {
    return EXIT_SUCCESS;
}

int ffs_getxattr(const char *path, const char *name, char *value, size_t size) {
    return -ENOTSUP;
}

void *ffs_init(struct fuse_conn_info *conn) {
    return FFS_DATA;
}

void ffs_destroy(void *userdata) {
}
