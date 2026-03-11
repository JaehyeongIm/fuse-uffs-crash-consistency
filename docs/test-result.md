# FUSE 기반 UFFS 파일시스템 — Test Result Report

**문서 표준**: ISO/IEC/IEEE 29119-3
**Test Report ID**: TR-UFFS-001
**Reference Test Plan**: TP-UFFS-001 (test-plan.md)
**Reference SRS**: SRS v1.2
**Version**: 1.0
**Date**: 2026-03-11
**Status**: Final
**작성자**: 임재형

---

# 1. 요약 (Executive Summary)

2026-03-11에 FUSE 기반 UFFS 파일시스템에 대한 통합 테스트, Crash Test, Durability Test, 음성 테스트를 수행하였다.
계획된 12개 TC 전체가 실행되었으며, 판정 대상 11개가 모두 통과하였다.
핵심 품질 목표인 **Crash Consistency**는 파일(1MB·4KB)과 디렉토리 엔트리 양방향으로 검증되었고,
**NFR-REL-001** (1,000회 반복 power-cut 시 데이터 손실 0건)이 완전히 충족되었다.
TC-CRASH-NEG-001(음성 테스트)에서는 fsync 미호출 크래시 시 파일이 복구되지 않음(size=0)이 관측되어
SRS FR-FILE-005-3의 비보장 경계가 문서화되었다.

| 항목 | 결과 |
|------|------|
| 계획 TC 수 | 12개 |
| 실행 TC 수 | **12 / 12 (100%)** |
| 통과 (판정 대상) | **11 / 11 (100%)** |
| 실패 | 0 |
| 관측 전용 (판정 없음) | 1 (TC-CRASH-NEG-001) |
| 데이터 손실 (1,000회 반복) | **0건 (0%)** |
| NFR-REL-001 인수 기준 | **충족** |

---

# 2. 테스트 환경 실제 구성

## 2.1 하드웨어 / OS

| 항목 | 실제 구성 |
|------|---------|
| 호스트명 | `posinpc` |
| OS | Ubuntu Linux (ARM64) |
| 실행 사용자 | root |
| 플래시 이미지 | `/tmp/flash.img`, `/tmp/uffs_tcdir001.img` (파일 기반 NAND 에뮬레이션) |
| 마운트 포인트 | `/mnt/uffs`, `/tmp/uffs_tcdir001_mnt` |

## 2.2 소프트웨어

| 소프트웨어 | 비고 |
|---------|------|
| UFFS 바이너리 | `./build/uffs` (CMake Debug 빌드) |
| FUSE | FUSE 3 (libfuse3) |
| 테스트 프레임워크 | Google Test (GTest) |
| 크래시 시뮬레이션 | `kill -9` (프로세스 강제 종료) |
| 데이터 검증 | `sha256sum` |
| 디렉토리 검증 | Python3 `os.listdir()`, `os.open()` |

---

# 3. 테스트 실행 요약

## 3.1 실행 일시

| TC 그룹 | 실행 시각 |
|--------|---------|
| Integration Test (test_integration) | 2026-03-11 오전 (총 28 ms) |
| TC-CRASH-005 (1MB) | 2026-03-11 오전 |
| TC-CRASH-005 (4KB) | 2026-03-11 오전 |
| TC-REL-001 (50회) | 2026-03-11 09:20:52 ~ 09:21:04 |
| TC-REL-001 (100회) | 2026-03-11 09:21:16 ~ 09:21:41 |
| TC-REL-001 (500회) | 2026-03-11 09:22:05 ~ 09:24:20 |
| TC-REL-001 (1,000회) | 2026-03-11 09:24:36 ~ 09:28:48 |
| TC-CRASH-DIR-001 | 2026-03-11 오전 |
| TC-CRASH-NEG-001 | 2026-03-11 오전 |

## 3.2 TC별 판정 요약

