#include "ffs_common.h"

#include <fuse.h>
#include <errno.h>
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

void ffs_resolve_path(char fpath[PATH_MAX], const char *path) {
    strcpy(fpath, FFS_DATA->rootdir);
    strncat(fpath, path, PATH_MAX);
}
