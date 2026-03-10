#pragma once

/* ─── On-demand GC (SDD §6.x) ─────────────────────────────
 *
 * NAND 특성상 블록 전체를 erase해야만 재사용 가능하다.
 * CoW 쓰기로 obsolete(dirty==0) 페이지가 쌓이면 블록이 꽉 차서
 * 더 이상 새 물리 페이지를 할당할 수 없게 된다.
 * 그 시점에 on-demand GC를 호출해 공간을 확보한다.
 *
 * victim 선택 전략
 *   gc_collect_block : 특정 블록을 강제 GC (블록이 꽉 찼을 때)
 *   gc_collect       : 전역 victim 선택 (빈 블록 자체가 없을 때)
 *     - Pass 1: 유효 페이지가 하나도 없는 블록 → 즉시 erase
 *     - Pass 2: obsolete 페이지가 가장 많은 블록 → gc_collect_block
 */

int gc_collect_block(int block_id); /* 특정 블록 GC */
int gc_collect(void);               /* 전역 GC: 최적 victim 선택 */
