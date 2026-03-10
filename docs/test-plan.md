# FUSE 기반 UFFS 파일시스템 — Test Plan

**문서 표준**: ISO/IEC/IEEE 29119-3
**Test Plan ID**: TP-UFFS-001
**Reference SRS**: SRS v1.0 (2026.03.04)
**Version**: 0.1
**Date**: 2026-03-04
**Status**: Draft

---

# 1. 소개 (Introduction)

## 1.1 배경 및 목적 (Background and Purpose)

본 테스트 계획서는 FUSE 기반 UFFS 파일시스템(이하 "테스트 대상 시스템")이 SRS v1.0에 정의된 요구사항을 충족하는지 검증하기 위한 테스트 활동 전반을 정의한다. 본 프로젝트의 핵심 품질 목표는 **Crash Consistency** 보장이며, 모든 테스트 활동은 이 목표를 중심으로 계획된다.

## 1.2 테스트 범위 (Scope of Testing)

### In Scope

| 요구사항 ID | 요구사항 이름 | 테스트 유형 |
|------------|-------------|-----------|
| FR-FILE-001 | 파일 열기 | Integration Test |
| FR-FILE-002 | 파일 생성 | Integration Test |
| FR-FILE-003 | 파일 읽기 | Integration Test |
| FR-FILE-004 | 파일 쓰기 | Integration Test |
| FR-FILE-005 | 파일 동기화 (fsync) | Crash Test |
| FR-FILE-006 | 파일 이름 변경 (rename) | Integration Test |
| FR-FILE-007 | 파일 삭제 | Integration Test |
| FR-DIR-001 | 디렉토리 동기화 (fsync) | Crash Test |
| FR-DIR-002 | 디렉토리 읽기 | Integration Test |
| FR-DIR-003 | 디렉토리 생성 | Integration Test |
| FR-DIR-004 | 디렉토리 삭제 | Integration Test |
| FR-META-001 | 메타데이터 조회 | Integration Test |
| NFR-REL-001 | fsync 이후 데이터 손실 방지 | Durability Test |

### Out of Scope

- **성능 테스트**: 처리량, 마운트 시간 등 — SRS 3.4에 의해 제외
- **동시성 테스트**: 다중 스레드 동시 접근 — SRS 1.2 Out of Scope
- **플래시 관리**: 웨어 레벨링, 배드 블록 관리, ECC 자동 수정
- **크래시 복구**: 마운트 시 자동 검증 및 복구 로직

## 1.3 참조 문서 (References)

1. SRS v1.0 — FUSE 기반 UFFS 파일시스템 (2026.02.22)
2. ISO/IEC/IEEE 29119-3:2021 — Software and systems engineering — Software testing — Part 3: Test documentation
3. ISO/IEC/IEEE 29148:2018 — Systems and software engineering — Life cycle processes — Requirements engineering
4. libfuse 3.18.1 API Reference — https://github.com/libfuse/libfuse

## 1.4 용어 정의 (Glossary)

| 용어 | 정의 |
|-----|------|
| **Crash Test** | 전원 차단 또는 프로세스 강제 종료 후 재마운트하여 데이터 정합성을 검증하는 테스트 |
| **Durability Test** | 반복적 power-cut 시나리오에서 데이터 손실이 발생하지 않는지 검증하는 신뢰성 테스트 |
| **Test Oracle** | 테스트 결과의 정합성을 판정하는 기준 메커니즘 |
| **Power-Cut Simulation** | UFFS 프로세스의 `kill -9` 또는 VM 강제 종료를 통한 전원 차단 재현 |
| **TC** | Test Case. 개별 테스트 케이스 |
| **Entry Criteria** | 테스트 활동을 시작하기 위해 충족해야 하는 전제 조건 |
| **Exit Criteria** | 테스트 활동을 완료로 판정하기 위한 기준 |

---

# 2. 테스트 컨텍스트 (Context of Testing)

