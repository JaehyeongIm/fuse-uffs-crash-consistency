#pragma once

#include "flash.h"

/* ─── 크기 상수 (SDD §3.5) ────────────────────────────────── */
#define MAX_DIR_COUNT    32
#define MAX_FILE_COUNT   64
#define MAX_DATA_COUNT   512

/* ─── TreeNode: 디렉토리·파일 공통 노드 (SDD §3.5) ─────────── */
typedef struct {
    u16  serial;                     /* 고유 시리얼 번호 */
    u16  parent;                     /* 부모 디렉토리 시리얼 */
    int  block_id;                   /* 헤더 블록 ID */
    char name[MAX_FILENAME_LEN];     /* 파일·디렉토리 이름 */
    u32  size;                       /* 파일 크기 (file only; dir = 0) */
    u32  create_time;
    u32  last_modify;
    u8   is_dirty;                   /* 마지막 fsync 이후 변경 여부 */
} TreeNode;

/* ─── DATA 블록 테이블 엔트리 ────────────────────────────────── */
typedef struct {
    u16 file_serial;   /* 어떤 파일의 DATA 블록인가 (tag.parent) */
    u16 data_idx;      /* 파일 내 DATA 블록 인덱스 (0-based, tag.serial) */
    int block_id;
} DataBlockEntry;

/* ─── UffsTree: 전역 인메모리 트리 ──────────────────────────── */
typedef struct {
    TreeNode      dirs[MAX_DIR_COUNT];
    TreeNode      files[MAX_FILE_COUNT];
    DataBlockEntry data_table[MAX_DATA_COUNT];
    int           dir_count;
    int           file_count;
    int           data_count;
} UffsTree;

extern UffsTree g_tree;

/* ─── API ──────────────────────────────────────────────────── */

/* 마운트 시 플래시 스캔으로 트리 구축 (SDD §5.2) */
int tree_build(void);

/* 경로로 노드 검색 */
TreeNode *tree_find_file(const char *path);
TreeNode *tree_find_dir(const char *path);

/* 시리얼로 노드 검색 (내부용) */
TreeNode *tree_find_file_by_serial(u16 serial);
TreeNode *tree_find_dir_by_serial(u16 serial);

/* 새 시리얼 할당 (사용 중이지 않은 가장 작은 값) */
u16 tree_alloc_serial(void);

/* 노드 삽입 */
int tree_insert_file(u16 serial, u16 parent, int block_id, const char *name);
int tree_insert_dir(u16 serial, u16 parent, int block_id, const char *name);

/* 노드 제거 */
int tree_remove_file(u16 serial);
int tree_remove_dir(u16 serial);

/* 자식 노드 열거 (dirs + files) */
void tree_enumerate_children(u16 parent_serial,
                              TreeNode **out_nodes, int *out_count,
                              int max_count);

/* 해당 파일의 DATA 블록 조회 */
DataBlockEntry *tree_find_data_block(u16 file_serial, u16 data_idx);

/* DATA 블록 등록 */
int tree_insert_data_block(u16 file_serial, u16 data_idx, int block_id);

/* 파일에 속한 모든 DATA 블록 테이블 항목 제거 (file_unlink용) */
int tree_remove_data_blocks_by_serial(u16 file_serial);

/* GC: 블록 ID 변경 시 트리 전체 갱신 */
void tree_update_block_id(int old_id, int new_id);
