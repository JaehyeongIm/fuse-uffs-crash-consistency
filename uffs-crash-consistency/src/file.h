#pragma once

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <sys/types.h>

/* FR-FILE-001: 파일 열기 */
int file_open(const char *path, struct fuse_file_info *fi);

/* FR-FILE-002: 파일 생성 */
int file_create(const char *path, mode_t mode, struct fuse_file_info *fi);

/* FR-FILE-003: 파일 읽기 */
int file_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi);

/* FR-FILE-004: 파일 쓰기 */
int file_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi);

/* FR-FILE-005: 파일 fsync */
int file_fsync(const char *path, int isdatasync, struct fuse_file_info *fi);

/* FR-FILE-006: 파일 rename */
int file_rename(const char *from, const char *to, unsigned int flags);

/* FR-FILE-007: 파일 삭제 */
int file_unlink(const char *path);