| TC ID | TC 이름 | 관련 요구사항 | 판정 | 비고 |
|-------|--------|------------|------|------|
| TC-FILE-001 | 파일 열기 | FR-FILE-001 | **PASS** | 2 ms |
| TC-FILE-002 | 파일 생성 | FR-FILE-002 | **PASS** | 1 ms |
| TC-FILE-003 | 파일 읽기 | FR-FILE-003 | **PASS** | 8 ms |
| TC-FILE-004 | 파일 쓰기 | FR-FILE-004 | **PASS** | 5 ms |
| TC-FILE-006 | 파일 이름 변경 | FR-FILE-006 | **PASS** | 2 ms |
| TC-FILE-007 | 파일 삭제 | FR-FILE-007 | **PASS** | 0 ms |
| TC-DIR-002 | 디렉토리 읽기 | FR-DIR-002 | **PASS** | 2 ms |
| TC-DIR-003 | 디렉토리 생성 | FR-DIR-003 | **PASS** | 2 ms |
| TC-DIR-004 | 디렉토리 삭제 | FR-DIR-004 | **PASS** | 1 ms |
| TC-META-001 | 메타데이터 조회 | FR-META-001 | **PASS** | 4 ms |
| TC-CRASH-005 | fsync 후 크래시 내구성 (파일) | FR-FILE-005 | **PASS** | 1MB + 4KB 모두 통과 |
| TC-REL-001 | fsync 내구성 반복 | NFR-REL-001 | **PASS** | 1,000회 손실 0건 |
| TC-CRASH-DIR-001 | fsync 후 크래시 내구성 (디렉토리) | FR-DIR-001 | **PASS** | 엔트리 보존 확인 |
| TC-CRASH-NEG-001 | fsync 이전 크래시 (비보장) | FR-FILE-005-3 | **관측 완료** | 판정 대상 아님 |

---

# 4. 개별 TC 실행 결과

## 4.1 Integration Test — TC-FILE-001 ~ TC-META-001

**실행 명령어**
```
./build/uffs /tmp/flash.img /mnt/uffs &
UFFS_MOUNT_PATH=/mnt/uffs ./build/tests/test_integration
```

**실행 로그**
```
[==========] Running 10 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 10 tests from UffsTest
[ RUN      ] UffsTest.TC_FILE_001_Open
[       OK ] UffsTest.TC_FILE_001_Open (2 ms)
[ RUN      ] UffsTest.TC_FILE_002_Create
[       OK ] UffsTest.TC_FILE_002_Create (1 ms)
[ RUN      ] UffsTest.TC_FILE_003_Read
[       OK ] UffsTest.TC_FILE_003_Read (8 ms)
[ RUN      ] UffsTest.TC_FILE_004_Write
[       OK ] UffsTest.TC_FILE_004_Write (5 ms)
[ RUN      ] UffsTest.TC_FILE_006_Rename
[       OK ] UffsTest.TC_FILE_006_Rename (2 ms)
[ RUN      ] UffsTest.TC_FILE_007_Unlink
[       OK ] UffsTest.TC_FILE_007_Unlink (0 ms)
[ RUN      ] UffsTest.TC_DIR_002_Readdir
[       OK ] UffsTest.TC_DIR_002_Readdir (2 ms)
[ RUN      ] UffsTest.TC_DIR_003_Mkdir
[       OK ] UffsTest.TC_DIR_003_Mkdir (2 ms)
[ RUN      ] UffsTest.TC_DIR_004_Rmdir
[       OK ] UffsTest.TC_DIR_004_Rmdir (1 ms)
[ RUN      ] UffsTest.TC_META_001_GetAttr
[       OK ] UffsTest.TC_META_001_GetAttr (4 ms)
[----------] 10 tests from UffsTest (28 ms total)

[----------] Global test environment tear-down
[==========] 10 tests from 1 test suite ran. (28 ms total)
[  PASSED  ] 10 tests.
```

**판정**: PASS — 10 / 10 통과 (28 ms)

---

## 4.2 TC-CRASH-005 — fsync(fd) 후 크래시 내구성

### 시나리오 A — 1MB 파일

**실행 명령어**
```
UFFS_BIN=../uffs UFFS_DEVICE=/tmp/flash.img UFFS_MOUNT=/mnt/uffs \
  bash tc_crash_005.sh
```

**실행 로그**
```
[INFO] TC-CRASH-005 시작: FILE_SIZE=1048576B
[INFO] Step 1: 마운트
[INFO] Step 2: 1048576B 데이터 write + fsync
[INFO] 원본 SHA256: 2a82d04d4b094619919d039e575ace0ea907ea607783bd91d1f202353ab3c952
[INFO] Step 3: kill -9 (크래시 시뮬레이션)
[INFO] Step 4: 마운트 해제
[INFO] Step 5: 재마운트
[INFO] Step 6: 데이터 검증
[INFO] 복구 SHA256: 2a82d04d4b094619919d039e575ace0ea907ea607783bd91d1f202353ab3c952
[INFO] 복구 크기:   1048576B
[PASS] TC-CRASH-005: fsync(fd) 후 크래시에서 데이터 완전 보존
```

| 항목 | 결과 |
|------|------|
| 파일 크기 | 1,048,576 B (1MB) |
| 원본 SHA256 | `2a82d04d4b094619919d039e575ace0ea907ea607783bd91d1f202353ab3c952` |
| 복구 SHA256 | `2a82d04d4b094619919d039e575ace0ea907ea607783bd91d1f202353ab3c952` |
| 크기 일치 | 1,048,576 B == 1,048,576 B |
| **판정** | **PASS** |

### 시나리오 B — 4KB 파일

**실행 명령어**
```
UFFS_BIN=../uffs UFFS_DEVICE=/tmp/flash.img UFFS_MOUNT=/mnt/uffs \
FILE_SIZE=4096 bash tc_crash_005.sh
```

**실행 로그**
```
[INFO] TC-CRASH-005 시작: FILE_SIZE=4096B
[INFO] Step 1: 마운트
[INFO] Step 2: 4096B 데이터 write + fsync
[INFO] 원본 SHA256: f33672a36f54ba230a37aafae7b3e5228083daed24e82302f7ac1c281e2cc29b
[INFO] Step 3: kill -9 (크래시 시뮬레이션)
[INFO] Step 4: 마운트 해제
[INFO] Step 5: 재마운트
[INFO] Step 6: 데이터 검증
[INFO] 복구 SHA256: f33672a36f54ba230a37aafae7b3e5228083daed24e82302f7ac1c281e2cc29b
[INFO] 복구 크기:   4096B
[PASS] TC-CRASH-005: fsync(fd) 후 크래시에서 데이터 완전 보존
```

| 항목 | 결과 |
|------|------|
| 파일 크기 | 4,096 B (4KB) |
| 원본 SHA256 | `f33672a36f54ba230a37aafae7b3e5228083daed24e82302f7ac1c281e2cc29b` |
| 복구 SHA256 | `f33672a36f54ba230a37aafae7b3e5228083daed24e82302f7ac1c281e2cc29b` |
| 크기 일치 | 4,096 B == 4,096 B |
| **판정** | **PASS** |

---

## 4.3 TC-REL-001 — fsync 내구성 반복 테스트 (NFR-REL-001)

**테스트 조건**: `FILE_SIZE=4096B`, kill -9 크래시 시뮬레이션, 단계적 반복 횟수 증가

| 반복 횟수 | 시작 시각 | 완료 시각 | 소요 시간 | 데이터 손실 | 크기 불일치 | 판정 |
|---------|---------|---------|---------|----------|----------|------|
| 50회 | 09:20:52 | 09:21:04 | 12초 | 0건 | 0건 | **PASS** |
| 100회 | 09:21:16 | 09:21:41 | 25초 | 0건 | 0건 | **PASS** |
| 500회 | 09:22:05 | 09:24:20 | 2분 15초 | 0건 | 0건 | **PASS** |
| 1,000회 | 09:24:36 | 09:28:48 | 4분 12초 | 0건 | 0건 | **PASS** |

**1,000회 실행 로그 (요약)**
```
[INFO] TC-REL-001 시작: ITERATIONS=1000, FILE_SIZE=4096B
[INFO] 시작 시각: 2026-03-11 09:24:36
[INFO] 진행: 100/1000 | 손실: 0 | 크기불일치: 0
[INFO] 진행: 200/1000 | 손실: 0 | 크기불일치: 0
[INFO] 진행: 300/1000 | 손실: 0 | 크기불일치: 0
[INFO] 진행: 400/1000 | 손실: 0 | 크기불일치: 0
[INFO] 진행: 500/1000 | 손실: 0 | 크기불일치: 0
[INFO] 진행: 600/1000 | 손실: 0 | 크기불일치: 0
[INFO] 진행: 700/1000 | 손실: 0 | 크기불일치: 0
[INFO] 진행: 800/1000 | 손실: 0 | 크기불일치: 0
[INFO] 진행: 900/1000 | 손실: 0 | 크기불일치: 0
[INFO] 진행: 1000/1000 | 손실: 0 | 크기불일치: 0

──────────────────────────────────────────
TC-REL-001 결과 요약
──────────────────────────────────────────
[INFO] 완료 시각:       2026-03-11 09:28:48
[INFO] 총 반복 횟수:    1000
[INFO] 데이터 손실:     0건
[INFO] 크기 불일치:     0건
[INFO] 총 실패 횟수:    0건
──────────────────────────────────────────
[PASS] TC-REL-001: 1000회 반복 power-cut 시나리오에서 데이터 손실 0건
[PASS] NFR-REL-001 인수 기준 충족: 데이터 손실률 0%
```

**판정**: **PASS** — NFR-REL-001 인수 기준 (1,000회 데이터 손실 0건) 완전 충족

---

## 4.4 TC-CRASH-DIR-001 — fsync(dirfd) 후 크래시 내구성

