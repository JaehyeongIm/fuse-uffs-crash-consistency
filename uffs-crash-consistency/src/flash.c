#include "flash.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>

static int g_flash_fd = -1;

/* ─── 내부 헬퍼 ─────────────────────────────────────────── */

static off_t page_off(int block_id, int page_id)
{
    return (off_t)block_id * PAGES_PER_BLOCK * PAGE_SIZE
         + (off_t)page_id * PAGE_SIZE;
}

/* ─── flash_init ─────────────────────────────────────────── */

int flash_init(const char *device_path)
{
    off_t expected = (off_t)TOTAL_BLOCKS * PAGES_PER_BLOCK * PAGE_SIZE;

    /* 기존 이미지 열기 시도 */
    g_flash_fd = open(device_path, O_RDWR);
    if (g_flash_fd >= 0) {
        /* 크기 확인 */
        off_t cur = lseek(g_flash_fd, 0, SEEK_END);
        if (cur >= expected) return 0;
        /* 크기 부족 → 아래에서 재초기화 */
        close(g_flash_fd);
        g_flash_fd = -1;
    }

    /* 새 이미지 파일 생성 및 0xFF 초기화 */
    g_flash_fd = open(device_path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (g_flash_fd < 0) {
        perror("flash_init: open");
        return -errno;
    }

    /* PAGE_SIZE 단위로 0xFF 채우기 */
    u8 buf[PAGE_SIZE];
    memset(buf, 0xFF, PAGE_SIZE);
    for (int b = 0; b < TOTAL_BLOCKS; b++) {
        for (int p = 0; p < PAGES_PER_BLOCK; p++) {
            off_t off = page_off(b, p);
            if (pwrite(g_flash_fd, buf, PAGE_SIZE, off) != PAGE_SIZE) {
                perror("flash_init: pwrite");
                return -EIO;
            }
        }
    }
    return 0;
}

/* ─── flash_format ───────────────────────────────────────── */

int flash_format(void)
{
    /* Block 0: 매직 "UFFS" */
    u8 magic_data[PAGE_DATA_SIZE];
    memset(magic_data, 0, sizeof(magic_data));
    magic_data[0] = 'U';
    magic_data[1] = 'F';
    magic_data[2] = 'F';
    magic_data[3] = 'S';

    uffs_Tag tag;
    memset(&tag, 0, sizeof(tag));
    tag.s.type     = UFFS_TYPE_FILE; /* 매직 블록은 타입 무관 */
    tag.s.serial   = 0;
    tag.s.parent   = 0;
    tag.s.page_id  = 0;
    tag.s.data_len = 4;
    tag.s.dirty    = 1; /* clean */
    tag.s.valid    = 0; /* valid */

    if (flash_write_page_unsealed(0, 0, magic_data, sizeof(magic_data), &tag) != 0)
        return -EIO;
    if (flash_seal_page(0, 0) != 0)
        return -EIO;

    /* Block 1: 루트 디렉토리 (serial=0xFF, parent=0xFF) */
    uffs_FileInfo fi;
    memset(&fi, 0, sizeof(fi));
    fi.attr        = FILE_ATTR_DIR;
    fi.create_time = (u32)time(NULL);
    fi.last_modify = fi.create_time;
    fi.access      = fi.create_time;
    fi.size        = 0;
    fi.name_len    = 1;
    fi.name[0]     = '/';
    fi.name[1]     = '\0';

    uffs_Tag root_tag;
    memset(&root_tag, 0, sizeof(root_tag));
    root_tag.s.type     = UFFS_TYPE_DIR;
    root_tag.s.serial   = ROOT_DIR_SERIAL;
    root_tag.s.parent   = ROOT_DIR_SERIAL;
    root_tag.s.page_id  = 0;
    root_tag.s.data_len = sizeof(uffs_FileInfo);
    root_tag.s.dirty    = 1;
    root_tag.s.valid    = 0;

    if (flash_write_page_unsealed(1, 0, &fi, sizeof(fi), &root_tag) != 0)
        return -EIO;
    if (flash_seal_page(1, 0) != 0)
        return -EIO;

    return flash_sync();
}

/* ─── flash_format_check ─────────────────────────────────── */

int flash_format_check(void)
{
    u8 data[PAGE_DATA_SIZE];
    uffs_MiniHeader hdr;
    uffs_Tag tag;

    if (flash_read_page(0, 0, &hdr, data, &tag) != 0) return 0;
    return (data[0] == 'U' && data[1] == 'F' && data[2] == 'F' && data[3] == 'S');
}

/* ─── flash_read_page ────────────────────────────────────── */

int flash_read_page(int block_id, int page_id,
                    uffs_MiniHeader *hdr, void *data, uffs_Tag *tag)
{
    u8 buf[PAGE_SIZE];
    off_t off = page_off(block_id, page_id);

    ssize_t n = pread(g_flash_fd, buf, PAGE_SIZE, off);
    if (n != PAGE_SIZE) return -EIO;

    if (hdr)  memcpy(hdr,  buf + PAGE_MINIHEADER_OFF, sizeof(uffs_MiniHeader));
    if (data) memcpy(data, buf + PAGE_DATA_OFF,        PAGE_DATA_SIZE);
    if (tag)  memcpy(tag,  buf + PAGE_TAG_OFF,         sizeof(uffs_Tag));
    return 0;
}

/* ─── flash_write_page_unsealed ──────────────────────────── */

int flash_write_page_unsealed(int block_id, int page_id,
                               const void *data, size_t data_len,
                               const uffs_Tag *tag_tmpl)
{
    u8 buf[PAGE_SIZE];
    memset(buf, 0xFF, PAGE_SIZE);

    /* MiniHeader */
    uffs_MiniHeader *hdr = (uffs_MiniHeader *)(buf + PAGE_MINIHEADER_OFF);
    hdr->status   = 0x01;
    hdr->reserved = 0;
    hdr->crc      = 0;

    /* Data */
    if (data && data_len > 0) {
        size_t len = data_len < PAGE_DATA_SIZE ? data_len : PAGE_DATA_SIZE;
        memcpy(buf + PAGE_DATA_OFF, data, len);
    }

    /* Tag */
    if (tag_tmpl) {
        memcpy(buf + PAGE_TAG_OFF, tag_tmpl, sizeof(uffs_Tag));
    }
    /* seal_byte를 SEAL_WRITING으로 덮어쓰기 */
    buf[PAGE_SEAL_BYTE_OFF] = SEAL_WRITING;

    off_t off = page_off(block_id, page_id);
    ssize_t n = pwrite(g_flash_fd, buf, PAGE_SIZE, off);
    if (n != PAGE_SIZE) return -EIO;
    return 0;
}

/* ─── flash_seal_page ────────────────────────────────────── */

int flash_seal_page(int block_id, int page_id)
{
    u8 seal = SEAL_DONE;
    off_t off = page_off(block_id, page_id) + PAGE_SEAL_BYTE_OFF;
    if (pwrite(g_flash_fd, &seal, 1, off) != 1) return -EIO;
    return 0;
}

/* ─── flash_sync ─────────────────────────────────────────── */

int flash_sync(void)
{
    if (fdatasync(g_flash_fd) != 0) return -EIO;
    return 0;
}

/* ─── flash_alloc_block ──────────────────────────────────── */

int flash_alloc_block(void)
{
    /* Block 0 = magic, Block 1 = root dir; 2 이상에서 빈 블록 탐색 */
    for (int b = 2; b < TOTAL_BLOCKS; b++) {
        uffs_MiniHeader hdr;
        if (flash_read_page(b, 0, &hdr, NULL, NULL) != 0) continue;
        if (hdr.status == 0xFF) return b; /* 0xFF = 미사용 블록 */
    }
    return -ENOSPC;
}

/* ─── flash_alloc_page ───────────────────────────────────── */

int flash_alloc_page(int block_id)
{
    for (int p = 0; p < PAGES_PER_BLOCK; p++) {
        uffs_MiniHeader hdr;
        if (flash_read_page(block_id, p, &hdr, NULL, NULL) != 0) continue;
        if (hdr.status == 0xFF) return p; /* 0xFF = 미기록 페이지 */
    }
    return -1; /* 블록 가득 참 */
}

/* ─── flash_obsolete_page ────────────────────────────────── */

int flash_obsolete_page(int block_id, int page_id)
{
    /* TagStore 첫 32-bit word의 bit 0 = dirty (little-endian)
     * PAGE_TAG_OFF(516) + 바이트 0의 bit 0을 1→0 flip → NAND 호환 */
    u8 byte0;
    off_t off = page_off(block_id, page_id) + PAGE_TAG_OFF;
    if (pread(g_flash_fd, &byte0, 1, off) != 1) return -EIO;
    byte0 &= ~0x01u; /* dirty 1→0 */
    if (pwrite(g_flash_fd, &byte0, 1, off) != 1) return -EIO;
    return 0;
}

/* ─── flash_erase_block ──────────────────────────────────── */

int flash_erase_block(int block_id)
{
    u8 buf[PAGE_SIZE];
    memset(buf, 0xFF, PAGE_SIZE);
    for (int p = 0; p < PAGES_PER_BLOCK; p++) {
        off_t off = page_off(block_id, p);
        if (pwrite(g_flash_fd, buf, PAGE_SIZE, off) != PAGE_SIZE) return -EIO;
    }
    return 0;
}
