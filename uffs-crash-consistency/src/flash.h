#pragma once

#include <stdint.h>
#include <stddef.h>

/* ─── 기본 타입 ───────────────────────────────────────────── */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

/* ─── Flash 지오메트리 상수 (SDD §8.2) ───────────────────── */
#define PAGES_PER_BLOCK     32
#define PAGE_DATA_SIZE      512
#define PAGE_SPARE_SIZE     16
#define PAGE_SIZE           528          /* MiniHeader(4) + Data(512) + Tag(12) */
#define TOTAL_BLOCKS        128
#define MAX_FILENAME_LEN    488          /* PAGE_DATA_SIZE - 24 */

/* ─── Seal byte 값 (SDD §4.3) ────────────────────────────── */
#define SEAL_EMPTY          0xFF         /* 빈 페이지 */
#define SEAL_WRITING        0xFE         /* 쓰기 진행 중 (Unseal) */
#define SEAL_DONE           0xFC         /* 씰 완료 */
/* CoW 무효화: tag.s.dirty 1→0 flip (NAND 호환, uffs-reference 방식)
 * 유효 페이지 = seal_byte==SEAL_DONE && tag.s.dirty==1
 * flash_obsolete_page()가 TAG 바이트 0의 bit 0을 1→0으로 flip */

/* ─── 블록 타입 (SDD §4.1) ───────────────────────────────── */
#define UFFS_TYPE_DIR       1
#define UFFS_TYPE_FILE      2
#define UFFS_TYPE_DATA      3

/* ─── 특수 시리얼 번호 (SDD §4.6) ───────────────────────── */
#define ROOT_DIR_SERIAL     0xFF

/* ─── 파일 속성 ──────────────────────────────────────────── */
#define FILE_ATTR_DIR       0x80
#define FILE_ATTR_WRITE     0x01

/* ─── 페이지 내 오프셋 ───────────────────────────────────── */
#define PAGE_MINIHEADER_OFF  0
#define PAGE_DATA_OFF        4
#define PAGE_TAG_OFF         516         /* 4 + 512 */
#define PAGE_SEAL_BYTE_OFF   526         /* 4 + 512 + 8 + 2 */

/* ─── 파일 데이터 용량 상수 ──────────────────────────────── */
#define FILE_HEADER_DATA_PAGES  31       /* FILE 블록 pages 1..31 */
#define DATA_BLOCK_PAGES        32       /* DATA 블록 pages 0..31 */
#define MAX_DATA_IN_HEADER      (FILE_HEADER_DATA_PAGES * PAGE_DATA_SIZE)   /* 15872 */
#define MAX_DATA_PER_DATABLOCK  (DATA_BLOCK_PAGES * PAGE_DATA_SIZE)          /* 16384 */

/* ─── 온디스크 구조체 ────────────────────────────────────── */

/* MiniHeader: 페이지 앞 4바이트 (SDD §4.2) */
typedef struct {
    u8  status;     /* 0xFF=empty, 0x01=used */
    u8  reserved;
    u16 crc;        /* CRC 미구현; 항상 0 */
} uffs_MiniHeader;

/* TagStore: 8바이트 비트필드 (SDD §4.3 + uffs-reference uffs_public.h 구조 정렬)
 * 1st word (32b): dirty(1)+valid(1)+type(2)+block_ts(2)+data_len(12)+serial(14)
 * 2nd word (32b): parent(10)+page_id(6)+reserved(4)+tag_ecc(12)
 */
struct uffs_TagStoreSt {
    u32 dirty    : 1;   /* 0=dirty, 1=clean */
    u32 valid    : 1;   /* 0=valid, 1=invalid */
    u32 type     : 2;   /* DIR=1, FILE=2, DATA=3 */
    u32 block_ts : 2;
    u32 data_len : 12;  /* 유효 바이트 수 */
    u32 serial   : 14;  /* 파일/디렉토리 고유 번호 */
    u32 parent   : 10;  /* 부모 시리얼 */
    u32 page_id  : 6;   /* 블록 내 페이지 인덱스 */
    u32 reserved : 4;   /* 예약 (uffs-reference 호환) */
    u32 tag_ecc  : 12;
};

/* Tag: 12바이트 = TagStore(8) + data_sum(2) + seal_byte(1) + pad(1) */
typedef struct {
    struct uffs_TagStoreSt s;
    u16 data_sum;
    u8  seal_byte;
    u8  _pad;
} uffs_Tag;

/* FileInfo: Page 0의 Data 영역 512바이트 (SDD §4.5) */
typedef struct {
    u32  attr;                   /* FILE_ATTR_DIR or FILE_ATTR_WRITE */
    u32  create_time;            /* UNIX timestamp */
    u32  last_modify;
    u32  access;
    u32  size;                   /* 파일 크기 (reserved 재사용; fsync 시 기록) */
    u32  name_len;
    char name[MAX_FILENAME_LEN]; /* null-terminated */
} uffs_FileInfo;

/* 컴파일 타임 크기 검증 */
_Static_assert(sizeof(uffs_MiniHeader) == 4,  "MiniHeader must be 4 bytes");
_Static_assert(sizeof(uffs_Tag)        == 12, "Tag must be 12 bytes");
_Static_assert(sizeof(uffs_FileInfo)   == 512,"FileInfo must be 512 bytes");

/* ─── Flash I/O API ──────────────────────────────────────── */

int flash_init(const char *device_path);
int flash_format(void);
int flash_format_check(void);

int flash_read_page(int block_id, int page_id,
                    uffs_MiniHeader *hdr, void *data, uffs_Tag *tag);

int flash_write_page_unsealed(int block_id, int page_id,
                               const void *data, size_t data_len,
                               const uffs_Tag *tag_tmpl);

int flash_seal_page(int block_id, int page_id);
int flash_sync(void);
int flash_alloc_block(void);        /* 빈 블록 ID 반환; -ENOSPC on full */
int flash_alloc_page(int block_id); /* 블록 내 다음 빈 물리 페이지 반환; -1이면 블록 가득 참 */
int flash_obsolete_page(int block_id, int page_id); /* tag.s.dirty 1→0 flip (NAND 호환) */
int flash_erase_block(int block_id);                /* 블록 전체를 0xFF로 초기화 (GC용) */
