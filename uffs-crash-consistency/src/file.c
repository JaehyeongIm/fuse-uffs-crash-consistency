#include "file.h"
#include "tree.h"
#include "flash.h"
#include "gc.h"
#include "utils.h"

#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

/* ─── fi->fh 에 TreeNode* 저장/복원 ─────────────────────── */
#define FH_SET(fi, ptr)  ((fi)->fh = (uint64_t)(uintptr_t)(ptr))
#define FH_GET(fi)       ((TreeNode *)(uintptr_t)((fi)->fh))

/* ─── 내부 헬퍼: 물리 페이지 탐색 ──────────────────────────
 *
 * CoW로 인해 한 블록 내 동일 logical page_id를 가진 물리 페이지가
 * 여러 개 존재할 수 있다. 높은 인덱스일수록 최신 버전이다.
 *
 * find_latest_valid_phys : SEAL_DONE && dirty==1 (영구 유효)
 * find_latest_active_phys: (SEAL_DONE || SEAL_WRITING) && dirty==1
 *                           (SEAL_WRITING 포함, 아직 씰 안 된 페이지도 허용)
 */
static int find_latest_valid_phys(int blk, int logical_pg)
{
    for (int p = PAGES_PER_BLOCK - 1; p >= 0; p--) {
        uffs_MiniHeader hdr;
        uffs_Tag        tag;
        if (flash_read_page(blk, p, &hdr, NULL, &tag) != 0) continue;
        if (hdr.status == 0xFF) continue;
        if (tag.seal_byte == SEAL_DONE && tag.s.dirty == 1 &&
            (int)tag.s.page_id == logical_pg)
            return p;
    }
    return -1;
}

static int find_latest_active_phys(int blk, int logical_pg)
{
    for (int p = PAGES_PER_BLOCK - 1; p >= 0; p--) {
        uffs_MiniHeader hdr;
        uffs_Tag        tag;
        if (flash_read_page(blk, p, &hdr, NULL, &tag) != 0) continue;
        if (hdr.status == 0xFF) continue;
        if ((tag.seal_byte == SEAL_DONE || tag.seal_byte == SEAL_WRITING) &&
            tag.s.dirty == 1 &&
            (int)tag.s.page_id == logical_pg)
            return p;
    }
    return -1;
}

/* 블록의 현재 block_ts 반환 (첫 번째 유효 씰 페이지 기준) */
static u8 get_block_ts(int blk)
{
    for (int p = 0; p < PAGES_PER_BLOCK; p++) {
        uffs_MiniHeader hdr;
        uffs_Tag        tag;
        if (flash_read_page(blk, p, &hdr, NULL, &tag) != 0) continue;
        if (hdr.status == 0xFF) break;
        if (tag.seal_byte == SEAL_DONE && tag.s.dirty == 1)
            return (u8)tag.s.block_ts;
    }
    return 0;
}

/* ─── 태그 초기화 헬퍼 ──────────────────────────────────── */

static void make_file_tag(uffs_Tag *tag, u16 serial, u16 parent,
                           int page_id, u16 data_len, u8 block_ts)
{
    memset(tag, 0, sizeof(*tag));
    tag->s.type      = UFFS_TYPE_FILE;
    tag->s.serial    = serial;
    tag->s.parent    = parent;
    tag->s.page_id   = (u32)page_id;
    tag->s.data_len  = data_len;
    tag->s.dirty     = 1;
    tag->s.valid     = 0;
    tag->s.block_ts  = block_ts;
}

static void make_data_tag(uffs_Tag *tag, u16 file_serial, u16 data_idx,
                           int page_id, u16 data_len, u8 block_ts)
{
    memset(tag, 0, sizeof(*tag));
    tag->s.type      = UFFS_TYPE_DATA;
    tag->s.serial    = data_idx;    /* DATA 블록 인덱스 */
    tag->s.parent    = file_serial; /* 소유 파일 시리얼 */
    tag->s.page_id   = (u32)page_id;
    tag->s.data_len  = data_len;
    tag->s.dirty     = 1;
    tag->s.valid     = 0;
    tag->s.block_ts  = block_ts;
}

/* ─── FileInfo 채우기 ───────────────────────────────────── */

