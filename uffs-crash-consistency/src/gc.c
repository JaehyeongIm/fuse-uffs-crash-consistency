#include "gc.h"
#include "flash.h"
#include "tree.h"

#include <string.h>
#include <errno.h>

/* ─── 내부 헬퍼 ─────────────────────────────────────────── */

/* 블록이 실제 사용 중(page 0 status != 0xFF)인지 확인 */
static int block_is_used(int block_id)
{
    uffs_MiniHeader hdr;
    if (flash_read_page(block_id, 0, &hdr, NULL, NULL) != 0) return 0;
    return hdr.status != 0xFF;
}

/* 블록 내 SEAL_DONE && dirty==1 페이지 수 반환 */
static int count_valid_pages(int block_id)
{
    int cnt = 0;
    for (int p = 0; p < PAGES_PER_BLOCK; p++) {
        uffs_MiniHeader hdr;
        uffs_Tag        tag;
        if (flash_read_page(block_id, p, &hdr, NULL, &tag) != 0) continue;
        if (hdr.status == 0xFF) break; /* 이후 페이지도 비어있음 */
        if (tag.seal_byte == SEAL_DONE && tag.s.dirty == 1) cnt++;
    }
    return cnt;
}

/* 블록 내 dirty==0 (obsolete) 페이지 수 반환 */
static int count_obsolete_pages(int block_id)
{
    int cnt = 0;
    for (int p = 0; p < PAGES_PER_BLOCK; p++) {
        uffs_MiniHeader hdr;
        uffs_Tag        tag;
        if (flash_read_page(block_id, p, &hdr, NULL, &tag) != 0) continue;
        if (hdr.status == 0xFF) break;
        if (tag.s.dirty == 0) cnt++;
    }
    return cnt;
}

/* ─── gc_collect_block ───────────────────────────────────── */
/*
 * 특정 블록을 GC한다.
 * - 유효 페이지(SEAL_DONE && dirty==1)가 없으면 바로 erase.
 * - 유효 페이지가 있으면:
 *     1. 새 블록 할당
 *     2. logical_pg별 최신 유효 물리 페이지를 새 블록에 순차 복사
 *        (block_ts를 modulo-3으로 1 증가)
 *     3. tree_update_block_id(old, new)
 *     4. 구 블록 erase
 */
int gc_collect_block(int block_id)
{
    /* 유효 페이지 확인 */
    if (count_valid_pages(block_id) == 0)
        return flash_erase_block(block_id);

    /* 새 블록 할당 */
    int new_blk = flash_alloc_block();
    if (new_blk < 0) return -ENOSPC;

    /* 구 블록 block_ts 읽기 */
    u8 old_ts = 0;
    for (int p = 0; p < PAGES_PER_BLOCK; p++) {
        uffs_MiniHeader hdr;
        uffs_Tag        tag;
        if (flash_read_page(block_id, p, &hdr, NULL, &tag) != 0) continue;
        if (hdr.status == 0xFF) break;
        if (tag.seal_byte == SEAL_DONE && tag.s.dirty == 1) {
            old_ts = (u8)tag.s.block_ts;
            break;
        }
    }
    u8 new_ts = (u8)((old_ts + 1) % 3); /* 2-bit modulo-3 */

    /* logical_pg → 최신 유효 물리 페이지 인덱스 매핑
     * (더 높은 물리 인덱스가 최신 CoW 버전) */
    int best_phys[PAGES_PER_BLOCK];
    memset(best_phys, -1, sizeof(best_phys));

    for (int p = 0; p < PAGES_PER_BLOCK; p++) {
        uffs_MiniHeader hdr;
        uffs_Tag        tag;
        if (flash_read_page(block_id, p, &hdr, NULL, &tag) != 0) continue;
        if (hdr.status == 0xFF) break;
        if (tag.seal_byte == SEAL_DONE && tag.s.dirty == 1) {
            int lg = (int)tag.s.page_id;
            if (lg >= 0 && lg < PAGES_PER_BLOCK)
                best_phys[lg] = p; /* 높은 인덱스가 덮어씀 → 자동으로 최신 */
        }
    }

    /* 유효 페이지를 새 블록에 순차 기록 */
    u8  page_data[PAGE_DATA_SIZE];
    int new_p = 0;

    for (int lg = 0; lg < PAGES_PER_BLOCK; lg++) {
        if (best_phys[lg] < 0) continue;

        uffs_MiniHeader hdr;
        uffs_Tag        tag;
        if (flash_read_page(block_id, best_phys[lg], &hdr, page_data, &tag) != 0)
            goto fail;

        tag.s.block_ts = new_ts;

        if (flash_write_page_unsealed(new_blk, new_p, page_data,
                                      tag.s.data_len, &tag) != 0) goto fail;
        if (flash_seal_page(new_blk, new_p) != 0) goto fail;
        new_p++;
    }

    /* 트리 갱신 및 구 블록 erase */
    tree_update_block_id(block_id, new_blk);
    return flash_erase_block(block_id);

fail:
    flash_erase_block(new_blk);
    return -EIO;
}

/* ─── gc_collect ─────────────────────────────────────────── */
/*
 * 전역 GC: 빈 블록이 없을 때 호출.
 * Pass 1: 유효 페이지가 전혀 없는 사용 중 블록 → 즉시 erase (빠름)
 * Pass 2: obsolete 페이지가 가장 많은 블록 → gc_collect_block
 */
int gc_collect(void)
{
    /* Pass 1: all-obsolete 블록 즉시 erase */
    for (int b = 2; b < TOTAL_BLOCKS; b++) {
        if (!block_is_used(b)) continue;
        if (count_valid_pages(b) == 0)
            return flash_erase_block(b);
    }

    /* Pass 2: obsolete 비율이 가장 높은 블록 GC */
    int best_blk = -1, best_obs = 0;
    for (int b = 2; b < TOTAL_BLOCKS; b++) {
        if (!block_is_used(b)) continue;
        int obs = count_obsolete_pages(b);
        if (obs > best_obs) {
            best_obs = obs;
            best_blk = b;
        }
    }

    if (best_blk < 0) return -ENOSPC;
    return gc_collect_block(best_blk);
}
