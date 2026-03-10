#include "tree.h"
#include "flash.h"
#include "utils.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

/* ─── 전역 트리 인스턴스 ────────────────────────────────────── */
UffsTree g_tree;

/* ─── 내부 헬퍼 ─────────────────────────────────────────────── */

/* modulo-3 block_ts 비교: new_ts가 old_ts보다 새로우면 1 반환 */
static int block_ts_is_newer(u8 new_ts, u8 old_ts)
{
    return ((int)new_ts - (int)old_ts + 3) % 3 == 1;
}

/* 블록에서 첫 번째 유효(SEAL_DONE && dirty==1) 페이지의 block_ts 반환 */
static u8 read_block_ts(int block_id)
{
    for (int p = 0; p < PAGES_PER_BLOCK; p++) {
        uffs_MiniHeader hdr;
        uffs_Tag        tag;
        if (flash_read_page(block_id, p, &hdr, NULL, &tag) != 0) continue;
        if (hdr.status == 0xFF) break;
        if (tag.seal_byte == SEAL_DONE && tag.s.dirty == 1)
            return (u8)tag.s.block_ts;
    }
    return 0;
}

/* ─── tree_build: 마운트 스캔 (SDD §5.2) ───────────────────── */
/*
 * Phase 0: 불완전 erase 복구
 *   page 0 = 0xFF(erased) 인데 page 1 != 0xFF → 블록 전체 erase
 *
 * Phase 1: 블록 스캔
 *   각 블록에서 logical page_id==0 && SEAL_DONE && dirty==1 인
 *   가장 높은 물리 인덱스의 헤더 페이지를 기준으로 트리에 삽입.
 *   동일 시리얼 중복 시 block_ts(modulo-3)로 더 새로운 블록 선택.
 *   패배한 블록은 erase_pending에 추가.
 *
 * Phase 2: pending 블록 erase
 */
int tree_build(void)
{
    memset(&g_tree, 0, sizeof(g_tree));

    /* ── Phase 0: 불완전 erase 복구 ─────────────────────────── */
    for (int b = 2; b < TOTAL_BLOCKS; b++) {
        uffs_MiniHeader h0, h1;
        if (flash_read_page(b, 0, &h0, NULL, NULL) != 0) continue;
        if (h0.status != 0xFF) continue; /* 사용 중 블록 → 스킵 */
        /* page 0은 erased, page 1 확인 */
        if (flash_read_page(b, 1, &h1, NULL, NULL) != 0) continue;
        if (h1.status != 0xFF)
            flash_erase_block(b); /* 불완전 erase → 마저 erase */
    }

    /* ── Phase 1: 블록 스캔 ─────────────────────────────────── */
    int erase_pending[TOTAL_BLOCKS];
    int erase_count = 0;

    for (int b = 1; b < TOTAL_BLOCKS; b++) {
        uffs_MiniHeader hdr0;
        if (flash_read_page(b, 0, &hdr0, NULL, NULL) != 0) continue;
        if (hdr0.status == 0xFF) continue; /* 빈 블록 */

        /* logical page_id==0 의 최신 유효 물리 페이지 탐색 (높은 인덱스 우선) */
        uffs_Tag      best_tag;
        uffs_FileInfo best_fi;
        int           best_phys = -1;

        for (int p = PAGES_PER_BLOCK - 1; p >= 0; p--) {
            uffs_MiniHeader h;
            uffs_Tag        t;
            uffs_FileInfo   fi;
            if (flash_read_page(b, p, &h, &fi, &t) != 0) continue;
            if (h.status == 0xFF) continue;
            if (t.seal_byte == SEAL_DONE && t.s.dirty == 1 && t.s.page_id == 0) {
                best_phys = p;
                best_tag  = t;
                best_fi   = fi;
                break;
            }
        }

        if (best_phys < 0) continue; /* 유효 헤더 없음 */

        switch (best_tag.s.type) {

        case UFFS_TYPE_DIR: {
            TreeNode *existing = tree_find_dir_by_serial(best_tag.s.serial);
            if (existing) {
                u8 e_ts = read_block_ts(existing->block_id);
                if (block_ts_is_newer((u8)best_tag.s.block_ts, e_ts)) {
                    erase_pending[erase_count++] = existing->block_id;
                    existing->block_id    = b;
                    existing->create_time = best_fi.create_time;
                    existing->last_modify = best_fi.last_modify;
                    strncpy(existing->name, best_fi.name, MAX_FILENAME_LEN - 1);
                } else {
                    erase_pending[erase_count++] = b;
                }
            } else {
                tree_insert_dir(best_tag.s.serial, best_tag.s.parent,
                                b, best_fi.name);
                TreeNode *n = tree_find_dir_by_serial(best_tag.s.serial);
                if (n) {
                    n->create_time = best_fi.create_time;
                    n->last_modify = best_fi.last_modify;
                    n->size        = 0;
                }
            }
            break;
        }

        case UFFS_TYPE_FILE: {
            TreeNode *existing = tree_find_file_by_serial(best_tag.s.serial);
            if (existing) {
                u8 e_ts = read_block_ts(existing->block_id);
                if (block_ts_is_newer((u8)best_tag.s.block_ts, e_ts)) {
                    erase_pending[erase_count++] = existing->block_id;
                    existing->block_id    = b;
                    existing->size        = best_fi.size;
                    existing->create_time = best_fi.create_time;
                    existing->last_modify = best_fi.last_modify;
                    strncpy(existing->name, best_fi.name, MAX_FILENAME_LEN - 1);
                } else {
                    erase_pending[erase_count++] = b;
                }
            } else {
                tree_insert_file(best_tag.s.serial, best_tag.s.parent,
                                 b, best_fi.name);
                TreeNode *n = tree_find_file_by_serial(best_tag.s.serial);
                if (n) {
                    n->size        = best_fi.size;
                    n->create_time = best_fi.create_time;
                    n->last_modify = best_fi.last_modify;
                }
            }
            break;
        }

        case UFFS_TYPE_DATA: {
            /* tag.parent = file_serial, tag.serial = data_block_index */
            DataBlockEntry *existing =
                tree_find_data_block(best_tag.s.parent, best_tag.s.serial);
            if (existing) {
                u8 e_ts = read_block_ts(existing->block_id);
                if (block_ts_is_newer((u8)best_tag.s.block_ts, e_ts)) {
                    erase_pending[erase_count++] = existing->block_id;
                    existing->block_id = b;
                } else {
                    erase_pending[erase_count++] = b;
                }
            } else {
                tree_insert_data_block(best_tag.s.parent,
                                       best_tag.s.serial, b);
            }
            break;
        }

        default:
            break;
        }
    }

    /* ── Phase 2: pending 블록 erase ────────────────────────── */
    for (int i = 0; i < erase_count; i++)
        flash_erase_block(erase_pending[i]);

    return 0;
}