static void fill_file_info(uffs_FileInfo *fi, TreeNode *node, u32 now)
{
    memset(fi, 0, sizeof(*fi));
    fi->attr        = FILE_ATTR_WRITE;
    fi->create_time = node->create_time;
    fi->last_modify = now;
    fi->access      = now;
    fi->size        = node->size;
    fi->name_len    = (u32)strlen(node->name);
    strncpy(fi->name, node->name, MAX_FILENAME_LEN - 1);
}

/* ─── locate_page: 오프셋 → (블록, 논리 페이지, 페이지 내 위치) ──
 *
 * FILE 헤더 블록: logical page 1..31 = 데이터 (최대 15,872B)
 * DATA 블록     : logical page 0..31 = 데이터 (최대 16,384B/블록)
 *
 * *pg_out 은 논리(logical) 페이지 인덱스를 반환한다.
 * CoW로 인해 실제 물리 페이지는 find_latest_*_phys로 별도 탐색한다.
 *
 * alloc_new: 1이면 DATA 블록이 없을 때 신규 할당 (GC 포함)
 */
static int locate_page(TreeNode *node, off_t offset,
                        int *blk_out, int *pg_out,
                        int *off_in_pg_out, int *avail_out,
                        int alloc_new)
{
    if (offset < (off_t)MAX_DATA_IN_HEADER) {
        /* FILE 헤더 블록의 데이터 페이지 (logical 1 이상) */
        *blk_out       = node->block_id;
        *pg_out        = 1 + (int)(offset / PAGE_DATA_SIZE);
        *off_in_pg_out = (int)(offset % PAGE_DATA_SIZE);
        *avail_out     = PAGE_DATA_SIZE - *off_in_pg_out;
        return 0;
    }

    /* DATA 블록 영역 */
    off_t data_off    = offset - (off_t)MAX_DATA_IN_HEADER;
    u16   data_idx    = (u16)(data_off / MAX_DATA_PER_DATABLOCK);
    int   off_in_dblk = (int)(data_off % MAX_DATA_PER_DATABLOCK);

    *pg_out        = off_in_dblk / PAGE_DATA_SIZE;
    *off_in_pg_out = off_in_dblk % PAGE_DATA_SIZE;
    *avail_out     = PAGE_DATA_SIZE - *off_in_pg_out;

    DataBlockEntry *de = tree_find_data_block(node->serial, data_idx);
    if (de) {
        *blk_out = de->block_id;
        return 0;
    }

    if (!alloc_new) return -ENOENT;

    /* 새 DATA 블록 할당 (빈 블록 없으면 GC 후 재시도) */
    int new_blk = flash_alloc_block();
    if (new_blk < 0) {
        if (gc_collect() != 0) return -ENOSPC;
        new_blk = flash_alloc_block();
        if (new_blk < 0) return -ENOSPC;
    }

    if (tree_insert_data_block(node->serial, data_idx, new_blk) != 0)
        return -ENOSPC;
    *blk_out = new_blk;
    return 0;
}

/* ─── file_open (FR-FILE-001) ───────────────────────────── */

int file_open(const char *path, struct fuse_file_info *fi)
{
    TreeNode *node = tree_find_file(path);
    if (!node) return -ENOENT;
    FH_SET(fi, node);
    return 0;
}

/* ─── file_create (FR-FILE-002) ─────────────────────────── */

int file_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    (void)mode;

    if (tree_find_file(path)) return -EEXIST;
    if (tree_find_dir(path))  return -EEXIST;

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
    fi_data.attr        = FILE_ATTR_WRITE;
    fi_data.create_time = now;
    fi_data.last_modify = now;
    fi_data.access      = now;
    fi_data.size        = 0;
    fi_data.name_len    = (u32)strlen(child_name);
    strncpy(fi_data.name, child_name, MAX_FILENAME_LEN - 1);

    uffs_Tag tag;
    make_file_tag(&tag, new_serial, parent->serial, 0,
                  (u16)sizeof(uffs_FileInfo), 0 /* block_ts=0 */);

    if (flash_write_page_unsealed(blk, 0, &fi_data, sizeof(fi_data), &tag) != 0)
        return -EIO;
    if (flash_seal_page(blk, 0) != 0)
        return -EIO;

    if (tree_insert_file(new_serial, parent->serial, blk, child_name) != 0)
        return -ENOSPC;

    TreeNode *n = tree_find_file_by_serial(new_serial);
    if (n) {
        n->create_time = now;
        n->last_modify = now;
        n->size        = 0;
        n->is_dirty    = 0;
        FH_SET(fi, n);
    }
    return 0;
}

