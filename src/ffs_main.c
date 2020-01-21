#include "ffs_common.h"
#include "ffs_fuse.h"

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct fuse_operations ffs_op = {
        .statfs     = ffs_statfs,
        .opendir    = ffs_opendir,
        .open       = ffs_open,
        .readdir    = ffs_readdir,
        .getattr    = ffs_getattr,
        .chmod      = ffs_chmod,
        .chown      = ffs_chown,
        .read       = ffs_read,
        .access     = ffs_access,
        .utimens    = ffs_utimens,
        .getxattr   = ffs_getxattr,
        .init       = ffs_init,
        .destroy    = ffs_destroy
};

int main(int argc, char *argv[], char *envp[]) {
    if ((argc < 3) || (argv[argc - 2][0] == '-') || (argv[argc - 1][0] == '-')) {
        fprintf(stderr, "usage:\tffs [FUSE and mount options] [source] [destination]\n");
        return EXIT_FAILURE;
    }

    struct ffs_init_data *ffs_data = (struct ffs_init_data *) malloc(sizeof(struct ffs_init_data));
    if (ffs_data == NULL) {
        return EXIT_FAILURE;
    }

    // remove mount source from options
    ffs_data->source = realpath(argv[argc - 2], NULL);
    argv[argc - 2] = argv[argc - 1];
    argv[argc - 1] = NULL;
    argc--;

    return fuse_main(argc, argv, &ffs_op, ffs_data);
}
