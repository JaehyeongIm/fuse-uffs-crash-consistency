#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>

#include "flash.h"
#include "tree.h"
#include "meta.h"
#include "file.h"
#include "dir.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ─── 전역 락 ────────────────────────────────────────────── */
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

#define LOCK()   pthread_mutex_lock(&g_lock)
#define UNLOCK() pthread_mutex_unlock(&g_lock)

/* ─── FUSE 콜백 래퍼 (락 + 위임) ────────────────────────── */

static void *uffs_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    (void)conn;
    cfg->use_ino     = 0;
    cfg->entry_timeout  = 0;
    cfg->attr_timeout   = 0;
    cfg->negative_timeout = 0;

    const char *dev = (const char *)fuse_get_context()->private_data;

    if (flash_init(dev) != 0) {
        fprintf(stderr, "uffs: flash_init failed\n");
        return NULL;
    }

    if (!flash_format_check()) {
        if (flash_format() != 0) {
            fprintf(stderr, "uffs: flash_format failed\n");
            return NULL;
        }
    }

    if (tree_build() != 0) {
        fprintf(stderr, "uffs: tree_build failed\n");
        return NULL;
    }

    return NULL;
}

static int uffs_getattr(const char *path, struct stat *stbuf,
                         struct fuse_file_info *fi)
{
    LOCK();
    int rc = meta_getattr(path, stbuf, fi);
    UNLOCK();
    return rc;
}

static int uffs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags)
{
    LOCK();
    int rc = dir_readdir(path, buf, filler, offset, fi, flags);
    UNLOCK();
    return rc;
}

static int uffs_mkdir(const char *path, mode_t mode)
{
    LOCK();
    int rc = dir_mkdir(path, mode);
    UNLOCK();
    return rc;
}

static int uffs_rmdir(const char *path)
{
    LOCK();
    int rc = dir_rmdir(path);
    UNLOCK();
    return rc;
}

static int uffs_create(const char *path, mode_t mode,
                        struct fuse_file_info *fi)
{
    LOCK();
    int rc = file_create(path, mode, fi);
    UNLOCK();
    return rc;
}

static int uffs_open(const char *path, struct fuse_file_info *fi)
{
    LOCK();
    int rc = file_open(path, fi);
    UNLOCK();
    return rc;
}

static int uffs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    LOCK();
    int rc = file_read(path, buf, size, offset, fi);
    UNLOCK();
    return rc;
}

static int uffs_write(const char *path, const char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi)
{
    LOCK();
    int rc = file_write(path, buf, size, offset, fi);
    UNLOCK();
    return rc;
}

static int uffs_fsync(const char *path, int isdatasync,
                       struct fuse_file_info *fi)
{
    LOCK();
    int rc;
    if (tree_find_dir(path)) {
        /* 디렉토리 fsync (FR-DIR-001): 펜딩 플래시 쓰기 전체 동기화 */
        rc = flash_sync();
    } else {
        rc = file_fsync(path, isdatasync, fi);
    }
    UNLOCK();
    return rc;
}

static int uffs_rename(const char *from, const char *to, unsigned int flags)
{
    LOCK();
    int rc = file_rename(from, to, flags);
    UNLOCK();
    return rc;
}

static int uffs_unlink(const char *path)
{
    LOCK();
    int rc = file_unlink(path);
    UNLOCK();
    return rc;
}

/* ─── FUSE 연산 테이블 ───────────────────────────────────── */

static const struct fuse_operations uffs_ops = {
    .init    = uffs_init,
    .getattr = uffs_getattr,
    .readdir = uffs_readdir,
    .mkdir   = uffs_mkdir,
    .rmdir   = uffs_rmdir,
    .create  = uffs_create,
    .open    = uffs_open,
    .read    = uffs_read,
    .write   = uffs_write,
    .fsync   = uffs_fsync,
    .rename  = uffs_rename,
    .unlink  = uffs_unlink,
};

/* ─── main ───────────────────────────────────────────────── */
/*
 * 사용법: uffs <device_path> <mountpoint> [FUSE 옵션...]
 * device_path 를 fuse_main private_data 로 전달한다.
 */
int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "사용법: %s <device_path> <mountpoint> [FUSE 옵션...]\n",
                argv[0]);
        return 1;
    }

    const char *device_path = argv[1];

    /* argv 재구성: device_path 제거, mountpoint 와 FUSE 옵션만 전달 */
    int   new_argc = argc - 1;
    char **new_argv = malloc((size_t)(new_argc + 1) * sizeof(char *));
    if (!new_argv) return 1;

    new_argv[0] = argv[0];
    for (int i = 1; i < new_argc; i++)
        new_argv[i] = argv[i + 1];
    new_argv[new_argc] = NULL;

    int rc = fuse_main(new_argc, new_argv, &uffs_ops, (void *)device_path);
    free(new_argv);
    return rc;
}