/* ─── file_read (FR-FILE-003) ───────────────────────────── */

int file_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi)
{
    TreeNode *node = (fi && fi->fh) ? FH_GET(fi) : tree_find_file(path);
    if (!node) return -ENOENT;

    if (offset >= (off_t)node->size) return 0;

    size_t to_read = size;
    if (offset + (off_t)to_read > (off_t)node->size)
        to_read = (size_t)((off_t)node->size - offset);

    size_t bytes_done = 0;
    while (bytes_done < to_read) {
        int blk, logical_pg, off_in_pg, avail;
        if (locate_page(node, offset + (off_t)bytes_done,
                        &blk, &logical_pg, &off_in_pg, &avail, 0) != 0)
            break;

        /* CoW: 이 논리 페이지의 최신 유효(씰된) 물리 페이지 탐색 */
        int phys = find_latest_valid_phys(blk, logical_pg);
        if (phys < 0) {
            /* SEAL_WRITING 포함 (미씰 페이지까지 허용) */
            phys = find_latest_active_phys(blk, logical_pg);
            if (phys < 0) break; /* 해당 논리 페이지 미기록 */
        }

        u8 page_data[PAGE_DATA_SIZE];
        if (flash_read_page(blk, phys, NULL, page_data, NULL) != 0) break;

        size_t chunk = (size_t)avail;
        if (chunk > to_read - bytes_done) chunk = to_read - bytes_done;

        memcpy(buf + bytes_done, page_data + off_in_pg, chunk);
        bytes_done += chunk;
    }
    return (int)bytes_done;
}

/* ─── file_write (FR-FILE-004) ──────────────────────────── */
/*
 * CoW 쓰기 절차 (논리 페이지 단위):
 *   1. 논리 페이지의 기존 물리 페이지 탐색 (RMW)
 *   2. 새 물리 슬롯 할당 (블록 가득 참 시 gc_collect_block → 재시도)
 *   3. 기존 데이터 읽기 + 새 데이터 패치
 *   4. 새 물리 페이지 쓰기 + 씰
 *   5. 기존 물리 페이지 무효화 (dirty 1→0)
 */
