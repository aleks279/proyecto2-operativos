#ifndef _PARAMS_H_
#define _PARAMS_H_

#define FUSE_USE_VERSION 26

#define _XOPEN_SOURCE 500

#include <limits.h>
#include <stdio.h>
struct hrfs_state {
    FILE *logfile;
    char *rootdir;
};
#define HRFS_DATA ((struct hrfs_state *) fuse_get_context()->private_data)

#endif
