# FUSE 기반 UFFS 파일시스템 — Crash Consistency 검증 환경

UFFS(Ultra-low-cost Flash File System)를 FUSE로 구현하고,
**전원 차단(power-cut) 후 데이터 정합성을 보장하는 Crash Consistency를 검증하는 테스트 환경**을 제공한다.

UFFS 기반 개발을 진행하는 개발자가 실제 하드웨어 없이 Linux 환경에서
kill -9 크래시 시뮬레이션으로 Crash Consistency를 검증할 수 있다.

---

## 검증 결과

| 항목 | 결과 |
|------|------|
| 판정 대상 TC | 12 / 12 PASS (100%) |
| 데이터 손실 (1,000회 power-cut) | 0건 (0%) |
| 검증 시나리오 | 7종 (100B · 4KB · 8KB · 20KB · 1MB · 디렉토리 · 멀티파일) |

---

## 요구사항

- Ubuntu 20.04 이상 (ARM64 / x86_64)
- `sudo` 권한

---

## 빠른 시작

### 1단계 — 의존성 설치

```bash
bash install_deps.sh
```

설치 항목: `build-essential`, `cmake`, `pkg-config`, `libfuse3-dev`, `fuse3`, `libgtest-dev`, `/mnt/uffs` 마운트 포인트 생성

### 2단계 — 전체 테스트 실행

```bash
bash run_tests.sh
```

---

## 테스트 구성

| TC | 내용 | 판정 |
|----|------|------|
| TC-FILE-001 ~ TC-META-001 | 파일·디렉토리·메타데이터 기능 통합 테스트 (10종) | 합격/불합격 |
| TC-CRASH-005 | fsync 후 kill -9 → 재마운트 SHA256 검증 (1MB · 4KB) | 합격/불합격 |
| TC-CRASH-FILE-ADV | 고급 크래시 정합성 5시나리오 (서브페이지·크로스블록·덮어쓰기·추가쓰기·멀티파일) | 합격/불합격 |
| TC-CRASH-DIR-001 | fsync(dirfd) 후 kill -9 → 디렉토리 엔트리 보존 검증 | 합격/불합격 |
| TC-CRASH-NEG-001 | fsync 미호출 크래시 → 비보장 경계 관측 | 관측 전용 |
| TC-REL-001 | 1,000회 반복 power-cut → 데이터 손실 0건 (NFR-REL-001) | 합격/불합격 |

---

## 핵심 설계

| 메커니즘 | 역할 |
|---------|------|
| CoW (Copy-on-Write) | 페이지 단위 즉시 기록, dirty 1→0 flip으로 구버전 원자적 무효화 |
| seal_byte 3단계 | `0xFF`(빈) → `0xFE`(쓰기중) → `0xFC`(완료) — remount 시 미완료 쓰기 자동 무시 |
| block_ts modulo-3 | 중복 블록 중 최신 버전 선택 기준 |
| Rolling Spare Block | GC 전용 예비 블록 유지 — 빈 블록 없는 상황에서도 GC 실행 가능 |

---

## 디렉토리 구조

```
uffs-crash-consistency/
├── src/                # 파일시스템 구현 (2,098 LOC)
│   ├── flash.h/c       # NAND 에뮬레이션 I/O
│   ├── tree.h/c        # 마운트 스캔 · 파일 트리
│   ├── gc.h/c          # Garbage Collection
│   ├── file.h/c        # 파일 연산 (CoW · fsync)
│   ├── dir.h/c         # 디렉토리 연산
│   ├── meta.h/c        # 메타데이터 조회
│   └── main.c          # FUSE 진입점
└── tests/              # 테스트 (1,330 LOC)
    ├── test_integration.cpp
    ├── tc_crash_005.sh
    ├── tc_crash_file_adv.sh
    ├── tc_crash_dir_001.sh
    ├── tc_crash_neg_001.sh
    └── tc_rel_001.sh

docs/
├── SRS.md              # 소프트웨어 요구사항 명세
├── SDD.md              # 소프트웨어 설계 문서
├── test-plan.md        # 테스트 계획서
└── test-result.md      # 테스트 결과서
```

---

## 참조

- `docs/SRS.md` — 요구사항 및 Crash Consistency 보장 범위 정의
- `docs/test-result.md` — 전체 TC 실행 결과 및 SHA256 검증값