## 2.1 테스트 대상 (Test Items)

| 항목 | 내용 |
|-----|------|
| **시스템 이름** | FUSE 기반 UFFS 파일시스템 |
| **버전** | 1.0 |
| **실행 환경** | Ubuntu 20.04 / ARM64 (Guest VM) |
| **주요 바이너리** | `uffs_mount` (마운트 실행파일) |
| **핵심 라이브러리** | libfuse 3.18.1 |

## 2.2 이해관계자 (Stakeholders)

| 이해관계자 | 역할 | 관심사 |
|-----------|------|--------|
| 개발자 (임재형) | 개발 및 테스트 수행 | 기능 정확성, Crash Consistency |

## 2.3 역할 및 책임 (Roles and Responsibilities)

| 역할 | 담당자 | 책임 |
|-----|--------|------|
| **테스트 계획자** | 임재형 | 테스트 계획서 작성 및 유지 |
| **테스트 설계자** | 임재형 | TC 설계 및 테스트 데이터 준비 |
| **테스트 실행자** | 임재형 | TC 실행 및 결과 기록 |
| **결함 보고자** | 임재형 | 결함 발견 시 이슈 등록 및 재현 절차 작성 |

## 2.4 소통 계획 (Communication)

| 소통 유형 | 방법 | 주기 |
|----------|------|------|
| 테스트 결과 보고 | `test-result.md` 업데이트 | TC 실행 직후 |
| 결함 보고 | GitHub Issue 등록 | 결함 발견 즉시 |
| 최종 테스트 보고 | Test Report 문서 작성 | 모든 TC 완료 후 |

## 2.5 위험 관리 (Risk Register)

### 제품 위험 (Product Risks)

| 위험 ID | 위험 항목 | 가능성 | 영향 | 완화 전략 |
|--------|---------|-------|------|----------|
| PR-001 | fsync 이후 데이터 손실 발생 | High | Critical | TC-CRASH-005, TC-REL-001로 1,000회 반복 검증 |
| PR-002 | 디렉토리 엔트리 손실 (rename/create 후 crash) | High | Critical | TC-CRASH-DIR-001로 전용 검증 |
| PR-003 | 파일 크기(`st_size`) 불일치 | Medium | High | TC-META-001, TC-CRASH-005 판정 항목에 포함 |
| PR-004 | 재마운트 후 파일시스템 마운트 실패 | Low | Critical | 각 Crash Test의 사전조건으로 마운트 성공 여부 확인 |

### 프로젝트 위험 (Project Risks)

| 위험 ID | 위험 항목 | 가능성 | 영향 | 완화 전략 |
|--------|---------|-------|------|----------|
| PJR-001 | VM 강제 종료가 실제 power-cut을 완전히 재현하지 못함 | Medium | Medium | kill -9 (프로세스 크래시)와 VM 강제 종료 두 가지 시나리오 모두 실행 |
| PJR-002 | 1,000회 반복 테스트의 실행 시간 과다 | Medium | Low | 자동화 스크립트로 무인 실행 |
| PJR-003 | libfuse 3.18.1 Ubuntu 20.04 패키지 가용성 문제 | Low | High | 소스 빌드로 대체, 빌드 절차 SRS OTH-INST-001 참조 |

---

# 3. 테스트 전략 (Test Strategy)

## 3.1 테스트 레벨 및 유형 (Test Levels and Types)

| 레벨/유형 | 설명 | 적용 요구사항 |
|---------|------|-------------|
| **Integration Test** | FUSE 핸들러와 UFFS 구현의 통합 기능 검증 | FR-FILE-001~004, 006~007, FR-DIR-002~004, FR-META-001 |
| **Crash Test** | 전원 차단 시뮬레이션 후 재마운트하여 정합성 검증 | FR-FILE-005, FR-DIR-001 |
| **Durability Test** | 반복 power-cut 시나리오(1,000회+)로 데이터 손실 부재 검증 | NFR-REL-001 |
| **Negative Test** | 비보장 경계 확인 (fsync 없는 crash) | FR-FILE-005-3 |

