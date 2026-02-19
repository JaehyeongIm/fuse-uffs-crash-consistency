# FUSE 기반 UFFS 파일시스템 — Test Plan

**Document**: Test Plan  
**Reference**: ISO/IEC/IEEE 29148:2018
**Version**: 0.1  
**Date**: 2026-02-16  
**Status**: Draft  

---

# 1. 목적 (Purpose)

본 테스트 계획서는 FUSE 기반 UFFS 파일시스템이 SRS에 정의된 요구사항을 충족하는지 검증하기 위한 테스트 범위, 전략, 환경, 케이스 구조, 인수 기준을 정의한다.

## 핵심 검증 목표

- `fsync(fd)` 성공 후 크래시/재마운트 이후에도 파일 데이터와 `st_size`가 동일하게 관측되는지 검증 (FR-FILE-004)
- `fsync(dirfd)` 성공 후 디렉토리 엔트리 변화(create/rename)가 재마운트 후 동일하게 관측되는지 검증 (FR-DIR-001)
- 기본 파일 연산(create/read/write) 정상 동작 (FR-FILE-001~003)
- 최소 메타데이터 조회(getattr/stat) 동작 (FR-META-001)
- 성능 요구사항 충족 여부 (NFR-PERF-001~003)
- fsync 이후 power-cut 상황에서 데이터 손실 0건 (NFR-REL-001)

---

# 2. 테스트 범위 (Scope)

## 2.1 In Scope

### Functional Requirements
- FR-FILE-001: 파일 생성
- FR-FILE-002: 파일 읽기
- FR-FILE-003: 파일 쓰기
- FR-FILE-004: 파일 fsync 내구성
- FR-DIR-001: 디렉토리 fsync 내구성
- FR-META-001: 메타데이터 조회(getattr/stat)

### Non-Functional Requirements
- NFR-PERF-001: 순차 읽기 처리량
- NFR-PERF-002: 순차 쓰기 처리량
- NFR-PERF-003: 마운트 시간
- NFR-REL-001: fsync 이후 데이터 손실 방지

## 2.2 Out of Scope

- mkdir/rmdir 등 디렉토리 조작 기능 자체 구현 검증
- 웨어 레벨링, 배드 블록 관리, ECC 자동 수정
- 자동 복구 로직 검증 (본 릴리즈 범위 외)

---

# 3. 테스트 환경 (Test Environment)

## 3.1 하드웨어

**Host**
- Macbook M4 Pro
- RAM 48GB

**Guest VM**
- Ubuntu 20.04
- ARM64 (aarch64)
- RAM 4GB
- Disk 12GB

## 3.2 소프트웨어

- Linux Kernel 4.4+
- libfuse 3.2.0+
- GCC 7+ 또는 Clang 6+
- CMake 3.10+

## 3.3 테스트 도구

- bash + coreutils (`dd`, `cmp`, `stat`, `sha256sum`)
- fio (성능 테스트)
- VM 강제 종료 (power-cut 유사 시뮬레이션)
- 프로세스 강제 종료 (`kill -9`) 기반 crash 테스트

---

# 4. 테스트 전략 (Test Strategy)

## 4.1 테스트 레벨

- Integration Test (FUSE + 온디스크 레이아웃 통합)
- Crash/Durability Test (핵심)
- Performance Test

## 4.2 판정 원칙

테스트는 내부 구현 기준이 아니라 **사용자 관측 결과 기준**으로 판정한다.

- 파일 데이터: `sha256sum` 또는 `cmp`로 동일성 확인
- 파일 크기: `stat`의 `st_size`
- 디렉토리 엔트리: `open()` 성공 여부 또는 `readdir()` 결과

---

# 5. 테스트 케이스 설계 규칙

## 5.1 TC ID 체계

- TC-FILE-### : 파일 기능
- TC-META-### : 메타데이터
- TC-DIR-###  : 디렉토리 내구성
- TC-CRASH-###: 크래시 내구성
- TC-PERF-### : 성능

## 5.2 케이스 템플릿

- 목적
- 관련 요구사항
- 사전조건
- 테스트 절차
- 기대 결과
- 판정 기준

---

# 6. 테스트 케이스

---

## TC-FILE-001 — 파일 생성

**Related Requirements**: FR-FILE-001  

### 절차
1. `open("test.txt", O_CREAT | O_WRONLY)`
2. `close()`

### 기대 결과
- 파일 존재
- `stat()` 성공

---

## TC-FILE-002 — 파일 쓰기 및 읽기

**Related Requirements**: FR-FILE-002, FR-FILE-003  

### 절차
1. 1MB 랜덤 데이터 생성
2. write 수행
3. read 수행
4. `sha256sum` 비교

### 기대 결과
- read 데이터와 write 데이터 동일

---

## TC-META-001 — 메타데이터 조회

**Related Requirements**: FR-META-001  

### 절차
1. 파일 생성 후 1MB write
2. `stat test.txt`

### 기대 결과
- `st_size == 1048576`
- 최소 필드 정상 반환

---

## TC-CRASH-001 — fsync(fd) 후 프로세스 크래시

**Related Requirements**: FR-FILE-004, NFR-REL-001  

### 절차
1. 파일 생성 및 write
2. `fsync(fd)` 호출
3. UFFS 프로세스 `kill -9`
4. 재마운트
5. checksum 및 `stat.size` 확인

### 기대 결과
- 파일 데이터 동일
- `st_size` 동일

---

## TC-CRASH-002 — fsync 이전 크래시 (비보장 케이스)

**Related Requirements**: FR-FILE-004  

### 절차
1. write 수행
2. fsync 없이 프로세스 종료
3. 재마운트 후 관측

### 기대 결과
- 결과는 보장 대상 아님
- 관측 결과 기록

---

## TC-CRASH-003 — fsync 후 VM 강제 종료

**Related Requirements**: FR-FILE-004, NFR-REL-001  

### 절차
1. write 수행
2. fsync 성공
3. VM 강제 종료
4. 재부팅 후 재마운트
5. checksum 비교

### 기대 결과
- 데이터 동일
- `st_size` 동일

---

## TC-DIR-001 — create + dirfsync + 크래시

**Related Requirements**: FR-DIR-001  

### 절차
1. 파일 생성
2. 디렉토리 fd 획득
3. `fsync(dirfd)`
4. 크래시 발생
5. 재마운트
6. 파일 존재 여부 확인

### 기대 결과
- 엔트리 동일하게 관측

---

## TC-PERF-001 — 순차 읽기 처리량

**Related Requirements**: NFR-PERF-001  

### 절차
- fio 기반 1GB 순차 읽기 테스트

### 기대 결과
- 평균 ≥ 10MB/s

---

## TC-PERF-002 — 순차 쓰기 처리량

**Related Requirements**: NFR-PERF-002  

### 기대 결과
- 평균 ≥ 5MB/s

---

## TC-PERF-003 — 마운트 시간

**Related Requirements**: NFR-PERF-003  

### 절차
- 대량 파일 환경에서 mount 시간 측정

### 기대 결과
- ≤ 10초

---

# 7. 인수 기준 (Acceptance Criteria)

## 기능

- 모든 TC-FILE, TC-META 통과
- TC-CRASH-001, TC-CRASH-003 통과
- TC-DIR-001 통과

## 신뢰성

- fsync 이후 power-cut 시 데이터 손실 0건
- 최소 50회 반복 테스트 (시간 여유 시 확장)

## 성능

- 모든 PERF 요구사항 목표 달성

---

# 8. 테스트 산출물

- `test-result.md`
- 로그 디렉토리 구조:

