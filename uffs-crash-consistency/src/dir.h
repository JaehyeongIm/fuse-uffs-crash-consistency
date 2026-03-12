#pragma once

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <sys/types.h>

/* FR-DIR-001: 디렉토리 읽기 (ls) */
int dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                off_t offset, struct fuse_file_info *fi,
                enum fuse_readdir_flags flags);

/* FR-DIR-002: 디렉토리 생성 */
int dir_mkdir(const char *path, mode_t mode);

/* FR-DIR-003: 디렉토리 삭제 (비어 있어야 함) */
int dir_rmdir(const char *path);