## 3.2 테스트 설계 기법 (Test Design Techniques)

| 기법 | 적용 케이스 | 설명 |
|-----|-----------|------|
| **시나리오 기반 테스트** | TC-CRASH-005, TC-CRASH-DIR-001 | 현실적 사용 흐름(write→fsync→crash→remount→read) 재현 |
| **동치 분할 (Equivalence Partitioning)** | TC-FILE-003, TC-FILE-004 | 소형(1KB), 중형(1MB) 데이터 크기 구간 대표값 사용 |
| **경계값 분석 (Boundary Value Analysis)** | TC-FILE-003, TC-FILE-004 | 오프셋 0, 파일 끝 오프셋 등 경계 지점 테스트 |
| **양/음성 테스트 (Positive/Negative)** | TC-FILE-006, TC-FILE-007, TC-CRASH-NEG-001 | 정상 케이스와 비보장 케이스 분리 |
| **반복/스트레스 테스트** | TC-REL-001 | 동일 시나리오 1,000회 반복으로 누적 결함 탐지 |

## 3.3 판정 원칙 — Test Oracle (VER-ORACLE-001)

테스트는 내부 구현(플래시 블록 레이아웃)이 아니라 **FUSE 인터페이스를 통한 사용자 관측 결과** 기준으로 판정한다 (SRS VER-ORACLE-001 준수).

| 검증 대상 | 판정 방법 |
|---------|---------|
| 파일 데이터 정합성 | `sha256sum` 또는 `cmp`로 원본과 바이트 단위 비교 |
| 파일 크기 | `stat()`의 `st_size` 비교 |
| 디렉토리 엔트리 | `open()` 성공 여부 또는 `readdir()` 반환 목록 확인 |
| 시스템 콜 반환값 | 성공 시 0, 실패 시 errno 확인 |

## 3.4 테스트 지표 (Testing Metrics)

| 지표 | 계산식 | 목표 |
|-----|-------|------|
| **TC 통과율** | (통과 TC 수 / 전체 TC 수) × 100 | 100% |
| **데이터 손실률** | (데이터 손실 발생 회차 / 전체 반복 횟수) × 100 | 0% |
| **크래시 정합성 성공률** | (정합성 유지 회차 / 전체 크래시 테스트 횟수) × 100 | 100% |
| **결함 밀도** | 발견된 Critical 결함 수 | 릴리즈 기준: 0건 |

## 3.5 테스트 데이터 요구사항 (Test Data Requirements)

| 데이터 유형 | 크기/패턴 | 용도 | 생성 방법 |
|-----------|---------|------|---------|
| 랜덤 바이너리 데이터 | 1KB, 1MB | 파일 읽기/쓰기 검증 | `/dev/urandom` |
| 전부 0 패턴 | 1MB | 내구성 테스트 패턴 1 | `dd if=/dev/zero` |
| 전부 1 패턴 (0xFF) | 1MB | 내구성 테스트 패턴 2 | python 스크립트 |
| 알려진 체크섬 데이터 | 가변 | 정합성 검증 기준값 | 사전 생성 후 저장 |

## 3.6 테스트 환경 (Test Environment)

### 3.6.1 하드웨어

| 항목 | 호스트 | 게스트 VM |
|-----|--------|---------|
| **기기** | MacBook M4 Pro | — |
| **CPU** | Apple M4 Pro | ARM64 (aarch64) |
| **RAM** | 48GB | 4GB |
| **저장소** | SSD 512GB | 12GB |

### 3.6.2 소프트웨어

