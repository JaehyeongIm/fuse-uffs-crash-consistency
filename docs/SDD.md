# FUSE 기반 UFFS 파일시스템
## Software Design Description (SDD)

**문서 표준**: IEEE 1016-2009 (Software Design Descriptions)
**버전(Version)**: 1.0
**작성일(Date)**: 2026.03.04
**작성자(Author)**: 임재형

---

## 목차

- [FUSE 기반 UFFS 파일시스템](#fuse-기반-uffs-파일시스템)
  - [Software Design Description (SDD)](#software-design-description-sdd)
  - [목차](#목차)
- [1. Introduction (개요)](#1-introduction-개요)
  - [1.1 Purpose (목적)](#11-purpose-목적)
  - [1.2 Scope (범위)](#12-scope-범위)
  - [1.3 References (참조 문서)](#13-references-참조-문서)
  - [1.4 Definitions (용어 정의)](#14-definitions-용어-정의)
- [2. System Architecture (시스템 아키텍처)](#2-system-architecture-시스템-아키텍처)
  - [2.1 Architecture Overview (아키텍처 개요)](#21-architecture-overview-아키텍처-개요)
  - [2.2 Component Overview (컴포넌트 개요)](#22-component-overview-컴포넌트-개요)
  - [2.3 Concurrency Policy (동시성 정책)](#23-concurrency-policy-동시성-정책)
- [3. Component Design (컴포넌트 설계)](#3-component-design-컴포넌트-설계)
  - [3.1 FUSE Adapter (FUSE 어댑터)](#31-fuse-adapter-fuse-어댑터)
  - [3.2 File Manager (파일 관리자)](#32-file-manager-파일-관리자)
  - [3.3 Directory Manager (디렉토리 관리자)](#33-directory-manager-디렉토리-관리자)
  - [3.4 Meta Manager (메타데이터 관리자)](#34-meta-manager-메타데이터-관리자)
  - [3.5 Tree Manager (트리 관리자)](#35-tree-manager-트리-관리자)
  - [3.6 Flash I/O Layer (플래시 I/O 계층)](#36-flash-io-layer-플래시-io-계층)
- [4. Data Design (데이터 설계)](#4-data-design-데이터-설계)
  - [4.1 On-Disk Layout (온디스크 레이아웃)](#41-on-disk-layout-온디스크-레이아웃)
  - [4.2 Page Layout (페이지 레이아웃)](#42-page-layout-페이지-레이아웃)
  - [4.3 Tag Structure (태그 구조)](#43-tag-structure-태그-구조)
  - [4.4 Seal Mechanism (씰 메커니즘)](#44-seal-mechanism-씰-메커니즘)
  - [4.5 Block Header Structures (블록 헤더 구조체)](#45-block-header-structures-블록-헤더-구조체)
  - [4.6 Serial Number Scheme (시리얼 번호 체계)](#46-serial-number-scheme-시리얼-번호-체계)
  - [4.7 In-Memory Data Structures (인메모리 데이터 구조)](#47-in-memory-data-structures-인메모리-데이터-구조)
- [5. Crash Consistency Design (크래시 정합성 설계)](#5-crash-consistency-design-크래시-정합성-설계)
  - [5.1 Two-Phase Write Protocol (2단계 쓰기 프로토콜)](#51-two-phase-write-protocol-2단계-쓰기-프로토콜)
  - [5.2 Mount Scan Recovery (마운트 스캔 복구)](#52-mount-scan-recovery-마운트-스캔-복구)
  - [5.3 fsync(fd) Protocol](#53-fsyncfd-protocol)
  - [5.4 fsync(dirfd) Protocol](#54-fsyncdirfd-protocol)
  - [5.5 Non-Guaranteed Boundary (비보장 경계)](#55-non-guaranteed-boundary-비보장-경계)
- [6. FUSE Operations Design (FUSE 연산 설계)](#6-fuse-operations-design-fuse-연산-설계)
  - [6.1 Operation Table (연산 테이블)](#61-operation-table-연산-테이블)
  - [6.2 Error Code Mapping (에러 코드 매핑)](#62-error-code-mapping-에러-코드-매핑)
  - [6.3 UC-001 Flow: 파일 생성 및 쓰기](#63-uc-001-flow-파일-생성-및-쓰기)
- [7. Error Handling (에러 처리)](#7-error-handling-에러-처리)
- [8. Build Configuration (빌드 설정)](#8-build-configuration-빌드-설정)
  - [8.1 Source File Structure (소스 파일 구조)](#81-source-file-structure-소스-파일-구조)
  - [8.2 Compile-Time Constants (컴파일 타임 상수)](#82-compile-time-constants-컴파일-타임-상수)
  - [8.3 CMake Configuration](#83-cmake-configuration)
- [문서 개정 이력](#문서-개정-이력)

---

# 1. Introduction (개요)

## 1.1 Purpose (목적)

본 Software Design Description (SDD)는 FUSE 기반 UFFS 파일시스템의 설계를 기술한다. SRS(Software Requirements Specification)에 정의된 요구사항을 구현하기 위한 아키텍처, 컴포넌트, 데이터 구조 및 크래시 정합성 메커니즘을 설명한다.

**정의하는 제품:**
- **제품명**: FUSE 기반 UFFS 파일시스템
- **버전**: 1.0

## 1.2 Scope (범위)

본 SDD는 SRS v1.0에 정의된 기능 요구사항(FR-FILE-001~007, FR-DIR-001~004, FR-META-001)과 크래시 정합성 요구사항(NFR-REL-001)을 구현하는 설계를 다룬다.

**설계 범위:**
- FUSE 어댑터를 통한 POSIX 파일 API 구현
- UFFS 개념 기반 온디스크 레이아웃 및 태그 구조
- 2단계 쓰기(Two-phase Write) 기반 크래시 정합성 보장
- 마운트 시 스캔 기반 인메모리 트리 재구성

**설계 외 범위:**
- 배드 블록 관리, 웨어 레벨링, ECC
- 성능 최적화
- 동시성 지원 (전역 락으로 직렬화)

## 1.3 References (참조 문서)

1. **SRS**: FUSE 기반 UFFS 파일시스템 SRS v1.0 (`docs/SRS.md`)
2. **Test Plan**: FUSE 기반 UFFS 파일시스템 테스트 계획서 v0.3 (`docs/test-plan.md`)
3. **UFFS Reference Implementation**: `uffs-reference/` (설계 개념 참조)
4. **uffs-disk**: `uffs-disk/` (초기 구현 참조, 직접 재사용 안 함)
5. **IEEE 1016-2009**: Software Design Descriptions
6. **libfuse 3.18.1**: https://github.com/libfuse/libfuse

## 1.4 Definitions (용어 정의)

| 용어 | 정의 |
|-----|------|
| **Page** | 플래시 I/O의 최소 단위. 528바이트 (MiniHeader 4B + Data 512B + Tag 12B) |
| **Block** | 페이지의 집합. 32 페이지/블록. 씰(Seal) 단위 |
| **TagStore** | 페이지의 스페어 영역에 저장되는 8바이트 메타데이터 구조체 |
| **Tag** | TagStore(8B) + data_sum(2B) + seal_byte(1B) = 12B |
| **seal_byte** | 페이지 씰 상태 표시자. 0xFF=빈 페이지, 0x00=쓰기 중(Unseal), 0xFE=완료(Sealed) |
| **Serial** | 파일/디렉토리를 식별하는 14비트 고유 번호 |
| **Parent** | 파일/디렉토리가 속한 상위 디렉토리의 시리얼 번호 (10비트) |
| **File Header Block** | 파일의 메타데이터(UffsFileInfo)와 파일 데이터를 담는 블록. page 0 = 메타, pages 1~31 = 데이터 |
| **Data Block** | 파일 데이터 전용 블록. parent = 해당 파일의 serial |
| **Dir Block** | 디렉토리 메타데이터를 담는 블록. page 0 = UffsFileInfo |
| **Sealed** | seal_byte=0xFE인 페이지. 크래시 후에도 신뢰할 수 있는 상태 |
| **Unseal** | seal_byte=0x00인 페이지. 쓰기 진행 중이거나 크래시로 인해 미완료된 상태 |
| **Two-phase Write** | 데이터 기록(Unseal) → 씰 확정(Sealed) 순서로 크래시 정합성을 보장하는 쓰기 프로토콜 |
| **Global Lock** | FUSE 콜백 전체를 직렬화하는 단일 뮤텍스 |
| **TreeNode** | 인메모리 트리의 노드. dir/file/data 유니온 타입 |

---

# 2. System Architecture (시스템 아키텍처)

## 2.1 Architecture Overview (아키텍처 개요)

```
┌─────────────────────────────────────────────────────┐
│              Application Layer                       │
│         (cp, mv, cat, 사용자 프로세스)                │
└───────────────────────┬─────────────────────────────┘
                        │ POSIX API (open, read, write, fsync, ...)
┌───────────────────────▼─────────────────────────────┐
│           Linux VFS (Kernel Space)                   │
└───────────────────────┬─────────────────────────────┘
                        │ FUSE Protocol (/dev/fuse)
┌───────────────────────▼─────────────────────────────┐
│        FUSE 기반 UFFS 파일시스템 (Userspace)          │
│  ┌─────────────────────────────────────────────┐    │
│  │           FUSE Adapter (fuse_ops.c)          │    │
│  │     [Global Lock: pthread_mutex_t g_lock]    │    │
│  └──────┬──────────┬──────────┬────────────────┘    │
│         │          │          │                      │
│  ┌──────▼──┐ ┌─────▼──┐ ┌────▼────┐                │
│  │  File   │ │  Dir   │ │  Meta  │                  │
│  │ Manager │ │Manager │ │Manager │                  │
│  │(file.c) │ │(dir.c) │ │(meta.c)│                  │
│  └──────┬──┘ └─────┬──┘ └────┬────┘                │
│         └──────────┴─────────┘                      │
│                    │                                 │
│  ┌─────────────────▼─────────────────────────┐      │
│  │          Tree Manager (tree.c)             │      │
│  │  [dir_table / file_table / data_table]     │      │
│  └─────────────────┬─────────────────────────┘      │
│                    │                                 │
│  ┌─────────────────▼─────────────────────────┐      │
│  │          Flash I/O Layer (flash.c)         │      │
│  │    [readPage / writePage / sealPage]       │      │
│  └─────────────────┬─────────────────────────┘      │
└───────────────────────────────────────────────────── ┘
                        │ pread / pwrite (file descriptor)
┌───────────────────────▼─────────────────────────────┐
│      NAND Flash Device / Image File                  │
│             (Physical Storage)                       │
└─────────────────────────────────────────────────────┘
```

## 2.2 Component Overview (컴포넌트 개요)

| 컴포넌트 | 소스 파일 | 책임 | 대응 요구사항 |
|--------|---------|-----|------------|
| **FUSE Adapter** | `fuse_ops.c` | libfuse 콜백 구현, Global Lock 관리, 경로 파싱 | 전체 FR |
| **File Manager** | `file.c` | 파일 open/create/read/write/fsync/rename/unlink | FR-FILE-001~007 |
| **Directory Manager** | `dir.c` | 디렉토리 fsync/readdir/mkdir/rmdir | FR-DIR-001~004 |
| **Meta Manager** | `meta.c` | getattr (stat 반환) | FR-META-001 |
| **Tree Manager** | `tree.c` | 인메모리 트리 구축/조회, 마운트 스캔 | 전체 FR |
| **Flash I/O Layer** | `flash.c` | 페이지 읽기/쓰기/씰, fdatasync | NFR-REL-001 |
| **Main** | `main.c` | FUSE 초기화, 마운트 진입점 | EXT-UI-001~002 |
| **Utils** | `utils.c` | 경로 파싱, 시리얼 할당, 에러 변환 등 유틸리티 | - |

## 2.3 Concurrency Policy (동시성 정책)

동시성은 SRS §1.2 Out of Scope에 따라 지원하지 않는다. 모든 FUSE 콜백은 **전역 뮤텍스(g_lock)** 를 획득한 후 실행되며, FUSE 콜백 반환 전에 해제한다.

```c
// fuse_ops.c 패턴
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static int uffs_fuse_read(const char *path, char *buf, size_t size,
                          off_t offset, struct fuse_file_info *fi) {
    pthread_mutex_lock(&g_lock);
    int ret = file_read(path, buf, size, offset);
    pthread_mutex_unlock(&g_lock);
    return ret;
}
```

---

# 3. Component Design (컴포넌트 설계)

## 3.1 FUSE Adapter (FUSE 어댑터)

**책임**: libfuse 3.18.1의 `fuse_operations` 구조체를 구현하여 VFS 요청을 내부 컴포넌트로 라우팅한다.

**인터페이스 (`fuse_ops.h`)**:

```c
// FUSE operations 구조체 초기화
struct fuse_operations uffs_fuse_ops;
void uffs_fuse_ops_init(void);
```

**경로 파싱**: 모든 FUSE 콜백은 경로 문자열(`/dir/file`)을 받는다. `utils_path_split(path, &parent_name, &child_name)`으로 분리 후 Tree Manager에서 시리얼 번호를 조회한다.

**FUSE 연산 등록 목록**:

| fuse_operations 필드 | 구현 함수 | 대응 FR |
|--------------------|---------|--------|
| `.getattr` | `uffs_fuse_getattr` | FR-META-001 |
| `.open` | `uffs_fuse_open` | FR-FILE-001 |
| `.create` | `uffs_fuse_create` | FR-FILE-002 |
| `.read` | `uffs_fuse_read` | FR-FILE-003 |
| `.write` | `uffs_fuse_write` | FR-FILE-004 |
| `.fsync` | `uffs_fuse_fsync` | FR-FILE-005, FR-DIR-001 |
| `.rename` | `uffs_fuse_rename` | FR-FILE-006 |
| `.unlink` | `uffs_fuse_unlink` | FR-FILE-007 |
| `.readdir` | `uffs_fuse_readdir` | FR-DIR-002 |
| `.mkdir` | `uffs_fuse_mkdir` | FR-DIR-003 |
| `.rmdir` | `uffs_fuse_rmdir` | FR-DIR-004 |

## 3.2 File Manager (파일 관리자)

**책임**: 파일 CRUD 및 fsync 구현.

**주요 함수 시그니처**:

```c
int file_open(const char *path, struct fuse_file_info *fi);
int file_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int file_read(const char *path, char *buf, size_t size, off_t offset);
int file_write(const char *path, const char *buf, size_t size, off_t offset);
int file_fsync(const char *path, int isdatasync, struct fuse_file_info *fi);
int file_rename(const char *from, const char *to, unsigned int flags);
int file_unlink(const char *path);
```

**`file_write` 설계 (Unseal 단계)**:

```
file_write(path, buf, size, offset):
  1. tree_find_file(path) → file_node
  2. 대상 블록/페이지 계산 (offset / PAGE_DATA_SIZE)
  3. flash_write_page_unsealed(block_id, page_id, data)
     - seal_byte = 0x00 (쓰기 중)으로 기록
  4. 인메모리 파일 크기 업데이트 (file_node->size)
  5. return size
```

**`file_fsync` 설계 (Seal 단계)**:

→ §5.3 참조

**`file_create` 설계**:

```
file_create(path, mode):
  1. parent_serial = tree_find_dir(parent_path)
  2. new_serial = tree_alloc_serial()
  3. UffsFileInfo 초기화 (name, create_time, attr)
  4. flash_alloc_block() → file_header_block_id
  5. flash_write_page_unsealed(file_header_block_id, 0, &file_info)
  6. tree_insert_file(new_serial, parent_serial, file_header_block_id)
  7. return 0
```

## 3.3 Directory Manager (디렉토리 관리자)

**책임**: 디렉토리 생성/삭제/읽기 및 fsync 구현.

**주요 함수 시그니처**:

```c
int dir_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                off_t offset, struct fuse_file_info *fi,
                enum fuse_readdir_flags flags);
int dir_mkdir(const char *path, mode_t mode);
int dir_rmdir(const char *path);
int dir_fsync(const char *path, int isdatasync, struct fuse_file_info *fi);
```

**`dir_readdir` 설계**:

```
dir_readdir(path, buf, filler):
  1. dir_serial = tree_find_dir(path)
  2. tree_enumerate_children(dir_serial) → [child_nodes]
  3. for each child_node:
       filler(buf, child_node->name, NULL, 0, 0)
  4. return 0
```

**`dir_mkdir` 설계**:

```
dir_mkdir(path, mode):
  1. parent_serial = tree_find_dir(parent_path)
  2. new_serial = tree_alloc_serial()
  3. UffsFileInfo 초기화 (name, attr = FILE_ATTR_DIR)
  4. flash_alloc_block() → dir_block_id
  5. flash_write_page_unsealed(dir_block_id, 0, &dir_info)
  6. tree_insert_dir(new_serial, parent_serial, dir_block_id)
  7. return 0
```

**`dir_fsync` 설계**:

→ §5.4 참조

## 3.4 Meta Manager (메타데이터 관리자)

**책임**: `getattr` 구현. POSIX `struct stat` 반환.

**주요 함수**:

```c
int meta_getattr(const char *path, struct stat *stbuf,
                 struct fuse_file_info *fi);
```

**`meta_getattr` 설계**:

```
meta_getattr(path, stbuf):
  1. node = tree_find(path)  // 파일 또는 디렉토리
  2. if node is dir:
       stbuf->st_mode = S_IFDIR | 0755
       stbuf->st_nlink = 2
       stbuf->st_size = 0
  3. if node is file:
       stbuf->st_mode = S_IFREG | 0644
       stbuf->st_nlink = 1
       stbuf->st_size = node->size  // 인메모리 파일 크기
  4. stbuf->st_atime / mtime / ctime = node->timestamps
  5. return 0
```

## 3.5 Tree Manager (트리 관리자)

**책임**: 인메모리 파일시스템 트리 관리. 마운트 시 플래시 스캔으로 재구성.

**인메모리 인덱스 구조**:

```c
// tree.h
#define MAX_DIR_COUNT   32
#define MAX_FILE_COUNT  64
#define MAX_DATA_COUNT  512

typedef struct {
    u16 serial;         // 고유 시리얼 번호
    u16 parent;         // 부모 디렉토리 시리얼
    int block_id;       // 헤더 블록 ID
    char name[MAX_FILENAME_LEN];
    u32  size;          // 파일 크기 (file only)
    u32  create_time;
    u32  last_modify;
    u8   is_dirty;      // 마지막 fsync 이후 변경 여부
} TreeNode;

typedef struct {
    TreeNode dirs[MAX_DIR_COUNT];
    TreeNode files[MAX_FILE_COUNT];
    int      dir_count;
    int      file_count;
} UffsTree;

extern UffsTree g_tree;
```

**주요 함수**:

```c
int  tree_build(int flash_fd);          // 마운트 시 스캔으로 트리 구축
TreeNode* tree_find_file(const char *path);
TreeNode* tree_find_dir(const char *path);
u16  tree_alloc_serial(void);
int  tree_insert_file(u16 serial, u16 parent, int block_id, const char *name);
int  tree_insert_dir(u16 serial, u16 parent, int block_id, const char *name);
int  tree_remove_node(u16 serial);
void tree_enumerate_children(u16 parent_serial, TreeNode **out, int *count);
```

**`tree_build` 설계** (§5.2 참조):

```
tree_build(flash_fd):
  for each block in [1, TOTAL_BLOCKS]:
    tag = read_tag(block, page=0)
    if tag.seal_byte != SEAL_DONE: skip  // 씰되지 않은 블록 무시
    if tag.s.type == UFFS_TYPE_DIR:
      dir_info = read_page_data(block, 0)
      tree_insert_dir(tag.s.serial, tag.s.parent, block, dir_info.name)
    elif tag.s.type == UFFS_TYPE_FILE:
      file_info = read_page_data(block, 0)
      total_size = count_sealed_data_pages(tag.s.serial)
      tree_insert_file(tag.s.serial, tag.s.parent, block, file_info.name)
      files[serial].size = total_size
```

## 3.6 Flash I/O Layer (플래시 I/O 계층)

**책임**: 페이지 단위 플래시 읽기/쓰기 추상화. 씰 메커니즘 구현.

**주요 함수**:

```c
int flash_init(const char *device_path);    // 디바이스 파일 열기
int flash_format(void);                     // 초기 포맷 (마법 번호 기록)
int flash_format_check(void);               // 포맷 여부 확인

int flash_read_page(int block_id, int page_id,
                    uffs_MiniHeader *hdr, void *data, uffs_Tag *tag);
int flash_write_page_unsealed(int block_id, int page_id,
                              const void *data, size_t data_len,
                              const uffs_Tag *tag_template);
int flash_seal_page(int block_id, int page_id);
int flash_sync(void);                       // fdatasync(flash_fd)
int flash_alloc_block(int type, u16 serial, u16 parent);  // 빈 블록 할당
```

**`flash_write_page_unsealed` 설계**:

```
flash_write_page_unsealed(block_id, page_id, data, len, tag_tmpl):
  1. page_buf 구성:
     - MiniHeader: {status=0x01, reserved=0, crc=crc16(data)}
     - Data: data (512B, 나머지 0 패딩)
     - Tag: tag_tmpl 복사, seal_byte=0x00 (Unseal)
  2. pwrite(flash_fd, page_buf, PAGE_SIZE, offset)
  3. return 0 or -errno
```

**`flash_seal_page` 설계**:

```
flash_seal_page(block_id, page_id):
  1. offset = 블록/페이지 오프셋 + TAG 영역 + seal_byte 위치
  2. seal_val = SEAL_DONE (0xFE)
  3. pwrite(flash_fd, &seal_val, 1, seal_offset)
  4. return 0 or -errno
```

---

# 4. Data Design (데이터 설계)

## 4.1 On-Disk Layout (온디스크 레이아웃)

```
플래시 이미지 파일 (예: uffs.img)
│
├── Block 0  [Magic Block]
│   └── Page 0: "UFFS" 마법 번호 (512B 데이터 영역)
│
├── Block 1  [Root Directory Block]
│   └── Page 0: UffsFileInfo (name="/", attr=DIR, serial=0xFF, parent=0xFF)
│            seal_byte = SEAL_DONE (0xFE)
│
├── Block 2~N  [File/Dir/Data Blocks]
│   └── 각 블록의 Page 0의 TagStore.type으로 블록 용도 결정
│       - UFFS_TYPE_DIR  (1): 디렉토리 헤더 블록
│       - UFFS_TYPE_FILE (2): 파일 헤더 블록 (Page 0=메타, Pages 1~31=데이터)
│       - UFFS_TYPE_DATA (3): 파일 데이터 확장 블록
│
└── Block N+1~127  [Free Blocks]
    └── MiniHeader.status = 0xFF (빈 블록)
```

**디바이스 파라미터 (기본값)**:

| 파라미터 | 값 | 설명 |
|--------|---|-----|
| `PAGES_PER_BLOCK` | 32 | 블록당 페이지 수 |
| `PAGE_DATA_SIZE` | 512 B | 페이지 데이터 영역 크기 |
| `PAGE_SPARE_SIZE` | 16 B | 페이지 스페어(태그) 영역 크기 |
| `PAGE_SIZE` | 528 B | 전체 페이지 크기 |
| `TOTAL_BLOCKS` | 128 | 전체 블록 수 |
| `MAX_FILENAME_LEN` | 488 B | 최대 파일명 길이 (512 - 24) |

## 4.2 Page Layout (페이지 레이아웃)

```
┌──────────────────────────────────────────────────────────────┐
│                       Page (528 bytes)                        │
│                                                               │
│  ┌──────────────┬───────────────────────┬───────────────────┐ │
│  │ MiniHeader   │       Data            │       Tag         │ │
│  │   (4 bytes)  │     (512 bytes)       │    (12 bytes)     │ │
│  │              │                       │                   │ │
│  │ status  : 1B │ UffsFileInfo (헤더)   │ TagStore   : 8B   │ │
│  │ reserved: 1B │   또는                │ data_sum   : 2B   │ │
│  │ crc     : 2B │ 파일 데이터 (최대512B) │ seal_byte  : 1B   │ │
│  └──────────────┴───────────────────────┴───────────────────┘ │
└──────────────────────────────────────────────────────────────┘
```

**MiniHeader 필드**:

| 필드 | 크기 | 설명 |
|-----|-----|-----|
| `status` | 1B | 0xFF=새것(empty), 0x01=사용중, 그 외=dirty |
| `reserved` | 1B | 예약 |
| `crc` | 2B | 데이터 영역의 CRC16 |

## 4.3 Tag Structure (태그 구조)

**`uffs_TagStoreSt` (8 bytes, 비트필드)**:

```c
struct uffs_TagStoreSt {
    u32 dirty    : 1;   // 0=dirty, 1=clean (NAND 비트 반전 관례)
    u32 valid    : 1;   // 0=valid, 1=invalid
    u32 type     : 2;   // 블록 타입: DIR(1), FILE(2), DATA(3)
    u32 block_ts : 2;   // 블록 타임스탬프 (버전 관리용)
    u32 data_len : 12;  // 데이터 영역 유효 바이트 수 (0~512)
    u32 serial   : 14;  // 파일/디렉토리 고유 시리얼 번호
    u32 parent   : 10;  // 부모 디렉토리 시리얼 번호
    u32 page_id  : 6;   // 블록 내 페이지 인덱스 (0~31)
    u32 tag_ecc  : 12;  // 태그 ECC
};
```

**`uffs_TagsSt` (12 bytes, = Tag)**:

```c
struct uffs_TagsSt {
    struct uffs_TagStoreSt s;  // 8 bytes
    u16 data_sum;              // 파일명/데이터 체크섬
    u8  seal_byte;             // 씰 상태 (아래 참조)
    // 1B implicit padding
};
```

**seal_byte 값**:

| 값 | 상수명 | 의미 |
|---|-------|-----|
| `0xFF` | `SEAL_EMPTY` | 빈 페이지 (미사용) |
| `0x00` | `SEAL_WRITING` | 데이터 기록 완료, 아직 씰되지 않음 |
| `0xFE` | `SEAL_DONE` | 씰 완료, 크래시 후에도 신뢰 가능 |

## 4.4 Seal Mechanism (씰 메커니즘)

UFFS는 NAND 플래시의 특성(비트를 0→1로 되돌릴 수 없음)을 활용한다. `seal_byte`를 마지막에 별도로 기록함으로써 원자성을 보장한다.

```
쓰기 시퀀스:
  1. pwrite(data + tag with seal_byte=0x00)   ← Unseal 상태
  2. [크래시 가능 구간]
  3. pwrite(seal_byte=0xFE at tag offset)     ← Sealed 상태

복구 시:
  - seal_byte=0xFE  → 신뢰, 인메모리 트리에 포함
  - seal_byte=0x00  → 크래시로 인해 미완료, 무시
  - seal_byte=0xFF  → 빈 페이지, 무시
```

## 4.5 Block Header Structures (블록 헤더 구조체)

**`uffs_FileInfoSt` (Page 0의 Data 영역, 512B)**:

```c
struct uffs_FileInfoSt {
    u32  attr;               // FILE_ATTR_DIR(0x80) 또는 FILE_ATTR_WRITE(0x01)
    u32  create_time;        // UNIX timestamp
    u32  last_modify;        // UNIX timestamp
    u32  access;             // UNIX timestamp
    u32  reserved;
    u32  name_len;           // 파일명 길이 (바이트)
    char name[488];          // 파일명 (null-terminated)
};
// 총 24 + 488 = 512 bytes
```

**블록 타입별 레이아웃**:

| 블록 타입 | Page 0 | Pages 1~31 |
|--------|--------|-----------|
| DIR (type=1) | UffsFileInfo (dir 메타) | 미사용 |
| FILE (type=2) | UffsFileInfo (file 메타, 크기 포함) | 파일 데이터 (최대 31×512 = 15,872B) |
| DATA (type=3) | 파일 데이터 | 파일 데이터 (최대 32×512 = 16,384B) |

> **Note**: 15,872B 이상의 파일은 DATA 블록을 추가 할당한다 (parent=file_serial, type=DATA).

## 4.6 Serial Number Scheme (시리얼 번호 체계)

| 예약 시리얼 | 값 | 의미 |
|-----------|---|-----|
| `ROOT_DIR_SERIAL` | `0xFF` | 루트 디렉토리 (`/`) |

- 일반 파일/디렉토리: `1 ~ 0xFE` (254개)
- `tree_alloc_serial()`: 현재 사용 중이지 않은 가장 작은 번호를 할당
- 14비트 필드이므로 최대 시리얼: 16,383 (확장 가능)

## 4.7 In-Memory Data Structures (인메모리 데이터 구조)

```
g_tree (UffsTree)
│
├── dirs[32]
│   ├── [0] {serial=0xFF, parent=0xFF, name="/",  block_id=1}  ← 루트
│   ├── [1] {serial=1,    parent=0xFF, name="docs", block_id=5}
│   └── ...
│
└── files[64]
    ├── [0] {serial=2, parent=0xFF, name="README.md", block_id=3, size=1024}
    └── ...
```

---

# 5. Crash Consistency Design (크래시 정합성 설계)

## 5.1 Two-Phase Write Protocol (2단계 쓰기 프로토콜)

크래시 정합성은 **2단계 쓰기(Two-phase Write)** 프로토콜로 구현한다.

```
Phase 1 (Unseal Write):
  flash_write_page_unsealed(block_id, page_id, data)
  → seal_byte = 0x00으로 페이지 기록
  → 이 시점에 크래시가 발생하면 마운트 스캔에서 무시됨

Phase 2 (Seal Commit):
  flash_seal_page(block_id, page_id)
  → seal_byte = 0xFE로 갱신 (단일 바이트 기록)
  → 이 시점 이후 크래시가 발생해도 마운트 스캔에서 인식됨
```

**핵심 불변식**: seal_byte=0xFE인 페이지만 마운트 스캔에서 신뢰한다.

## 5.2 Mount Scan Recovery (마운트 스캔 복구)

마운트 시 `tree_build()`는 전체 블록을 선형 스캔한다.

```
for each block B in [1, TOTAL_BLOCKS-1]:
  tag = read_tag(B, page_id=0)
  if tag.seal_byte != SEAL_DONE (0xFE):
    continue  // Unseal, Empty 블록 무시
  file_info = read_data(B, page_id=0)
  switch tag.s.type:
    DIR:  tree_insert_dir(serial, parent, block_id, name)
    FILE: tree_insert_file(serial, parent, block_id, name)
          file.size = count_sealed_data_pages(serial) * PAGE_DATA_SIZE
    DATA: data_table[serial].add(block_id)
```

**복구 보장**: 씰되지 않은 페이지는 전혀 인식되지 않으므로, 부분 쓰기로 인한 불일치 상태가 트리에 반영되지 않는다.

## 5.3 fsync(fd) Protocol

`FR-FILE-005` 구현. 파일 데이터 및 파일 크기 내구성 보장.

```
file_fsync(path):
  1. node = tree_find_file(path)
  2. // Phase 1: 모든 Unseal 데이터 페이지를 씰
     for each unseal data page in node's blocks:
       flash_seal_page(block_id, page_id)
  3. // Phase 2: 파일 헤더(Page 0) 갱신 및 씰
     //   size, last_modify 업데이트된 UffsFileInfo 기록
     flash_write_page_unsealed(node->block_id, 0, &updated_file_info)
     flash_seal_page(node->block_id, 0)
  4. // Phase 3: 물리적 동기화
     flash_sync()  // fdatasync(flash_fd)
  5. node->is_dirty = 0
  6. return 0
```

**크래시 시나리오 분석**:

| 크래시 발생 시점 | 복구 후 관측 결과 | 요구사항 충족 |
|--------------|---------------|------------|
| 씰 전 (Phase 1 중) | 해당 데이터 페이지 무시, 이전 상태 | fsync 미완료로 간주, FR-FILE-005 해당 없음 |
| 데이터 씰 완료, 헤더 씰 전 | 데이터는 있으나 크기 불일치 가능 | fsync 미완료로 간주 |
| 헤더 씰 완료, fdatasync 전 | OS 버퍼에 있을 수 있음 | fdatasync 보장 필요 |
| fdatasync 완료 후 | 모든 데이터/크기 정상 복구 | FR-FILE-005 충족 |

## 5.4 fsync(dirfd) Protocol

`FR-DIR-001` 구현. 디렉토리 이름 매핑 내구성 보장.

```
dir_fsync(path):
  1. dir_node = tree_find_dir(path)
  2. // 해당 디렉토리의 모든 자식 헤더 블록을 씰
     children = tree_enumerate_children(dir_node->serial)
     for each child in children:
       if child->is_dirty:
         // 자식 파일/디렉토리 헤더 블록(Page 0) 씰
         flash_seal_page(child->block_id, page_id=0)
         child->is_dirty = 0
  3. // 디렉토리 자신의 헤더 씰 (last_modify 업데이트)
     flash_write_page_unsealed(dir_node->block_id, 0, &updated_dir_info)
     flash_seal_page(dir_node->block_id, 0)
  4. flash_sync()
  5. dir_node->is_dirty = 0
  6. return 0
```

## 5.5 Non-Guaranteed Boundary (비보장 경계)

SRS §FR-FILE-005-3 및 §FR-DIR-001-2에 따라:

| 시나리오 | 보장 여부 |
|--------|---------|
| fsync(fd) 성공 → 크래시 → 재마운트 후 파일 데이터 읽기 | **보장** (FR-FILE-005-1~2) |
| fsync(fd) 성공 → 크래시 → 재마운트 후 파일 경로 접근 | **비보장** (FR-FILE-005-3) |
| fsync(dirfd) 성공 → 크래시 → 재마운트 후 파일 경로 접근 | **보장** (FR-DIR-001-1) |
| fsync 없이 write → 크래시 → 재마운트 후 데이터 읽기 | **비보장** (FR-DIR-001-2) |

---

# 6. FUSE Operations Design (FUSE 연산 설계)

## 6.1 Operation Table (연산 테이블)

| FUSE 콜백 | 입력 | 출력 | 내부 흐름 |
|---------|-----|-----|---------|
| `getattr` | path | struct stat | meta_getattr → tree_find → stat 채우기 |
| `open` | path, fi | fi->fh | tree_find_file, 존재 확인 |
| `create` | path, mode, fi | 0 | file_create: serial 할당, 헤더 블록 기록(Unseal) |
| `read` | path, buf, size, offset | bytes_read | 페이지 오프셋 계산, flash_read_page, 복사 |
| `write` | path, buf, size, offset | bytes_written | 페이지 계산, flash_write_page_unsealed |
| `fsync` | path, isdatasync, fi | 0 | file_fsync 또는 dir_fsync (§5.3/5.4) |
| `rename` | from, to, flags | 0 | tree 업데이트, 헤더 블록 name 필드 재기록 |
| `unlink` | path | 0 | tree 제거, 블록 무효화 |
| `readdir` | path, buf, filler | 0 | tree_enumerate_children, filler 호출 |
| `mkdir` | path, mode | 0 | dir_mkdir: serial 할당, 헤더 블록 기록(Unseal) |
| `rmdir` | path | 0 | 자식 없음 확인, tree 제거, 블록 무효화 |

## 6.2 Error Code Mapping (에러 코드 매핑)

| 내부 오류 상황 | FUSE 반환값 |
|------------|-----------|
| 경로 미존재 | `-ENOENT` |
| 이미 존재하는 경로 | `-EEXIST` |
| 디렉토리가 아닌 경로에 디렉토리 연산 | `-ENOTDIR` |
| 비어있지 않은 디렉토리 삭제 | `-ENOTEMPTY` |
| 플래시 블록 부족 | `-ENOSPC` |
| 플래시 I/O 오류 | `-EIO` |
| 인수 오류 | `-EINVAL` |

## 6.3 UC-001 Flow: 파일 생성 및 쓰기

SRS §5.4 UC-001의 설계 수준 흐름.

```
사용자: open("/mnt/uffs/test.txt", O_CREAT|O_WRONLY)
  → VFS → FUSE create 콜백
  → [g_lock 획득]
  → file_create("/mnt/uffs/test.txt", 0644)
    1. parent_serial = 0xFF (루트)
    2. new_serial = tree_alloc_serial() = 1
    3. UffsFileInfo{name="test.txt", attr=FILE_ATTR_WRITE, ...}
    4. block_id = flash_alloc_block(FILE, serial=1, parent=0xFF) = 3
    5. flash_write_page_unsealed(3, 0, &file_info)  ← seal_byte=0x00
    6. tree_insert_file(serial=1, parent=0xFF, block_id=3, "test.txt")
  → [g_lock 해제]
  → return fd

사용자: write(fd, "Hello", 5)
  → VFS → FUSE write 콜백
  → [g_lock 획득]
  → file_write("/mnt/uffs/test.txt", "Hello", 5, offset=0)
    1. page_id = 0 / PAGE_DATA_SIZE = 0 → block 3, page 1 (Page 0은 헤더)
    2. flash_write_page_unsealed(3, 1, "Hello\0...", 5)  ← seal_byte=0x00
    3. g_tree.files[0].size = 5
  → [g_lock 해제]
  → return 5

사용자: fsync(fd)
  → VFS → FUSE fsync 콜백
  → [g_lock 획득]
  → file_fsync("/mnt/uffs/test.txt", ...)
    1. flash_seal_page(3, 1)          ← 데이터 씰 (seal_byte=0xFE)
    2. update file_info.last_modify, data_len
    3. flash_write_page_unsealed(3, 0, &updated_file_info)
    4. flash_seal_page(3, 0)          ← 헤더 씰 (seal_byte=0xFE)
    5. flash_sync()                   ← fdatasync()
  → [g_lock 해제]
  → return 0   ← 이 시점부터 내구성 보장 (FR-FILE-005)
```

---

# 7. Error Handling (에러 처리)

**원칙**: 모든 에러는 POSIX errno 값으로 변환하여 FUSE 계층으로 반환한다. 파일시스템 상태를 변경하는 연산은 실패 시 인메모리 트리를 원래 상태로 복원한다 (Best-effort rollback).

**에러 처리 계층**:

| 계층 | 에러 유형 | 처리 방법 |
|-----|---------|---------|
| Flash I/O | pread/pwrite 실패 | `-EIO` 반환, 로그 출력 |
| Tree Manager | 노드 미발견 | `-ENOENT` 반환 |
| Tree Manager | 용량 초과 | `-ENOSPC` 반환 |
| FUSE Adapter | Global Lock 내부 예외 | Lock 해제 후 음수 errno 반환 |

**로그 정책**: `fprintf(stderr, "[component] message\n")` 형식으로 표준 에러에 출력. 운영 중 디버깅을 위해 `-d` 플래그로 상세 로그 활성화.

---

# 8. Build Configuration (빌드 설정)

## 8.1 Source File Structure (소스 파일 구조)

```
fuse-uffs/
├── src/
│   ├── main.c          // FUSE 초기화, 마운트 진입점
│   ├── fuse_ops.c      // FUSE 콜백 구현, Global Lock
│   ├── fuse_ops.h
│   ├── file.c          // 파일 연산 구현
│   ├── file.h
│   ├── dir.c           // 디렉토리 연산 구현
│   ├── dir.h
│   ├── meta.c          // getattr 구현
│   ├── meta.h
│   ├── tree.c          // 인메모리 트리 관리
│   ├── tree.h
│   ├── flash.c         // 플래시 I/O, 씰 메커니즘
│   ├── flash.h
│   ├── utils.c         // 경로 파싱, 시리얼 할당 등
│   └── utils.h
├── tests/
│   ├── unit/           // Google Test 단위 테스트
│   └── crash/          // 크래시 정합성 테스트
├── docs/
│   ├── SRS.md
│   ├── SDD.md          // 본 문서
│   └── test-plan.md
└── CMakeLists.txt
```

## 8.2 Compile-Time Constants (컴파일 타임 상수)

```c
// flash.h
#define PAGES_PER_BLOCK     32
#define PAGE_DATA_SIZE      512
#define PAGE_SPARE_SIZE     16
#define PAGE_SIZE           528       // PAGES_DATA_SIZE + SPARE
#define TOTAL_BLOCKS        128
#define MAX_FILENAME_LEN    488       // PAGE_DATA_SIZE - sizeof(uffs_FileInfoSt 고정부)

// Seal byte values
#define SEAL_EMPTY          0xFF
#define SEAL_WRITING        0x00
#define SEAL_DONE           0xFE

// Block types (uffs_TagStoreSt.type)
#define UFFS_TYPE_DIR       1
#define UFFS_TYPE_FILE      2
#define UFFS_TYPE_DATA      3

// Special serials
#define ROOT_DIR_SERIAL     0xFF

// tree.h
#define MAX_DIR_COUNT       32
#define MAX_FILE_COUNT      64
```

## 8.3 CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.16)
project(fuse_uffs C)

set(CMAKE_C_STANDARD 11)

# libfuse 3.x 탐색
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED fuse3)

# 소스 파일
set(SOURCES
    src/main.c
    src/fuse_ops.c
    src/file.c
    src/dir.c
    src/meta.c
    src/tree.c
    src/flash.c
    src/utils.c
)

add_executable(fuse_uffs ${SOURCES})

target_include_directories(fuse_uffs PRIVATE ${FUSE_INCLUDE_DIRS} src/)
target_compile_options(fuse_uffs PRIVATE ${FUSE_CFLAGS_OTHER})
target_link_libraries(fuse_uffs PRIVATE ${FUSE_LIBRARIES} pthread)

# 테스트
enable_testing()
add_subdirectory(tests)
```

---


# 문서 개정 이력

| 버전 | 날짜 | 작성자 | 변경 내용 |
|-----|-----|------|---------|
| 1.0 | 2026.03.04 | 임재형 | 초안 작성 (IEEE 1016-2009 기반) |