**실행 명령어**
```
UFFS_BIN=../uffs \
UFFS_DEVICE=/tmp/uffs_tcdir001.img \
UFFS_MOUNT=/tmp/uffs_tcdir001_mnt \
  bash tc_crash_dir_001.sh
```

**실행 로그**
```
[INFO] TC-CRASH-DIR-001 시작
[INFO] Step 1: 마운트
[INFO] Step 2: new_file.txt 생성 + fsync(dirfd)
fsync(dirfd) 완료
[INFO] Step 3: kill -9 크래시 시뮬레이션
[INFO] Step 4: 재마운트
[INFO] Step 5: new_file.txt readdir/open 확인
[INFO] 결과: 디렉토리 엔트리: ['new_file.txt']
FOUND:crash-dir-test
[PASS] TC-CRASH-DIR-001: fsync(dirfd) 후 크래시에서 디렉토리 엔트리 보존 확인
```

| 항목 | 결과 |
|------|------|
| 파일 생성 후 fsync(dirfd) | 완료 |
| kill -9 크래시 후 재마운트 | 성공 |
| readdir 결과 | `['new_file.txt']` — 엔트리 보존됨 |
| 파일 open + 내용 읽기 | `crash-dir-test` — 내용 일치 |
| **판정** | **PASS** |

---

## 4.5 TC-CRASH-NEG-001 — fsync 이전 크래시 (비보장 경계 관측)

> 본 TC는 합격/불합격 판정 대상이 아니다. fsync 미호출 시의 동작을 관측하여 SRS FR-FILE-005-3의 비보장 경계를 문서화하는 것이 목적이다.

**실행 명령어**
```
UFFS_BIN=../uffs \
UFFS_DEVICE=/tmp/uffs_tcdir001.img \
UFFS_MOUNT=/tmp/uffs_tcdir001_mnt \
  bash tc_crash_neg_001.sh
```

**실행 로그**
```
[INFO] TC-CRASH-NEG-001 시작 (음성 테스트 — 결과는 판정 대상 아님)
[INFO] Step 1: 마운트
[INFO] Step 2: write 수행 — fsync 호출 없음
[INFO] write 완료 (fsync 미호출). 원본 SHA256: d9829ae307d27a90385a3d8be4ae5069f7ab5fd7a2ca44a6dfdcebaa80e23bdb
[INFO] Step 3: kill -9 크래시 시뮬레이션
[INFO] Step 4: 재마운트
[OBS]  관측 결과: partial  (size=0, sha256_match=False)
[OBS]  이 케이스는 합격/불합격 판정 대상이 아닙니다 (FR-FILE-005-3 비보장 경계).
[OBS]  결과를 test-result.md 에 기록하십시오.

TC-CRASH-NEG-001 완료 (음성 테스트, 판정 없음)
```

**관측 결과 기록**

| 관측 항목 | 결과 |
|---------|------|
| 원본 SHA256 | `d9829ae307d27a90385a3d8be4ae5069f7ab5fd7a2ca44a6dfdcebaa80e23bdb` |
| 복구 후 파일 크기 | 0 B (파일 엔트리는 존재하나 내용 손실) |
| SHA256 일치 여부 | False |
| 관측 분류 | `partial` — 파일 엔트리는 유지되었으나 데이터 손실 |
| **판정** | **판정 없음 (관측 전용)** |

**해석**: fsync 미호출 상태에서 kill -9 크래시 발생 시, 재마운트 후 파일 크기가 0으로 복구된다. 이는 SRS FR-FILE-005-3에서 명시적으로 비보장으로 규정한 동작이며, 본 시스템의 결함이 아니다. 데이터 내구성은 fsync 호출 이후에만 보장된다.

---

# 5. 테스트 지표 (Testing Metrics)

| 지표 | 계산식 | 결과 | 목표 | 충족 여부 |
|-----|-------|------|------|---------|
| TC 실행률 | 12 / 12 × 100 | **100%** | 100% | ✅ |
| TC 통과율 (판정 대상) | 11 / 11 × 100 | **100%** | 100% | ✅ |
| 데이터 손실률 (1,000회) | 0 / 1,000 × 100 | **0%** | 0% | ✅ |
| 크래시 정합성 성공률 | 3 / 3 × 100 | **100%** | 100% | ✅ |
| Critical 결함 수 | — | **0건** | 0건 | ✅ |
| 크기 불일치 (1,000회) | 0 / 1,000 × 100 | **0%** | 0% | ✅ |

---

# 6. 종료 기준 충족 여부 (Exit Criteria)

test-plan.md §5.2 정상 종료 기준과 대조한다.