/* ─── 시리얼로 노드 검색 ────────────────────────────────────── */

TreeNode *tree_find_file_by_serial(u16 serial)
{
    for (int i = 0; i < g_tree.file_count; i++) {
        if (g_tree.files[i].serial == serial)
            return &g_tree.files[i];
    }
    return NULL;
}

TreeNode *tree_find_dir_by_serial(u16 serial)
{
    for (int i = 0; i < g_tree.dir_count; i++) {
        if (g_tree.dirs[i].serial == serial)
            return &g_tree.dirs[i];
    }
    return NULL;
}

/* ─── 경로로 노드 검색 ──────────────────────────────────────── */

TreeNode *tree_find_dir(const char *path)
{
    if (strcmp(path, "/") == 0) {
        return tree_find_dir_by_serial(ROOT_DIR_SERIAL);
    }

    char parent_path[512], child_name[512];
    utils_path_split(path, parent_path, child_name);

    TreeNode *parent = tree_find_dir(parent_path);
    if (!parent) return NULL;

    for (int i = 0; i < g_tree.dir_count; i++) {
        if (g_tree.dirs[i].parent == parent->serial &&
            strcmp(g_tree.dirs[i].name, child_name) == 0) {
            return &g_tree.dirs[i];
        }
    }
    return NULL;
}

TreeNode *tree_find_file(const char *path)
{
    char parent_path[512], child_name[512];
    utils_path_split(path, parent_path, child_name);

    TreeNode *parent = tree_find_dir(parent_path);
    if (!parent) return NULL;

    for (int i = 0; i < g_tree.file_count; i++) {
        if (g_tree.files[i].parent == parent->serial &&
            strcmp(g_tree.files[i].name, child_name) == 0) {
            return &g_tree.files[i];
        }
    }
    return NULL;
}

/* ─── 시리얼 할당 ───────────────────────────────────────────── */

u16 tree_alloc_serial(void)
{
    /* 1 ~ 0xFE 범위에서 사용 중이지 않은 가장 작은 번호 */
    for (u16 s = 1; s < ROOT_DIR_SERIAL; s++) {
        int in_use = 0;
        for (int i = 0; i < g_tree.dir_count; i++)
            if (g_tree.dirs[i].serial == s) { in_use = 1; break; }
        if (!in_use) {
            for (int i = 0; i < g_tree.file_count; i++)
                if (g_tree.files[i].serial == s) { in_use = 1; break; }
        }
        if (!in_use) return s;
    }
    return 0; /* 0 = 할당 불가 */
}

