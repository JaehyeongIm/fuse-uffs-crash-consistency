#include "dir.h"
#include "tree.h"
#include "flash.h"
#include "gc.h"
#include "utils.h"

#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>

/* ─── dir_readdir (FR-DIR-001) ──────────────────────────── */

int dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                off_t offset, struct fuse_file_info *fi,
                enum fuse_readdir_flags flags)
{
    (void)offset;
    (void)fi;
    (void)flags;

    TreeNode *dir = tree_find_dir(path);
    if (!dir) return -ENOENT;

    filler(buf, ".",  NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    /* 자식 노드 열거 */
    TreeNode *children[MAX_DIR_COUNT + MAX_FILE_COUNT];
    int       cnt = 0;
    tree_enumerate_children(dir->serial, children, &cnt,
                             MAX_DIR_COUNT + MAX_FILE_COUNT);

    for (int i = 0; i < cnt; i++)
        filler(buf, children[i]->name, NULL, 0, 0);

    return 0;
}

/* ─── dir_mkdir (FR-DIR-002) ────────────────────────────── */

int dir_mkdir(const char *path, mode_t mode)
{
    (void)mode;

    if (tree_find_dir(path))  return -EEXIST;
    if (tree_find_file(path)) return -EEXIST;

    char parent_path[512], child_name[512];
    utils_path_split(path, parent_path, child_name);

    TreeNode *parent = tree_find_dir(parent_path);
    if (!parent) return -ENOENT;

    u16 new_serial = tree_alloc_serial();
    if (new_serial == 0) return -ENOSPC;

    /* 블록 할당; 빈 블록 없으면 GC 후 재시도 */
    int blk = flash_alloc_block();
    if (blk < 0) {
        if (gc_collect() != 0) return -ENOSPC;
        blk = flash_alloc_block();
        if (blk < 0) return -ENOSPC;
    }

    u32 now = (u32)time(NULL);

    uffs_FileInfo fi_data;
    memset(&fi_data, 0, sizeof(fi_data));
    fi_data.attr        = FILE_ATTR_DIR;
    fi_data.create_time = now;
    fi_data.last_modify = now;
    fi_data.access      = now;
    fi_data.size        = 0;
    fi_data.name_len    = (u32)strlen(child_name);
    strncpy(fi_data.name, child_name, MAX_FILENAME_LEN - 1);

    uffs_Tag tag;
    memset(&tag, 0, sizeof(tag));
    tag.s.type      = UFFS_TYPE_DIR;
    tag.s.serial    = new_serial;
    tag.s.parent    = parent->serial;
    tag.s.page_id   = 0;
    tag.s.data_len  = (u16)sizeof(uffs_FileInfo);
    tag.s.dirty     = 1;
    tag.s.valid     = 0;
    tag.s.block_ts  = 0;

    if (flash_write_page_unsealed(blk, 0, &fi_data, sizeof(fi_data), &tag) != 0)
        return -EIO;
    if (flash_seal_page(blk, 0) != 0)
        return -EIO;

    if (tree_insert_dir(new_serial, parent->serial, blk, child_name) != 0)
        return -ENOSPC;

    TreeNode *n = tree_find_dir_by_serial(new_serial);
    if (n) {
        n->create_time = now;
        n->last_modify = now;
    }
    return 0;
}

/* ─── dir_rmdir (FR-DIR-003) ────────────────────────────── */
/*
 * 디렉토리가 비어 있어야 한다.
 * 헤더 페이지(logical 0)를 dirty 1→0 flip으로 무효화한 후
 * 인메모리 트리에서 제거한다.
 */
int dir_rmdir(const char *path)
{
    if (strcmp(path, "/") == 0) return -EBUSY;

    TreeNode *dir = tree_find_dir(path);
    if (!dir) return -ENOENT;

    /* 자식 존재 여부 확인 */
    TreeNode *children[1];
    int       cnt = 0;
    tree_enumerate_children(dir->serial, children, &cnt, 1);
    if (cnt > 0) return -ENOTEMPTY;

    /* 블록 내 모든 유효 페이지를 dirty 1→0 flip */
    for (int p = 0; p < PAGES_PER_BLOCK; p++) {
        uffs_MiniHeader hdr;
        uffs_Tag        tag;
        if (flash_read_page(dir->block_id, p, &hdr, NULL, &tag) != 0) continue;
        if (hdr.status == 0xFF) break;
        if ((tag.seal_byte == SEAL_DONE || tag.seal_byte == SEAL_WRITING) &&
            tag.s.dirty == 1)
            flash_obsolete_page(dir->block_id, p);
    }

    tree_remove_dir(dir->serial);
    return 0;
}