| 기준 | 조건 | 충족 여부 |
|-----|------|---------|
| Unit Test TC 전체 통과 | TC-FILE-001~007, TC-DIR-002~004, TC-META-001 모두 Pass | ✅ 10 / 10 |
| Crash Test 통과 | TC-CRASH-005, TC-CRASH-DIR-001 Pass | ✅ |
| Durability Test 통과 | TC-REL-001: 1,000회 중 데이터 손실 0건 | ✅ |
| Critical 미결 결함 없음 | Critical 결함 0건 | ✅ |

---

# 7. 인수 기준 충족 여부 (Acceptance Criteria)

test-plan.md §7 인수 기준과 대조한다.

## AC-001: 파일 연산 인수 기준

- [x] TC-FILE-001 통과 (파일 열기)
- [x] TC-FILE-002 통과 (파일 생성)
- [x] TC-FILE-003 통과 (파일 읽기)
- [x] TC-FILE-004 통과 (파일 쓰기)
- [x] TC-CRASH-005 통과 (파일 fsync 내구성)
- [x] TC-FILE-006 통과 (파일 rename)
- [x] TC-FILE-007 통과 (파일 삭제)
- [x] TC-CRASH-DIR-001 통과 (디렉토리 fsync 내구성)
- [x] TC-DIR-002 통과 (디렉토리 읽기)
- [x] TC-DIR-003 통과 (디렉토리 생성)
- [x] TC-DIR-004 통과 (디렉토리 삭제)
- [x] TC-META-001 통과 (메타데이터 조회)

## AC-002: 크래시 정합성 인수 기준 (핵심)

- [x] TC-CRASH-005 — fsync(fd) 이후 데이터 손실 0건
- [x] TC-CRASH-DIR-001 — fsync(dirfd) 이후 디렉토리 엔트리 오류 0건
- [x] TC-REL-001 — 1,000회 반복 power-cut 시 데이터 손실 0건 (NFR-REL-001)
- [x] 크래시 정합성 성공률 100%, 데이터 손실률 0%

## AC-003: 릴리즈 준비 완료

- [x] AC-001 파일 연산 기준 충족
- [x] AC-002 크래시 정합성 기준 충족
- [ ] 문서화 완료 — SRS v1.2, SDD, test-plan 작성 완료; 사용자 매뉴얼·API 문서 미작성
- [x] test-result.md 작성 완료

---

# 8. 발견된 결함 (Defects)

이번 테스트 사이클에서 새로 발견된 결함 없음.

| 결함 ID | 심각도 | 내용 | 상태 |
|--------|-------|------|------|
| — | — | 결함 없음 | — |

> 개발 단계에서 발견 및 수정된 사항:
> - CoW 예비 슬롯 부재 (`FILE_HEADER_DATA_PAGES` 31→30): **수정 완료**
> - GC 데드락 (`gc_collect_block`의 `flash_alloc_block` 호출): Rolling Spare 도입으로 **수정 완료**

---

# 9. 결론 및 권고사항

## 9.1 결론

FUSE 기반 UFFS 파일시스템은 핵심 품질 목표인 **Crash Consistency**를 달성하였다.

- **기능 정확성**: 11개 판정 대상 TC 전체 통과 (100%)
- **Crash Consistency (파일)**: 1MB 및 4KB 파일에 대한 fsync 후 kill -9 크래시에서 SHA256 완전 일치
- **Crash Consistency (디렉토리)**: fsync(dirfd) 후 kill -9 크래시에서 디렉토리 엔트리 및 파일 내용 완전 보존
- **내구성 (NFR-REL-001)**: 1,000회 반복 power-cut 시나리오에서 데이터 손실 **0건 (손실률 0%)**
- **비보장 경계 확인**: fsync 미호출 크래시 시 데이터 손실 발생 — SRS FR-FILE-005-3에 명시된 비보장 동작으로 확인

## 9.2 권고사항 (후속 작업)

| 우선순위 | 항목 | 내용 |
|---------|------|------|
| Medium | FR-FILE-004-3 구현 | 선제적 GC (빈 블록 임계치 도달 시 사전 GC 트리거) |
| Low | FR-STORE-001 구현 | `.statfs` FUSE 콜백 등록 (사용 가능 용량 조회) |

---

# 문서 개정 이력

| 버전 | 날짜 | 작성자 | 변경 내용 |
|-----|------|--------|----------|
| 1.0 | 2026-03-11 | 임재형 | 최초 작성 — 12개 TC 전체 실행 결과 수록 (통합 10종, TC-CRASH-005 x2, TC-REL-001 x4, TC-CRASH-DIR-001, TC-CRASH-NEG-001 관측) |
