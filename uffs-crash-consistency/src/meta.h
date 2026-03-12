#pragma once

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <sys/stat.h>

/* FR-META-001: stat(path) → struct stat 채우기 */
int meta_getattr(const char *path, struct stat *stbuf,
                 struct fuse_file_info *fi);