/* ─── 노드 삽입 ─────────────────────────────────────────────── */

int tree_insert_dir(u16 serial, u16 parent, int block_id, const char *name)
{
    if (g_tree.dir_count >= MAX_DIR_COUNT) return -1;
    TreeNode *n    = &g_tree.dirs[g_tree.dir_count++];
    n->serial      = serial;
    n->parent      = parent;
    n->block_id    = block_id;
    n->size        = 0;
    n->is_dirty    = 0;
    n->create_time = 0;
    n->last_modify = 0;
    strncpy(n->name, name, MAX_FILENAME_LEN - 1);
    n->name[MAX_FILENAME_LEN - 1] = '\0';
    return 0;
}

int tree_insert_file(u16 serial, u16 parent, int block_id, const char *name)
{
    if (g_tree.file_count >= MAX_FILE_COUNT) return -1;
    TreeNode *n    = &g_tree.files[g_tree.file_count++];
    n->serial      = serial;
    n->parent      = parent;
    n->block_id    = block_id;
    n->size        = 0;
    n->is_dirty    = 0;
    n->create_time = 0;
    n->last_modify = 0;
    strncpy(n->name, name, MAX_FILENAME_LEN - 1);
    n->name[MAX_FILENAME_LEN - 1] = '\0';
    return 0;
}

/* ─── 노드 제거 ─────────────────────────────────────────────── */

int tree_remove_file(u16 serial)
{
    for (int i = 0; i < g_tree.file_count; i++) {
        if (g_tree.files[i].serial == serial) {
            /* 마지막 항목으로 덮어쓰고 카운트 감소 */
            g_tree.files[i] = g_tree.files[--g_tree.file_count];
            return 0;
        }
    }
    return -1;
}

int tree_remove_dir(u16 serial)
{
    for (int i = 0; i < g_tree.dir_count; i++) {
        if (g_tree.dirs[i].serial == serial) {
            g_tree.dirs[i] = g_tree.dirs[--g_tree.dir_count];
            return 0;
        }
    }
    return -1;
}

/* ─── 자식 노드 열거 ────────────────────────────────────────── */

void tree_enumerate_children(u16 parent_serial,
                              TreeNode **out_nodes, int *out_count,
                              int max_count)
{
    int cnt = 0;
    for (int i = 0; i < g_tree.dir_count && cnt < max_count; i++) {
        if (g_tree.dirs[i].parent == parent_serial &&
            g_tree.dirs[i].serial != ROOT_DIR_SERIAL) {
            out_nodes[cnt++] = &g_tree.dirs[i];
        }
    }
    for (int i = 0; i < g_tree.file_count && cnt < max_count; i++) {
        if (g_tree.files[i].parent == parent_serial) {
            out_nodes[cnt++] = &g_tree.files[i];
        }
    }
    *out_count = cnt;
}

/* ─── DATA 블록 관리 ────────────────────────────────────────── */

DataBlockEntry *tree_find_data_block(u16 file_serial, u16 data_idx)
{
    for (int i = 0; i < g_tree.data_count; i++) {
        if (g_tree.data_table[i].file_serial == file_serial &&
            g_tree.data_table[i].data_idx    == data_idx) {
            return &g_tree.data_table[i];
        }
    }
    return NULL;
}

int tree_insert_data_block(u16 file_serial, u16 data_idx, int block_id)
{
    if (g_tree.data_count >= MAX_DATA_COUNT) return -1;
    DataBlockEntry *e  = &g_tree.data_table[g_tree.data_count++];
    e->file_serial     = file_serial;
    e->data_idx        = data_idx;
    e->block_id        = block_id;
    return 0;
}

/* ─── tree_remove_data_blocks_by_serial ──────────────────── */

int tree_remove_data_blocks_by_serial(u16 file_serial)
{
    int i = 0;
    while (i < g_tree.data_count) {
        if (g_tree.data_table[i].file_serial == file_serial)
            g_tree.data_table[i] = g_tree.data_table[--g_tree.data_count];
        else
            i++;
    }
    return 0;
}

/* ─── tree_update_block_id ───────────────────────────────── */

void tree_update_block_id(int old_id, int new_id)
{
    for (int i = 0; i < g_tree.dir_count; i++)
        if (g_tree.dirs[i].block_id == old_id)
            g_tree.dirs[i].block_id = new_id;
    for (int i = 0; i < g_tree.file_count; i++)
        if (g_tree.files[i].block_id == old_id)
            g_tree.files[i].block_id = new_id;
    for (int i = 0; i < g_tree.data_count; i++)
        if (g_tree.data_table[i].block_id == old_id)
            g_tree.data_table[i].block_id = new_id;
}
