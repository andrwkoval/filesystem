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

void bitmap_set_bit(uint8_t *bitmap, uint16_t bit, uint8_t val) {
    if (val == 0) {
        bitmap[bit / 8] &= ~(1 << (bit % 8));
    } else if (val == 1) {
        bitmap[bit / 8] |= 1 << (bit % 8);
    }
}

uint8_t read_superblock(FILE *fs, ffs_sb_t *sb) {
    // skip MBR
    if (fseek(fs, 1024, SEEK_SET) == -1) {
        return EXIT_FAILURE;
    }

    // read superblock
    if (fread(sb, sizeof(ffs_sb_t), 1, fs) != 1) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

uint8_t read_inode(FILE *fs, uint64_t inodei, ffs_inode_t *inode) {
    // read superblock
    ffs_sb_t sb;
    if (read_superblock(fs, &sb) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    // decrease inode index, because they are counted from 1
    inodei--;
    // block group number
    uint64_t gbn = inodei / sb.sb_inodes_per_group;
    // inode index in its table
    uint64_t i_index = inodei % sb.sb_inodes_per_group;

    // move file pointer to block group descriptor
    if (fseek(fs, sizeof(ffs_block_t) + gbn * sizeof(ffs_bgd_t), SEEK_SET) == -1) {
        return EXIT_FAILURE;
    }

    // read block group descriptor
    ffs_bgd_t bgd;
    if (fread(&bgd, sizeof(ffs_bgd_t), 1, fs) != 1) {
        return EXIT_FAILURE;
    }

    // move file pointer to inode
    if (fseek(fs, sizeof(ffs_block_t) * bgd.bgd_inode_table + i_index * sizeof(ffs_inode_t), SEEK_SET) == -1) {
        return EXIT_FAILURE;
    }

    // read inode
    if (fread(inode, sizeof(ffs_inode_t), 1, fs) != 1) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int64_t entry_inode_no(FILE *fs, ffs_inode_t *inode, char *entry_name) {
    // number of blocks used by inode
    uint64_t blocks = inode->i_size / sizeof(ffs_block_t);
    if (blocks > 12) {
        blocks = 12;
    }

    // for each block
    for (size_t block = 0; block < blocks; ++block) {
        // move file pointer to record length of the first directory entry on the block
        if (fseek(fs, sizeof(ffs_block_t) * inode->i_block[block] + 4, SEEK_SET) == -1) {
            return EXIT_FAILURE;
        }

        uint16_t rl;
        do {
            // read entry length
            if (fread(&rl, 2, 1, fs) != 1) {
                return EXIT_FAILURE;
            }
            // if entry exists
            if (rl != 0) {
                uint16_t namelen;
                // read entry name length
                if (fread(&namelen, 2, 1, fs) != 1) {
                    return EXIT_FAILURE;
                }
                // allocate memory for the name
                char name[248];
                // read the name
                if (fread(name, namelen, 1, fs) != 1) {
                    return EXIT_FAILURE;
                }
                name[namelen] = '\0';
                // compare found entry with needed
                if (strcmp(name, entry_name) == 0) {
                    // go back to entry inode index
                    if (fseek(fs, -namelen - 8, SEEK_CUR) == -1) {
                        return EXIT_FAILURE;
                    }
                    // read entry inode index
                    uint32_t inode_no;
                    if (fread(&inode_no, 4, 1, fs) != 1) {
                        return EXIT_FAILURE;
                    }
                    return inode_no;
                }
                // move file pointer to next entry's record length
                if (fseek(fs, rl - namelen - 4, SEEK_CUR) == -1) {
                    return EXIT_FAILURE;
                }
            }
        } while (rl != 0);
    }

    return EXIT_FAILURE;
}

int64_t path_to_inode(FILE *fs, const char *path) {
    // inode number of root directory has fixed value of 2
    if (strcmp(path, "/") == 0) {
        return 2;
    }

    // start from root directory
    uint64_t inodeno = 2;
    ffs_inode_t inode;

    size_t i = 1, start = 1;
    char c = path[i];
    do {
        // path node
        if ((c == '/') || (c == 0)) {
            // read inode by its index
            if (read_inode(fs, inodeno, &inode) == EXIT_FAILURE) {
                return EXIT_FAILURE;
            }

            // allocate memory for the entry name
            char entry_name[248];

            // copy entry name
            memcpy(entry_name, path + start, i - start);
            entry_name[i - start + 1] = '\0';

            // get inode index by its name
            if ((inodeno = entry_inode_no(fs, &inode, entry_name)) == EXIT_FAILURE) {
                return EXIT_FAILURE;
            }
            start = i + 1;
        }
        i++;
    } while ((c = path[i]) != 0);

    return inodeno;
}

uint8_t write_inode(FILE *fs, uint64_t inodei, ffs_inode_t *inode) {
    // read superblock
    ffs_sb_t sb;
    if (read_superblock(fs, &sb) == EXIT_FAILURE) {
        fclose(fs);
        return EXIT_FAILURE;
    }

    // decrease inode index, because they are counted from 1
    inodei--;
    // block group number
    uint64_t gbn = inodei / sb.sb_inodes_per_group;
    // inode index in its table
    uint64_t i_index = inodei % sb.sb_inodes_per_group;

    // move file pointer to block group descriptor
    if (fseek(fs, sizeof(ffs_block_t) + gbn * sizeof(ffs_bgd_t), SEEK_SET) == -1) {
        return EXIT_FAILURE;
    }

    // read block group descriptor
    ffs_bgd_t bgd;
    if (fread(&bgd, sizeof(ffs_bgd_t), 1, fs) != 1) {
        return EXIT_FAILURE;
    }

    // move file pointer to inode
    if (fseek(fs, sizeof(ffs_block_t) * bgd.bgd_inode_table + i_index * sizeof(ffs_inode_t), SEEK_SET) == -1) {
        return EXIT_FAILURE;
    }

    // write inode
    if (fwrite(inode, sizeof(ffs_inode_t), 1, fs) != 1) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