| 소프트웨어 | 버전 | 용도 |
|---------|-----|------|
| Ubuntu | 20.04 | 테스트 실행 OS (Guest) |
| Linux Kernel | 4.x 이상 (FUSE 모듈 활성화) | FUSE 인터페이스 제공 |
| libfuse | 3.18.1 | FUSE 구현 라이브러리 |
| GCC 또는 Clang | 7+ / 6+ | 테스트 빌드 |
| CMake | 3.10+ | 빌드 시스템 |
| Google Test | 최신 안정 버전 | 테스트 프레임워크 |

### 3.6.3 테스트 도구

| 도구 | 용도 |
|-----|------|
| `gtest` (Google Test) | Unit/Integration Test 자동화 실행 |
| `sha256sum`, `cmp` | 데이터 동일성 검증 |
| `stat` | 파일 메타데이터 검증 |
| `kill -9` | 프로세스 크래시 시뮬레이션 |
| VM 강제 종료 | Power-cut 시뮬레이션 |
| bash 스크립트 | TC-REL-001 반복 자동화 |

---

# 4. 테스트 케이스 설계 규칙

## 4.1 TC ID 체계

SRS 5.5 RTM의 테스트 케이스 ID와 일치시킨다.

| 접두사 | 대상 |
|-------|------|
| `TC-FILE-###` | 파일 연산 (FR-FILE-###) |
| `TC-CRASH-###` | 파일 fsync 크래시 내구성 (FR-FILE-005) |
| `TC-CRASH-NEG-###` | 비보장 경계 음성 테스트 |
| `TC-CRASH-DIR-###` | 디렉토리 fsync 크래시 내구성 (FR-DIR-001) |
| `TC-DIR-###` | 디렉토리 연산 (FR-DIR-###) |
| `TC-META-###` | 메타데이터 (FR-META-001) |
| `TC-REL-###` | 내구성 반복 테스트 (NFR-REL-001) |

## 4.2 케이스 템플릿

각 TC는 다음 항목을 포함한다:

- **관련 요구사항**: SRS 요구사항 ID
- **사전조건**: 테스트 실행 전 충족 조건
- **테스트 절차**: 순서대로 기술된 실행 단계
- **기대 결과 / 판정**: 합격 기준

---

# 5. 진입 / 종료 / 중단 기준

## 5.1 진입 기준 (Entry Criteria)

테스트를 시작하기 위해 아래 조건이 모두 충족되어야 한다.

| 기준 | 확인 방법 |
|-----|---------|
| 테스트 대상 바이너리가 성공적으로 빌드됨 | `make` 종료 코드 0 |
| 파일시스템 마운트 성공 (EXT-UI-001) | `mount` 명령 성공 및 마운트 포인트 접근 가능 |
| 테스트 환경(VM, 소프트웨어) 설치 완료 | 소프트웨어 버전 확인 |
| 본 테스트 계획서 Draft 이상 상태 | 문서 상태 확인 |

## 5.2 종료 기준 (Exit Criteria)

### 정상 종료

| 기준 | 조건 |
|-----|------|
| Unit Test TC 전체 통과 | TC-FILE-001~007, TC-DIR-002~004, TC-META-001 모두 Pass |
| Crash Test 통과 | TC-CRASH-005, TC-CRASH-DIR-001 Pass |
| Durability Test 통과 | TC-REL-001: 1,000회 중 데이터 손실 0건 |
| Critical 미결 결함 없음 | 등록된 Critical 결함이 모두 Closed 상태 |

### 조기 종료 (Fail)

| 조건 | 조치 |
|-----|------|
| TC-CRASH-005 또는 TC-CRASH-DIR-001에서 데이터 손실 발생 | 테스트 중단, 결함 보고 후 수정 재개 |
| 재마운트 후 파일시스템 접근 불가 | 테스트 중단, Critical 결함 등록 |

## 5.3 중단 및 재개 기준 (Suspension and Resumption Criteria)

### 중단 기준

- 파일시스템 마운트 자체가 실패하여 어떠한 TC도 실행 불가한 경우
- TC-CRASH-005 또는 TC-CRASH-DIR-001에서 연속 3회 이상 데이터 손실 발생 시
- 테스트 환경(VM) 이상으로 재현 불가한 상태가 된 경우

### 재개 조건

- 중단 원인이 된 결함이 수정되고, 해당 TC가 Pass로 전환된 경우
- 테스트 환경 복구 완료 확인

---

# 6. 테스트 케이스 (Test Cases)

---

## TC-FILE-001 — 파일 열기

**Related Requirements**: FR-FILE-001

**사전조건**: 파일시스템 마운트됨, 대상 파일 존재

### 절차
1. `open("existing.txt", O_RDONLY)` 호출
2. 반환된 fd 확인
3. `close(fd)` 호출

### 기대 결과 / 판정
- fd >= 0 (음수이면 실패)

---

## TC-FILE-002 — 파일 생성

**Related Requirements**: FR-FILE-002

**사전조건**: 파일시스템 마운트됨, 대상 경로에 파일 없음

### 절차
1. `open("test.txt", O_CREAT | O_WRONLY, 0644)` 호출
2. `close(fd)` 호출
3. `stat("test.txt")` 호출

### 기대 결과 / 판정
- `stat()` 성공, `st_size == 0`

---

## TC-FILE-003 — 파일 읽기

**Related Requirements**: FR-FILE-003

**사전조건**: 파일시스템 마운트됨, 알려진 데이터가 기록된 파일 존재

### 절차
1. 1MB 랜덤 데이터를 준비하여 파일에 기록
2. `pread(fd, buf, size, offset)` 호출
3. 읽은 데이터와 원본 데이터 비교 (`sha256sum` 또는 `cmp`)

### 기대 결과 / 판정
- 읽은 데이터가 원본과 바이트 단위로 일치

---

## TC-FILE-004 — 파일 쓰기

**Related Requirements**: FR-FILE-004

**사전조건**: 파일시스템 마운트됨

### 절차
1. 파일 생성
2. 1MB 랜덤 데이터 `write(fd, data, 1MB)` 수행
3. `lseek(fd, 0, SEEK_SET)` 후 `read()` 수행
4. 데이터 비교

### 기대 결과 / 판정
- write 반환값이 요청 크기와 동일
- 읽은 데이터가 쓴 데이터와 일치

---

## TC-CRASH-005 — fsync(fd) 후 크래시 내구성

**Related Requirements**: FR-FILE-005, NFR-REL-001

**사전조건**: 파일시스템 마운트됨

### 절차 A — 프로세스 크래시
1. 파일 생성 및 1MB 데이터 write
2. `fsync(fd)` 호출, 반환값 0 확인
3. UFFS 프로세스 `kill -9`
4. 재마운트
5. `sha256sum` 및 `stat.st_size` 확인

### 절차 B — VM 강제 종료
1. 파일 생성 및 데이터 write
2. `fsync(fd)` 호출, 반환값 0 확인
3. VM 강제 종료
4. 재부팅 후 재마운트
5. 데이터 및 크기 확인

### 기대 결과 / 판정
- 파일 데이터가 원본과 일치 (sha256 동일)
- `st_size`가 fsync 완료 시점의 크기와 동일

---

## TC-CRASH-NEG-001 — fsync 이전 크래시 (비보장 케이스)

**Related Requirements**: FR-FILE-005 (FR-FILE-005-3 비보장 경계 확인)

**목적**: fsync 없이 크래시 발생 시 결과를 보장하지 않음을 확인하는 음성 테스트

### 절차
1. 파일 생성 및 write 수행
2. `fsync` 호출 없이 프로세스 강제 종료
3. 재마운트 후 파일 상태 관측 및 기록

### 기대 결과 / 판정
- 이 케이스의 결과(유지/손실)는 합격/불합격 판정 대상이 아님
- 관측 결과를 기록하여 비보장 범위를 문서화

---

## TC-FILE-006 — 파일 이름 변경 (rename)

**Related Requirements**: FR-FILE-006

**사전조건**: 파일시스템 마운트됨, `src.txt` 존재

### 절차
1. `open("src.txt", O_CREAT|O_WRONLY)` 후 `close()`
2. `rename("src.txt", "dst.txt")` 호출
3. `stat("dst.txt")` 및 `stat("src.txt")` 확인

### 기대 결과 / 판정
- `rename()` 반환값 0
- `stat("dst.txt")` 성공
- `stat("src.txt")` 실패 (ENOENT)

---

## TC-FILE-007 — 파일 삭제

**Related Requirements**: FR-FILE-007

**사전조건**: 파일시스템 마운트됨, `test.txt` 존재

### 절차
1. `unlink("test.txt")` 호출
2. `stat("test.txt")` 호출

### 기대 결과 / 판정
- `unlink()` 반환값 0
- `stat()` 실패 (ENOENT)

---

## TC-CRASH-DIR-001 — fsync(dirfd) 후 크래시 내구성

**Related Requirements**: FR-DIR-001

**사전조건**: 파일시스템 마운트됨

### 절차
1. 파일 생성 (`create "new_file.txt"`)
2. 부모 디렉토리 fd 획득 (`open("/mnt/uffs", O_RDONLY)`)
3. `fsync(dirfd)` 호출, 반환값 0 확인
4. UFFS 프로세스 `kill -9`
5. 재마운트
6. `readdir()` 또는 `open("new_file.txt")` 확인

### 기대 결과 / 판정
- 재마운트 후 `new_file.txt`가 디렉토리 엔트리에 동일하게 관측됨

---

## TC-DIR-002 — 디렉토리 읽기

**Related Requirements**: FR-DIR-002

**사전조건**: 파일시스템 마운트됨, 디렉토리 내 파일 1개 이상 존재

### 절차
1. 파일 생성 후 `readdir("/mnt/uffs")` 수행
2. 반환된 엔트리 목록 확인

### 기대 결과 / 판정
- 생성한 파일 이름이 목록에 포함됨

---

## TC-DIR-003 — 디렉토리 생성

**Related Requirements**: FR-DIR-003

**사전조건**: 파일시스템 마운트됨

### 절차
1. `mkdir("/mnt/uffs/newdir", 0755)` 호출
2. `stat("/mnt/uffs/newdir")` 호출

### 기대 결과 / 판정
- `mkdir()` 반환값 0
- `stat()` 성공, `S_ISDIR(st_mode)` == true

---

## TC-DIR-004 — 디렉토리 삭제

**Related Requirements**: FR-DIR-004

**사전조건**: 파일시스템 마운트됨, 빈 디렉토리 `emptydir` 존재

### 절차
1. `rmdir("/mnt/uffs/emptydir")` 호출
2. `stat("/mnt/uffs/emptydir")` 호출

### 기대 결과 / 판정
- `rmdir()` 반환값 0
- `stat()` 실패 (ENOENT)

---

## TC-META-001 — 메타데이터 조회

**Related Requirements**: FR-META-001

**사전조건**: 파일시스템 마운트됨

### 절차
1. 파일 생성 후 1MB write
2. `stat("test.txt")` 호출
3. `st_size`, `st_mode`, `st_nlink` 확인

### 기대 결과 / 판정
- `st_size == 1048576`
- `st_mode` 정상 (파일 타입 및 권한 필드 유효)
- `st_nlink >= 1`

---

## TC-REL-001 — fsync 내구성 반복 테스트

**Related Requirements**: NFR-REL-001

**목적**: 1,000회 이상의 반복 power-cut 시나리오에서 데이터 손실 0건 검증

### 절차
1. TC-CRASH-005 절차 A를 bash 스크립트로 자동화
2. 매 반복마다 다른 데이터 패턴 순환 사용 (전부 0, 전부 1, 랜덤)
3. 1,000회 반복 실행
4. 각 회차마다 데이터 손실 발생 여부 기록

### 기대 결과 / 판정
- 1,000회 전체에서 데이터 손실 0건
- `st_size` 불일치 0건

---

# 7. 인수 기준 (Acceptance Criteria)

SRS 4.2 인수 기준과 1:1 연계한다.

## AC-001: 파일 연산 인수 기준

- [ ] TC-FILE-001 통과 (파일 열기)
- [ ] TC-FILE-002 통과 (파일 생성)
- [ ] TC-FILE-003 통과 (파일 읽기)
- [ ] TC-FILE-004 통과 (파일 쓰기)
- [ ] TC-CRASH-005 통과 (파일 fsync 내구성)
- [ ] TC-FILE-006 통과 (파일 rename)
- [ ] TC-FILE-007 통과 (파일 삭제)
- [ ] TC-CRASH-DIR-001 통과 (디렉토리 fsync 내구성)
- [ ] TC-DIR-002 통과 (디렉토리 읽기)
- [ ] TC-DIR-003 통과 (디렉토리 생성)
- [ ] TC-DIR-004 통과 (디렉토리 삭제)
- [ ] TC-META-001 통과 (메타데이터 조회)

## AC-002: 크래시 정합성 인수 기준 (핵심)

- [ ] TC-CRASH-005 — fsync(fd) 이후 데이터 손실 0건
- [ ] TC-CRASH-DIR-001 — fsync(dirfd) 이후 디렉토리 엔트리 오류 0건
- [ ] TC-REL-001 — 1,000회 반복 power-cut 시 데이터 손실 0건 (NFR-REL-001)
- [ ] 크래시 정합성 성공률 100%, 데이터 손실률 0%

## AC-003: 릴리즈 준비 완료

- [ ] AC-001, AC-002 모두 통과
- [ ] 문서화 완료 (SRS, 설계서, 사용자 매뉴얼, API 문서)
- [ ] 라이선스 고지 및 저작권 표시 완료
- [ ] test-result.md 작성 완료

---

# 8. 태스크 및 일정 (Tasks and Schedule)

## 8.1 테스트 태스크

| 태스크 ID | 태스크 이름 | 선행 태스크 | 담당자 |
|---------|-----------|-----------|--------|
| T-001 | 테스트 환경 구축 (VM, libfuse, gtest) | — | 임재형 |
| T-002 | Unit Test TC 구현 (TC-FILE-001~007, TC-DIR-002~004, TC-META-001) | T-001 | 임재형 |
| T-003 | Crash Test TC 구현 (TC-CRASH-005, TC-CRASH-DIR-001) | T-001 | 임재형 |
| T-004 | Durability Test 자동화 스크립트 작성 (TC-REL-001) | T-003 | 임재형 |
| T-005 | Unit Test 실행 및 결과 기록 | T-002 | 임재형 |
| T-006 | Crash Test 실행 및 결과 기록 | T-003 | 임재형 |
| T-007 | Durability Test 실행 (1,000회) | T-004 | 임재형 |
| T-008 | test-result.md 작성 및 최종 검토 | T-005, T-006, T-007 | 임재형 |

## 8.2 일정

테스트 일정은 구현 완료 이후 확정된다 (TBD).

---

# 9. 테스트 산출물 (Test Deliverables)

| 산출물 | 설명 | 생성 시점 |
|-------|------|---------|
| `test-plan.md` (본 문서) | 테스트 계획서 | 테스트 시작 전 |
| `test-result.md` | 각 TC의 실행 결과 및 통과/실패 기록 | TC 실행 완료 후 |
| `logs/TC-<ID>-<timestamp>/` | TC별 실행 로그 및 데이터 | TC 실행 중 |
| GitHub Issues | 발견된 결함 보고서 | 결함 발견 즉시 |

---

## 문서 개정 이력

| 버전 | 날짜 | 작성자 | 변경 내용 |
|-----|------|--------|----------|
| 0.1 | 2026-03-04 | 임재형 | 초안 작성 |