int file_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi)
{
    TreeNode *node = (fi && fi->fh) ? FH_GET(fi) : tree_find_file(path);
    if (!node) return -ENOENT;

    size_t bytes_done = 0;
    while (bytes_done < size) {
        off_t cur_off = offset + (off_t)bytes_done;
        int blk, logical_pg, off_in_pg, avail;

        if (locate_page(node, cur_off, &blk, &logical_pg,
                        &off_in_pg, &avail, 1) != 0)
            return (bytes_done > 0) ? (int)bytes_done : -ENOSPC;

        /* 기존 물리 페이지 탐색 (RMW) */
        int old_phys = find_latest_valid_phys(blk, logical_pg);
        /* 새 물리 슬롯 할당 */
        int new_phys = flash_alloc_page(blk);

        if (new_phys < 0) {
            /* 블록 가득 참 → GC 후 재시도 */
            if (gc_collect_block(blk) != 0)
                return (bytes_done > 0) ? (int)bytes_done : -ENOSPC;
            /* GC 후 blk 변경 가능 (tree_update_block_id 반영) */
            if (locate_page(node, cur_off, &blk, &logical_pg,
                            &off_in_pg, &avail, 0) != 0)
                return (bytes_done > 0) ? (int)bytes_done : -EIO;
            old_phys = find_latest_valid_phys(blk, logical_pg);
            new_phys = flash_alloc_page(blk);
            if (new_phys < 0)
                return (bytes_done > 0) ? (int)bytes_done : -ENOSPC;
        }

        /* RMW: 기존 페이지 데이터 읽기 (없으면 0으로 초기화) */
        u8 page_data[PAGE_DATA_SIZE];
        if (old_phys >= 0) {
            if (flash_read_page(blk, old_phys, NULL, page_data, NULL) != 0)
                return (bytes_done > 0) ? (int)bytes_done : -EIO;
        } else {
            memset(page_data, 0, PAGE_DATA_SIZE);
        }

        /* 새 데이터 패치 */
        size_t chunk = (size_t)avail;
        if (chunk > size - bytes_done) chunk = size - bytes_done;
        memcpy(page_data + off_in_pg, buf + bytes_done, chunk);

        /* 태그 구성 */
        u8       block_ts = get_block_ts(blk);
        uffs_Tag tag;
        if (cur_off < (off_t)MAX_DATA_IN_HEADER) {
            make_file_tag(&tag, node->serial, node->parent,
                          logical_pg, (u16)PAGE_DATA_SIZE, block_ts);
        } else {
            off_t data_off = cur_off - (off_t)MAX_DATA_IN_HEADER;
            u16   data_idx = (u16)(data_off / MAX_DATA_PER_DATABLOCK);
            make_data_tag(&tag, node->serial, data_idx,
                          logical_pg, (u16)PAGE_DATA_SIZE, block_ts);
        }

        /* 새 물리 페이지 쓰기 + 씰 */
        if (flash_write_page_unsealed(blk, new_phys, page_data,
                                       PAGE_DATA_SIZE, &tag) != 0)
            return (bytes_done > 0) ? (int)bytes_done : -EIO;
        if (flash_seal_page(blk, new_phys) != 0)
            return (bytes_done > 0) ? (int)bytes_done : -EIO;

        /* 기존 물리 페이지 무효화 (dirty 1→0) */
        if (old_phys >= 0)
            flash_obsolete_page(blk, old_phys);

        bytes_done += chunk;
    }

    /* 인메모리 파일 크기·타임스탬프 갱신 */
    off_t new_end = offset + (off_t)size;
    if (new_end > (off_t)node->size) node->size = (u32)new_end;
    node->is_dirty    = 1;
    node->last_modify = (u32)time(NULL);

    return (int)size;
}

/* ─── file_fsync (FR-FILE-005, SDD §5.3) ────────────────── */
/*
 * 내구성 보장 절차:
 *   Phase 1: FILE·DATA 블록의 남은 SEAL_WRITING 페이지를 씰
 *   Phase 2: CoW 헤더 (logical page 0) → 파일 크기·타임스탬프 포함
 *   Phase 3: flash_sync() (fdatasync)
 */
int file_fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
    (void)isdatasync;

    TreeNode *node = (fi && fi->fh) ? FH_GET(fi) : tree_find_file(path);
    if (!node) return -ENOENT;

    /* Phase 1-a: FILE 블록의 SEAL_WRITING 페이지 씰 */
    for (int p = 0; p < PAGES_PER_BLOCK; p++) {
        uffs_MiniHeader hdr;
        uffs_Tag        tag;
        if (flash_read_page(node->block_id, p, &hdr, NULL, &tag) != 0) continue;
        if (hdr.status == 0xFF) break;
        if (tag.seal_byte == SEAL_WRITING && tag.s.dirty == 1) {
            if (flash_seal_page(node->block_id, p) != 0) return -EIO;
        }
    }

    /* Phase 1-b: DATA 블록의 SEAL_WRITING 페이지 씰 */
    for (int i = 0; i < g_tree.data_count; i++) {
        if (g_tree.data_table[i].file_serial != node->serial) continue;
        int dblk = g_tree.data_table[i].block_id;
        for (int p = 0; p < PAGES_PER_BLOCK; p++) {
            uffs_MiniHeader hdr;
            uffs_Tag        tag;
            if (flash_read_page(dblk, p, &hdr, NULL, &tag) != 0) continue;
            if (hdr.status == 0xFF) break;
            if (tag.seal_byte == SEAL_WRITING && tag.s.dirty == 1) {
                if (flash_seal_page(dblk, p) != 0) return -EIO;
            }
        }
    }

    /* Phase 2: CoW 헤더 (logical page 0) */
    int blk      = node->block_id;
    int old_phys = find_latest_valid_phys(blk, 0);
    if (old_phys < 0)
        old_phys = find_latest_active_phys(blk, 0);

    int new_phys = flash_alloc_page(blk);
    if (new_phys < 0) {
        if (gc_collect_block(blk) != 0) return -ENOSPC;
        blk      = node->block_id;
        old_phys = find_latest_valid_phys(blk, 0);
        if (old_phys < 0)
            old_phys = find_latest_active_phys(blk, 0);
        new_phys = flash_alloc_page(blk);
        if (new_phys < 0) return -ENOSPC;
    }

    u32 now = (u32)time(NULL);
    uffs_FileInfo fi_data;
    fill_file_info(&fi_data, node, now);

    u8       block_ts = get_block_ts(blk);
    uffs_Tag tag;
    make_file_tag(&tag, node->serial, node->parent, 0,
                  (u16)sizeof(uffs_FileInfo), block_ts);

    if (flash_write_page_unsealed(blk, new_phys, &fi_data,
                                   sizeof(fi_data), &tag) != 0)
        return -EIO;
    if (flash_seal_page(blk, new_phys) != 0) return -EIO;
    if (old_phys >= 0)
        flash_obsolete_page(blk, old_phys);

    /* Phase 3: 물리적 동기화 */
    if (flash_sync() != 0) return -EIO;

    node->last_modify = now;
    node->is_dirty    = 0;
    return 0;
}

