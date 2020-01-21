#include "ffs_common.h"
#include "ffs_mkfs.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[], char *envp[]) {
    printf("mkfs.ffs 1.0.0 (27-Dec-2019)\n\n");

    if (argc != 2) {
        fprintf(stderr, "Usage: mkfs.ffs [filename]\n");
        return EXIT_FAILURE;
    }

    struct stat stats;
    if (lstat(argv[1], &stats) == -1) {
        perror("stat");
        return EXIT_FAILURE;
    }

    if (!S_ISREG(stats.st_mode) && !S_ISBLK(stats.st_mode)) {
        fprintf(stderr, "File must be regular or block device\n");
        return EXIT_FAILURE;
    }

    if (stats.st_size < FFS_BLOCKS_PER_GROUP * FFS_BLOCKSIZE) {
        fprintf(stderr, "File must at least 4 megabytes in size\n");
        return EXIT_FAILURE;
    }

    int fd;
    if ((fd = open(argv[1], O_WRONLY)) < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    uint64_t bgn = stats.st_size / (FFS_BLOCKSIZE * FFS_BLOCKS_PER_GROUP);
    uint64_t bgdt_blocks = ((bgn % 64) == 0) ? bgn / 64 : (bgn / 64) + 1;

    printf("Creating filesystem with %d 2KB blocks and %d inodes\n\n", bgn * FFS_BLOCKS_PER_GROUP, bgn * FFS_INODES_PER_GROUP);

    ffs_write_superblock(fd, bgn, bgdt_blocks);
    ffs_write_bgd_table(fd, bgn, bgdt_blocks);
    ffs_write_block_groups(fd, bgn, bgdt_blocks);

    close(fd);

    return EXIT_SUCCESS;
}

void ffs_write_superblock(int fd, uint64_t bgn, uint64_t bgdt_blocks) {
    static ffs_sb_t sb;
    sb.sb_inodes_count = bgn * FFS_INODES_PER_GROUP;
    sb.sb_blocks_count = bgn * FFS_BLOCKS_PER_GROUP;
    sb.sb_free_blocks_count = bgn * (FFS_BLOCKS_PER_GROUP - 2 - FFS_INODE_TABLE_BLOCKS) - 2 - bgdt_blocks;
    sb.sb_free_inodes_count = sb.sb_inodes_count - FFS_RESERVED_INODES;
    sb.sb_log_block_size = FFS_LOG_BLOCK_SIZE;
    sb.sb_log_frag_size = FFS_LOG_BLOCK_SIZE;
    sb.sb_blocks_per_group = FFS_BLOCKS_PER_GROUP;
    sb.sb_frags_per_group = FFS_BLOCKS_PER_GROUP;
    sb.sb_inodes_per_group = FFS_INODES_PER_GROUP;
    sb.sb_max_mnt_count = 0xffff;
    sb.sb_magic = FFS_MAGIC;
    sb.sb_state = FFS_FILESYSTEM_STATE;
    sb.sb_errors = FFS_ERROR_HANDLER;
    sb.sb_minor_rev_level = 0;
    sb.sb_checkinterval = 0xffffffff;
    sb.sb_rev_level = 0;

    if (lseek(fd, 1024, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (writebuff(fd, &sb, sizeof(sb)) == -1) {
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Writing superblock and filesystem accounting information: done\n");
}

void ffs_write_bgd_table(int fd, uint64_t bgn, uint64_t bgdt_blocks) {
    if (lseek(fd, FFS_BLOCKSIZE, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        exit(EXIT_FAILURE);
    }

    for (uint64_t i = 0; i < bgn; ++i) {
        static ffs_bgd_t bgd;
        bgd.bgd_block_bitmap = i == 0 ? 1 + bgdt_blocks : i * FFS_BLOCKS_PER_GROUP;
        bgd.bgd_inode_bitmap = bgd.bgd_block_bitmap + 1;
        bgd.bgd_inode_table = bgd.bgd_inode_bitmap + 1;
        bgd.bgd_free_blocks_count = i == 0 ? FFS_BLOCKS_PER_GROUP - 4 - FFS_INODE_TABLE_BLOCKS - bgdt_blocks : FFS_BLOCKS_PER_GROUP - 2 - FFS_INODE_TABLE_BLOCKS;
        bgd.bgd_free_inodes_count = i == 0 ? FFS_INODES_PER_GROUP - FFS_RESERVED_INODES : FFS_INODES_PER_GROUP;
        bgd.bgd_used_dirs_count = i == 0 ? 1 : 0;

        if (writebuff(fd, &bgd, sizeof(bgd)) == -1) {
            perror("write");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }

    printf("Writing block group descriptors table: done\n");
}

void ffs_write_block_groups(int fd, uint64_t bgn, uint64_t bgdt_blocks) {
    if (lseek(fd, FFS_BLOCKSIZE * (bgdt_blocks + 1), SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        exit(EXIT_FAILURE);
    }

    static ffs_inode_t root_inode;
    root_inode.i_mode = 0x41ed;
    root_inode.i_size = FFS_BLOCKSIZE;
    root_inode.i_links_count = 2;
    root_inode.i_blocks = FFS_BLOCKSIZE / 512;
    root_inode.i_block[0] = 3 + bgdt_blocks + FFS_INODE_TABLE_BLOCKS;

    for (uint64_t i = 0; i < bgn; ++i) {
        static ffs_bg_t bg;
        for (uint8_t j = 0; j < 2 + FFS_INODE_TABLE_BLOCKS; ++j) {
            bitmap_set_bit(bg.bg_block_bitmap, j, 1);
        }

        if (i == 0) {
            for (uint64_t j = 0; j < bgdt_blocks + 2; ++j) {
                bitmap_set_bit(bg.bg_block_bitmap, 2 + FFS_INODE_TABLE_BLOCKS + j, 1);
            }
            for (uint8_t j = 0; j < FFS_RESERVED_INODES; ++j) {
                bitmap_set_bit(bg.bg_inode_bitmap, j, 1);
            }
            bg.bg_inode_table[1] = root_inode;
        }

        if (writebuff(fd, &bg, sizeof(bg)) == -1) {
            perror("write");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }

    ffs_de_t root_de[2];
    root_de[0].de_inode = 2;
    root_de[0].de_rec_len = FFS_DIR_ENTRY_RECORD_LENGTH;
    root_de[0].de_name_len = 1;
    root_de[0].de_name[0] = '.';
    root_de[0].de_name[1] = 0;

    root_de[1].de_inode = 2;
    root_de[1].de_rec_len = FFS_DIR_ENTRY_RECORD_LENGTH;
    root_de[1].de_name_len = 2;
    root_de[1].de_name[0] = '.';
    root_de[1].de_name[1] = '.';
    root_de[1].de_name[2] = 0;

    if (lseek(fd, FFS_BLOCKSIZE * root_inode.i_block[0], SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (writebuff(fd, &root_de, sizeof(root_de)) == -1) {
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Writing block groups: done\n\n");
}
