#include "gc.h"
#include "flash.h"
#include "tree.h"

#include <string.h>
#include <errno.h>

/* ─── gc_init ────────────────────────────────────────────── */
/*
 * tree_build() 완료 후 호출. 플래시를 역방향 스캔하여 실제 spare 블록을
 * 찾고 flash_set_gc_spare()로 등록한다.
 *
 * 왜 역방향(TOTAL_BLOCKS-1 → 2)인가:
 *   최초 포맷 시 block 127(GC_SPARE_BLOCK)이 비어있다.
 *   GC rolling 이후에는 다른 번호의 블록이 spare가 될 수 있으므로
 *   전체 스캔으로 실제 빈 블록을 찾는다.
 */
int gc_init(void)
{
    for (int b = TOTAL_BLOCKS - 1; b >= 2; b--) {
        uffs_MiniHeader hdr;
        if (flash_read_page(b, 0, &hdr, NULL, NULL) != 0) continue;
        if (hdr.status == 0xFF) {
            flash_set_gc_spare(b);
            return 0;
        }
    }
    /* 빈 블록이 전혀 없음: 스토리지 완전 소진 */
    flash_set_gc_spare(-1);
    return -ENOSPC;
}

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
 *     1. GC 전용 spare 블록 사용 (flash_alloc_block 대신 → 데드락 방지)
 *     2. logical_pg별 최신 유효 물리 페이지를 spare 블록에 순차 복사
 *        (block_ts를 modulo-3으로 1 증가)
 *     3. tree_update_block_id(old, spare)
 *     4. 구 블록 erase → 구 블록이 새 spare (rolling spare)
 *
 * Rolling spare 원리:
 *   spare_before → live 페이지 복사 목적지
 *   victim 블록 → erase 후 새 spare
 *   flash_alloc_block은 spare를 제외하고 탐색하므로 항상 1개 여유 보장
 */
int gc_collect_block(int block_id)
{
    /* 유효 페이지 확인 */
    if (count_valid_pages(block_id) == 0)
        return flash_erase_block(block_id);

    /* GC 전용 spare 블록 사용 (flash_alloc_block 대신) */
    int new_blk = flash_get_gc_spare();
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

    /* 트리 갱신 후 구 블록 erase → 구 블록이 새 spare (rolling) */
    tree_update_block_id(block_id, new_blk);
    if (flash_erase_block(block_id) != 0) goto fail;
    flash_set_gc_spare(block_id); /* 구 블록이 다음 GC의 spare */
    return 0;

fail:
    /* 복사 도중 실패: spare 블록 내용을 erase해 spare 상태로 복원 */
    flash_erase_block(new_blk);
    /* g_spare_block은 여전히 new_blk이므로 spare 상태 유지됨 */
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
