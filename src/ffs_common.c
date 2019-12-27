#include "ffs_common.h"

#include <ffs.h>
#include <fuse.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

ssize_t writebuff(int fd, void *buffer, size_t size) {
    size_t written_bytes = 0;
    errno = 0;
    while (written_bytes < size) {
        ssize_t just_written = write(fd, (uint8_t *) buffer + written_bytes, size - written_bytes);
        if (just_written == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            return -1;
        }
        written_bytes += just_written;
    }
    return written_bytes;
}

ssize_t readbuff(int fd, void *buffer, size_t size) {
    size_t read_bytes = 0;
    ssize_t just_read;
    errno = 0;
    do {
        just_read = read(fd, (uint8_t *) buffer + read_bytes, size - read_bytes);
        if (just_read < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            return just_read;
        }
        read_bytes += just_read;
    } while (read_bytes < size && just_read != 0);
    return read_bytes;
}

void bitmap_set_bit(uint8_t *bitmap, uint16_t bit, uint8_t val) {
    if (val == 0) {
        bitmap[bit / 8] &= ~(1 << (bit % 8));
    } else if (val == 1) {
        bitmap[bit / 8] |= 1 << (bit % 8);
    }
}

ssize_t read_inode(uint64_t inodei, ffs_inode_t *inode, char *fs) {
    int fd;
    if ((fd = open(fs, O_RDONLY)) < 0) {
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
        perror("read");
        close(fd);
        return EXIT_FAILURE;
    }

    inodei--;
    uint64_t gbn = inodei / sb.sb_inodes_per_group;
    uint64_t i_index = inodei % sb.sb_inodes_per_group;

    if (lseek(fd, FFS_BLOCKSIZE + gbn * sizeof(ffs_bgd_t), SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        return EXIT_FAILURE;
    }

    ffs_bgd_t bgd;
    if (readbuff(fd, &bgd, sizeof(bgd)) == -1) {
        perror("read");
        close(fd);
        return EXIT_FAILURE;
    }

    if (lseek(fd, FFS_BLOCKSIZE * bgd.bgd_inode_table + i_index * sizeof(ffs_inode_t), SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        return EXIT_FAILURE;
    }

    if (readbuff(fd, inode, sizeof(ffs_inode_t)) == -1) {
        perror("read");
        close(fd);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int64_t entry_inode_no(ffs_inode_t *inode, char *entry_name, char *fs) {
    uint64_t blocks = inode->i_size / FFS_BLOCKSIZE;
    if (blocks > 12) {
        blocks = 12;
    }

    ffs_de_t *des = (ffs_de_t *) calloc(sizeof(ffs_de_t), FFS_BLOCKSIZE / sizeof(ffs_de_t) * blocks);
    if (des == NULL) {
        perror("calloc");
        return EXIT_FAILURE;
    }

    int fd;
    if ((fd = open(fs, O_RDONLY)) < 0) {
        perror("open");
        free(des);
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < FFS_BLOCKSIZE / sizeof(ffs_de_t) * blocks; ++i) {
        if (lseek(fd, FFS_BLOCKSIZE * inode->i_block[i], SEEK_SET) == -1) {
            perror("lseek");
            close(fd);
            free(des);
            return EXIT_FAILURE;
        }
        if (readbuff(fd, des + i, sizeof(ffs_de_t)) == -1) {
            perror("read");
            close(fd);
            free(des);
            return EXIT_FAILURE;
        }
    }

    close(fd);

    for (uint64_t i = 0; i < FFS_BLOCKSIZE / sizeof(ffs_de_t) * blocks; ++i) {
        if (strncmp(des[i].de_name, entry_name, des[i].de_name_len) == 0) {
            //
            return des[i].de_inode;
        }
    }

    free(des);
    return EXIT_FAILURE;
}

int64_t path_to_inode(const char *path, char *fs) {
    if (strcmp(path, "/") == 0) {
        return 2;
    }

    uint64_t inodeno = 2;
    ffs_inode_t inode;

    size_t i = 1, start = 1;
    char c;
    while ((c = path[i]) != 0) {
        if ((c == '/') || (c == 0)) {
            if (read_inode(inodeno, &inode, fs) == EXIT_FAILURE) {
                return EXIT_FAILURE;
            }

            char *entry_name = (char *) malloc(i - start + 1);
            if (entry_name == NULL) {
                perror("malloc");
                return EXIT_FAILURE;
            }

            for (size_t j = 0; j < i - start + 1; ++j) {
                entry_name[j] = path[start + j];
            }
            entry_name[i - start + 1] = '\0';

            if ((inodeno = entry_inode_no(&inode, entry_name, fs)) == EXIT_FAILURE) {
                free(entry_name);
                return EXIT_FAILURE;
            }
            start = i + 1;
        }
        i++;
    }
    if (read_inode(inodeno, &inode, fs) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    char *entry_name = (char *) malloc(i - start + 1);
    if (entry_name == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    for (size_t j = 0; j < i - start + 1; ++j) {
        entry_name[j] = path[start + j];
    }
    entry_name[i - start + 1] = '\0';

    if ((inodeno = entry_inode_no(&inode, entry_name, fs)) == EXIT_FAILURE) {
        free(entry_name);
        return EXIT_FAILURE;
    }

    return inodeno;
}
