# FUSE 기반 UFFS 파일시스템
## Requirements Traceability Matrix (RTM)

**문서 표준**: ISO/IEC/IEEE 29148 § 5.5
**버전**: 1.0
**작성일**: 2026.03.04
**작성자**: 임재형
**참조 SRS**: SRS v1.0 (2026.03.04)
**참조 SDD**: SDD v1.0 (2026.03.04)
**참조 Test Plan**: TP-UFFS-001 v0.1 (2026.03.04)

---

## 목차

- [FUSE 기반 UFFS 파일시스템](#fuse-기반-uffs-파일시스템)
  - [Requirements Traceability Matrix (RTM)](#requirements-traceability-matrix-rtm)
  - [목차](#목차)
  - [1. 개요](#1-개요)
  - [2. 추적성 매트릭스 (순방향)](#2-추적성-매트릭스-순방향)
    - [2.1 기능 요구사항 — 파일 연산](#21-기능-요구사항--파일-연산)
    - [2.2 기능 요구사항 — 디렉토리 연산](#22-기능-요구사항--디렉토리-연산)
    - [2.3 기능 요구사항 — 메타데이터](#23-기능-요구사항--메타데이터)
    - [2.4 비기능 요구사항 — 신뢰성](#24-비기능-요구사항--신뢰성)
    - [2.5 외부 인터페이스 요구사항](#25-외부-인터페이스-요구사항)
    - [2.6 제약사항 및 지원 요구사항](#26-제약사항-및-지원-요구사항)
  - [3. 크래시 정합성 세부 추적](#3-크래시-정합성-세부-추적)
    - [3.1 검증 방법 ↔ 요구사항 매핑](#31-검증-방법--요구사항-매핑)
    - [3.2 Two-Phase Write 프로토콜 ↔ 요구사항](#32-two-phase-write-프로토콜--요구사항)
    - [3.3 크래시 시나리오 ↔ 요구사항 ↔ 테스트 케이스](#33-크래시-시나리오--요구사항--테스트-케이스)
  - [4. 설계 요소 → 요구사항 역방향 추적](#4-설계-요소--요구사항-역방향-추적)
  - [5. 테스트 케이스 → 요구사항 역방향 추적](#5-테스트-케이스--요구사항-역방향-추적)
  - [6. 인수 기준 추적](#6-인수-기준-추적)
  - [7. 커버리지 요약](#7-커버리지-요약)
    - [7.1 요구사항 커버리지](#71-요구사항-커버리지)
    - [7.2 테스트 케이스 커버리지](#72-테스트-케이스-커버리지)
    - [7.3 미커버 요구사항](#73-미커버-요구사항)
  - [문서 개정 이력](#문서-개정-이력)

---

## 1. 개요

본 RTM은 SRS에 정의된 요구사항이 SDD 설계 요소에 반영되고, 테스트 계획서의 테스트 케이스로 검증됨을 추적한다.

**추적 체계:**

```
SRS 요구사항 → SDD 설계 요소(컴포넌트/함수) → Test Plan 테스트 케이스 → 인수 기준
```

**상태 값 정의:**

| 상태 | 의미 |
|-----|-----|
| `TBD` | 구현 전 (To Be Determined) |
| `In Progress` | 구현 중 |
| `Implemented` | 구현 완료 |
| `Verified` | 테스트 검증 완료 |

---

## 2. 추적성 매트릭스 (순방향)

### 2.1 기능 요구사항 — 파일 연산

| 요구사항 ID | 요구사항 이름 | 우선순위 | SDD 컴포넌트 | SDD 설계 함수 | 테스트 케이스 ID | 테스트 유형 | 인수 기준 | 상태 |
|-----------|-------------|---------|------------|-------------|-----------------|-----------|---------|------|
| **FR-FILE-001** | 파일 열기 | Critical | FUSE Adapter<br>File Manager | `uffs_fuse_open`<br>`file_open` | TC-FILE-001 | Integration Test | AC-001 | TBD |
| **FR-FILE-002** | 파일 생성 | Critical | FUSE Adapter<br>File Manager<br>Tree Manager<br>Flash I/O | `uffs_fuse_create`<br>`file_create`<br>`tree_alloc_serial`<br>`flash_write_page_unsealed` | TC-FILE-002 | Integration Test | AC-001 | TBD |
| **FR-FILE-003** | 파일 읽기 | Critical | FUSE Adapter<br>File Manager<br>Flash I/O | `uffs_fuse_read`<br>`file_read`<br>`flash_read_page` | TC-FILE-003 | Integration Test | AC-001 | TBD |
| **FR-FILE-004** | 파일 쓰기 | Critical | FUSE Adapter<br>File Manager<br>Flash I/O | `uffs_fuse_write`<br>`file_write`<br>`flash_write_page_unsealed` | TC-FILE-004 | Integration Test | AC-001 | TBD |
| **FR-FILE-005** | 파일 동기화 (fsync) | Critical | FUSE Adapter<br>File Manager<br>Flash I/O | `uffs_fuse_fsync`<br>`file_fsync`<br>`flash_seal_page`<br>`flash_sync` | TC-CRASH-005 | Crash Test | AC-001<br>AC-002 | TBD |
| FR-FILE-005-1 | 파일 데이터 내구성 | Critical | Flash I/O<br>File Manager | `flash_seal_page`<br>`flash_sync` | TC-CRASH-005 | Crash Test | AC-002 | TBD |
| FR-FILE-005-2 | 파일 사이즈 내구성 | Critical | File Manager<br>Meta Manager | `file_fsync`<br>`meta_getattr` | TC-CRASH-005 | Crash Test | AC-002 | TBD |
| FR-FILE-005-3 | 내구성 보장의 경계 (비보장) | Critical | — | — | TC-CRASH-NEG-001 | Negative Test | (판정 없음) | TBD |
| **FR-FILE-006** | 파일 이름 변경 (rename) | High | FUSE Adapter<br>File Manager<br>Tree Manager | `uffs_fuse_rename`<br>`file_rename`<br>`tree_find_file` | TC-FILE-006 | Integration Test | AC-001 | TBD |
| **FR-FILE-007** | 파일 삭제 | Medium | FUSE Adapter<br>File Manager<br>Tree Manager | `uffs_fuse_unlink`<br>`file_unlink`<br>`tree_remove_node` | TC-FILE-007 | Integration Test | AC-001 | TBD |

---

### 2.2 기능 요구사항 — 디렉토리 연산

| 요구사항 ID | 요구사항 이름 | 우선순위 | SDD 컴포넌트 | SDD 설계 함수 | 테스트 케이스 ID | 테스트 유형 | 인수 기준 | 상태 |
|-----------|-------------|---------|------------|-------------|-----------------|-----------|---------|------|
| **FR-DIR-001** | 디렉토리 동기화 (fsync) | High | FUSE Adapter<br>Directory Manager<br>Flash I/O | `uffs_fuse_fsync`<br>`dir_fsync`<br>`flash_seal_page`<br>`flash_sync` | TC-CRASH-DIR-001 | Crash Test | AC-001<br>AC-002 | TBD |
| FR-DIR-001-1 | 이름 매핑 변화 내구성 | High | Directory Manager<br>Tree Manager | `dir_fsync`<br>`tree_enumerate_children` | TC-CRASH-DIR-001 | Crash Test | AC-002 | TBD |
| FR-DIR-001-2 | fsync 없는 범위 비보장 | High | — | — | TC-CRASH-NEG-001 | Negative Test | (판정 없음) | TBD |
| **FR-DIR-002** | 디렉토리 읽기 | High | FUSE Adapter<br>Directory Manager<br>Tree Manager | `uffs_fuse_readdir`<br>`dir_readdir`<br>`tree_enumerate_children` | TC-DIR-002 | Integration Test | AC-001 | TBD |
| **FR-DIR-003** | 디렉토리 생성 | High | FUSE Adapter<br>Directory Manager<br>Tree Manager<br>Flash I/O | `uffs_fuse_mkdir`<br>`dir_mkdir`<br>`tree_alloc_serial`<br>`flash_write_page_unsealed` | TC-DIR-003 | Integration Test | AC-001 | TBD |
| **FR-DIR-004** | 디렉토리 삭제 | Medium | FUSE Adapter<br>Directory Manager<br>Tree Manager | `uffs_fuse_rmdir`<br>`dir_rmdir`<br>`tree_remove_node` | TC-DIR-004 | Integration Test | AC-001 | TBD |

---

### 2.3 기능 요구사항 — 메타데이터

| 요구사항 ID | 요구사항 이름 | 우선순위 | SDD 컴포넌트 | SDD 설계 함수 | 테스트 케이스 ID | 테스트 유형 | 인수 기준 | 상태 |
|-----------|-------------|---------|------------|-------------|-----------------|-----------|---------|------|
| **FR-META-001** | 메타데이터 조회 | Medium | FUSE Adapter<br>Meta Manager<br>Tree Manager | `uffs_fuse_getattr`<br>`meta_getattr`<br>`tree_find` | TC-META-001 | Integration Test | AC-001 | TBD |

---

### 2.4 비기능 요구사항 — 신뢰성

| 요구사항 ID | 요구사항 이름 | 우선순위 | SDD 컴포넌트 | SDD 설계 요소 | 테스트 케이스 ID | 테스트 유형 | 인수 기준 | 상태 |
|-----------|-------------|---------|------------|-------------|-----------------|-----------|---------|------|
| **NFR-REL-001** | fsync 이후 데이터 손실 방지 | Critical | Flash I/O Layer<br>File Manager<br>Directory Manager | §5.1 Two-Phase Write Protocol<br>`flash_seal_page`<br>`flash_sync` (fdatasync) | TC-REL-001<br>TC-CRASH-005 | Durability Test<br>Crash Test | AC-002 | TBD |

---

### 2.5 외부 인터페이스 요구사항

| 요구사항 ID | 요구사항 이름 | 우선순위 | SDD 컴포넌트 | SDD 설계 요소 | 테스트 케이스 ID | 검증 방법 | 인수 기준 | 상태 |
|-----------|-------------|---------|------------|-------------|-----------------|---------|---------|------|
| **EXT-UI-001** | 마운트 명령어 | Critical | Main | `main.c` FUSE 초기화<br>`uffs_fuse_ops_init` | (진입 기준) | 마운트 성공 확인 | AC-003 | TBD |
| **EXT-UI-002** | 언마운트 명령어 | Critical | Main | `main.c` FUSE 종료 처리 | (진입 기준) | `umount` 명령 성공 | AC-003 | TBD |
| **EXT-SW-001** | FUSE API (libfuse 3.18.1) | Critical | FUSE Adapter | `fuse_operations` 구조체<br>`fuse_main()` | (전 TC 공통) | 전체 TC 실행 | AC-001 | TBD |

---

### 2.6 제약사항 및 지원 요구사항

| 요구사항 ID | 요구사항 이름 | 우선순위 | SDD 설계 요소 | 테스트 케이스 ID | 검증 방법 | 인수 기준 | 상태 |
|-----------|-------------|---------|------------|-----------------|---------|---------|------|
| **OTH-LEG-001** | 오픈소스 라이선스 | Critical | — (법적 요건) | — | 라이선스 파일 검토 | AC-003 | TBD |
| **OTH-INST-001** | 빌드 시스템 (CMake) | High | §8.3 CMakeLists.txt | (진입 기준) | `make` 성공 여부 | AC-003 | TBD |

---

## 3. 크래시 정합성 세부 추적

크래시 정합성은 본 프로젝트의 핵심 품질 목표이므로 별도로 세분화하여 추적한다.

### 3.1 검증 방법 ↔ 요구사항 매핑

| 검증 방법 ID | 검증 방법 이름 | 대상 요구사항 | 테스트 케이스 |
|-----------|-------------|------------|------------|
| VER-001 | 크래시 정합성 검증 | FR-FILE-005, FR-DIR-001, NFR-REL-001 | TC-CRASH-005, TC-CRASH-DIR-001, TC-REL-001 |
| VER-ORACLE-001 | FUSE read 기반 검증 오라클 | FR-FILE-004, FR-FILE-005 | TC-CRASH-005, TC-REL-001 |

### 3.2 Two-Phase Write 프로토콜 ↔ 요구사항

| SDD 설계 요소 | 설계 섹션 | 충족 요구사항 |
|------------|---------|------------|
| Two-Phase Write Protocol | §5.1 | NFR-REL-001, FR-FILE-005 |
| Mount Scan Recovery (`tree_build`) | §5.2 | NFR-REL-001, FR-FILE-005, FR-DIR-001 |
| `file_fsync` 프로토콜 | §5.3 | FR-FILE-005-1, FR-FILE-005-2 |
| `dir_fsync` 프로토콜 | §5.4 | FR-DIR-001-1 |
| 비보장 경계 정의 | §5.5 | FR-FILE-005-3, FR-DIR-001-2 |

### 3.3 크래시 시나리오 ↔ 요구사항 ↔ 테스트 케이스

| 크래시 시나리오 | 관련 요구사항 | 테스트 케이스 | 판정 기준 |
|-------------|------------|------------|---------|
| fsync(fd) 완료 후 kill -9 | FR-FILE-005-1, FR-FILE-005-2 | TC-CRASH-005 (절차 A) | 데이터 손실 0건, st_size 일치 |
| fsync(fd) 완료 후 VM 강제 종료 | FR-FILE-005-1, FR-FILE-005-2 | TC-CRASH-005 (절차 B) | 데이터 손실 0건, st_size 일치 |
| fsync(dirfd) 완료 후 kill -9 | FR-DIR-001-1 | TC-CRASH-DIR-001 | 디렉토리 엔트리 정상 관측 |
| fsync 없이 kill -9 (비보장) | FR-FILE-005-3, FR-DIR-001-2 | TC-CRASH-NEG-001 | 판정 없음 (관측만) |
| 1,000회 반복 power-cut | NFR-REL-001 | TC-REL-001 | 전 회차 데이터 손실 0건 |

---

## 4. 설계 요소 → 요구사항 역방향 추적

| SDD 컴포넌트 | 소스 파일 | 충족 요구사항 |
|------------|---------|------------|
| **FUSE Adapter** | `fuse_ops.c` | EXT-SW-001, FR-FILE-001~007, FR-DIR-001~004, FR-META-001 (전체) |
| **File Manager** | `file.c` | FR-FILE-001, FR-FILE-002, FR-FILE-003, FR-FILE-004, FR-FILE-005, FR-FILE-006, FR-FILE-007 |
| **Directory Manager** | `dir.c` | FR-DIR-001, FR-DIR-002, FR-DIR-003, FR-DIR-004 |
| **Meta Manager** | `meta.c` | FR-META-001 |
| **Tree Manager** | `tree.c` | FR-FILE-001~007, FR-DIR-001~004, FR-META-001 (공통 의존) |
| **Flash I/O Layer** | `flash.c` | NFR-REL-001, FR-FILE-005 (seal), FR-DIR-001 (seal) |
| **Main** | `main.c` | EXT-UI-001, EXT-UI-002 |
| **Utils** | `utils.c` | — (공통 유틸) |
| **CMakeLists.txt** | `CMakeLists.txt` | OTH-INST-001 |

---

## 5. 테스트 케이스 → 요구사항 역방향 추적

| 테스트 케이스 ID | 테스트 이름 | 테스트 유형 | 검증 요구사항 |
|---------------|-----------|-----------|------------|
| **TC-FILE-001** | 파일 열기 | Integration Test | FR-FILE-001 |
| **TC-FILE-002** | 파일 생성 | Integration Test | FR-FILE-002 |
| **TC-FILE-003** | 파일 읽기 | Integration Test | FR-FILE-003 |
| **TC-FILE-004** | 파일 쓰기 | Integration Test | FR-FILE-004 |
| **TC-CRASH-005** | fsync(fd) 후 크래시 내구성 | Crash Test | FR-FILE-005, FR-FILE-005-1, FR-FILE-005-2, NFR-REL-001 |
| **TC-CRASH-NEG-001** | fsync 이전 크래시 (비보장) | Negative Test | FR-FILE-005-3, FR-DIR-001-2 |
| **TC-FILE-006** | 파일 이름 변경 | Integration Test | FR-FILE-006 |
| **TC-FILE-007** | 파일 삭제 | Integration Test | FR-FILE-007 |
| **TC-CRASH-DIR-001** | fsync(dirfd) 후 크래시 내구성 | Crash Test | FR-DIR-001, FR-DIR-001-1 |
| **TC-DIR-002** | 디렉토리 읽기 | Integration Test | FR-DIR-002 |
| **TC-DIR-003** | 디렉토리 생성 | Integration Test | FR-DIR-003 |
| **TC-DIR-004** | 디렉토리 삭제 | Integration Test | FR-DIR-004 |
| **TC-META-001** | 메타데이터 조회 | Integration Test | FR-META-001 |
| **TC-REL-001** | fsync 내구성 반복 테스트 (1,000회) | Durability Test | NFR-REL-001 |

---

## 6. 인수 기준 추적

| 인수 기준 ID | 인수 기준 이름 | 연계 요구사항 | 연계 테스트 케이스 | 판정 조건 |
|-----------|-------------|------------|-----------------|---------|
| **AC-001** | 파일 연산 인수 기준 | FR-FILE-001~007, FR-DIR-001~004, FR-META-001 | TC-FILE-001~007, TC-CRASH-005, TC-CRASH-DIR-001, TC-DIR-002~004, TC-META-001 | 모든 TC Pass |
| **AC-002** | 크래시 정합성 인수 기준 | FR-FILE-005, FR-DIR-001, NFR-REL-001 | TC-CRASH-005, TC-CRASH-DIR-001, TC-REL-001 | 데이터 손실 0건, 메타데이터 일관성 100%, 1,000회 중 손실 0건 |
| **AC-003** | 릴리즈 준비 완료 | OTH-LEG-001, OTH-INST-001, EXT-UI-001~002 | AC-001 · AC-002 통과, 문서화 완료, 라이선스 검토 | 모든 하위 기준 통과 |

---

## 7. 커버리지 요약

### 7.1 요구사항 커버리지

| 구분 | 전체 요구사항 수 | 테스트 케이스 연결 | SDD 설계 반영 | 커버리지 |
|-----|--------------|-----------------|------------|--------|
| 기능 요구사항 (FR) | 12 개 (+ 5 서브) | 12 개 | 12 개 | 100% |
| 비기능 요구사항 (NFR) | 1 개 | 1 개 | 1 개 | 100% |
| 외부 인터페이스 (EXT) | 3 개 | 3 개 (공통/진입기준) | 3 개 | 100% |
| 제약/지원 (OTH) | 2 개 | 2 개 (진입기준/검토) | 1 개 (OTH-INST-001) | 100% |

### 7.2 테스트 케이스 커버리지

| 테스트 유형 | TC 수 | 커버 요구사항 수 |
|-----------|-------|--------------|
| Integration Test | 10 | 10 (FR-FILE-001~004, 006~007, FR-DIR-002~004, FR-META-001) |
| Crash Test | 2 | 2 (FR-FILE-005, FR-DIR-001) |
| Durability Test | 1 | 1 (NFR-REL-001) |
| Negative Test | 1 | 2 (FR-FILE-005-3, FR-DIR-001-2) |
| **합계** | **14** | **15** |

### 7.3 미커버 요구사항

현재 모든 요구사항이 설계 요소 및 테스트 케이스에 매핑되어 있다.

> **Note**: TC-CRASH-NEG-001은 비보장 경계(FR-FILE-005-3, FR-DIR-001-2)를 검증하는 음성 테스트로, 합격/불합격 판정 대상이 아니며 관측 결과를 문서화한다.

---

## 문서 개정 이력

| 버전 | 날짜 | 작성자 | 변경 내용 |
|-----|------|--------|----------|
| 1.0 | 2026.03.04 | 임재형 | 초안 작성 (SRS v1.0, SDD v1.0, TP-UFFS-001 v0.1 기반) |

---

**문서 끝 (End of RTM)**