/* ─── file_rename (FR-FILE-006) ─────────────────────────── */
/*
 * 인메모리 트리만 갱신한다 (내구성은 호출자가 fsync로 보장).
 * 크래시 후 remount 시 on-disk의 구 헤더가 복원되므로 rename은
 * fsync 없이 내구성이 보장되지 않는다 (POSIX 허용 동작).
 */
int file_rename(const char *from, const char *to, unsigned int flags)
{
    (void)flags;

    TreeNode *node = tree_find_file(from);
    if (!node) return -ENOENT;

    char to_parent[512], to_child[512];
    utils_path_split(to, to_parent, to_child);

    TreeNode *new_parent = tree_find_dir(to_parent);
    if (!new_parent) return -ENOENT;

    /* 대상에 기존 파일이 있으면 덮어쓰기 (POSIX 표준) */
    TreeNode *dst = tree_find_file(to);
    if (dst && dst->serial != node->serial)
        file_unlink(to);

    /* 인메모리 트리 갱신 */
    u32 now = (u32)time(NULL);
    node->parent      = new_parent->serial;
    node->is_dirty    = 1;
    node->last_modify = now;
    strncpy(node->name, to_child, MAX_FILENAME_LEN - 1);
    node->name[MAX_FILENAME_LEN - 1] = '\0';

    return 0;
}

/* ─── file_unlink (FR-FILE-007) ─────────────────────────── */
/*
 * 모든 유효 물리 페이지를 dirty 1→0 flip으로 무효화한다.
 * GC는 obsolete 페이지만 남은 블록을 자동으로 회수한다.
 */
int file_unlink(const char *path)
{
    TreeNode *node = tree_find_file(path);
    if (!node) return -ENOENT;

    /* FILE 블록의 모든 유효 페이지 무효화 */
    for (int p = 0; p < PAGES_PER_BLOCK; p++) {
        uffs_MiniHeader hdr;
        uffs_Tag        tag;
        if (flash_read_page(node->block_id, p, &hdr, NULL, &tag) != 0) continue;
        if (hdr.status == 0xFF) break;
        if ((tag.seal_byte == SEAL_DONE || tag.seal_byte == SEAL_WRITING) &&
            tag.s.dirty == 1)
            flash_obsolete_page(node->block_id, p);
    }

    /* DATA 블록의 모든 유효 페이지 무효화 */
    for (int i = 0; i < g_tree.data_count; i++) {
        if (g_tree.data_table[i].file_serial != node->serial) continue;
        int dblk = g_tree.data_table[i].block_id;
        for (int p = 0; p < PAGES_PER_BLOCK; p++) {
            uffs_MiniHeader hdr;
            uffs_Tag        tag;
            if (flash_read_page(dblk, p, &hdr, NULL, &tag) != 0) continue;
            if (hdr.status == 0xFF) break;
            if ((tag.seal_byte == SEAL_DONE || tag.seal_byte == SEAL_WRITING) &&
                tag.s.dirty == 1)
                flash_obsolete_page(dblk, p);
        }
    }

    u16 serial = node->serial;
    tree_remove_data_blocks_by_serial(serial);
    tree_remove_file(serial);
    return 0;
}
