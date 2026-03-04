# PROJECT CONTEXT

## 1. Project Identity

- **Project Name**: FUSE 기반 UFFS 파일시스템
- **Domain**: User-space filesystem over FUSE (libfuse 3.18.1)
- **Primary Goal**: Crash Consistency 보장 및 검증 (신뢰성 우선, 성능 비목표)
- **SRS**: v1.0 (2026.02.22)
- **SDD**: v1.0 (2026.03.04)
- **Test Plan**: TP-UFFS-001 v0.1 (2026.03.04)

---

## 2. Non-Negotiable Objectives (절대 목표)

1. **Crash Consistency**: fsync 성공 이후 전원 차단 시에도 데이터 및 메타데이터 일관성 보장.
2. **Metadata Integrity**: 파일 크기(`st_size`), 이름, 디렉토리 엔트리 일관성 절대 유지.
3. **Testability**: 모든 보장 범위는 FUSE 인터페이스를 통해 관측 및 검증 가능해야 한다.
4. **Traceability**: SRS 요구사항 → SDD 설계 → 테스트 케이스 → 결과까지 추적 가능해야 한다.

이 중 하나라도 위반하는 설계 변경은 허용되지 않는다.

---

## 3. Explicit Non-Goals (하지 않는 것)

- **No concurrent access support**: 동시성은 Out of Scope (SRS §1.2). 전역 락으로 직렬화.
- **No kernel-level optimization**: FUSE userspace 구현만 대상.
- **No performance optimization**: 처리량, 레이턴시 측정은 제외 (SRS §3.4).
- **No ECC / bad block management / wear leveling**: 플래시 관리 기능 없음.
- **No crash recovery on mount**: 마운트 시 자동 수정 기능 없음 (스캔 기반 재구성만).
- **No fine-grained locking**: 추가 근거 없이 세분화된 락 설계 금지.
- **No undefined behavior**: 정의되지 않은 동작 허용 불가.

---

## 4. Crash Model Definition

크래시는 언제든 발생할 수 있다:
- 임의의 명령어 경계
- `write()` / `pwrite()` 실행 중
- `rename()` 실행 중
- `fsync(fd)` 이후
- `fsync(dirfd)` 이후

**내구성 보장 경계 (Durability Boundary):**
- 파일 데이터 + 파일 크기: `fsync(fd)` 성공 반환 시점 이후 보장 (FR-FILE-005)
- 디렉토리 이름 매핑: `fsync(dirfd)` 성공 반환 시점 이후 보장 (FR-DIR-001)
- fsync 없이 write만 한 경우: 보장하지 않음 (FR-FILE-005-3, FR-DIR-001-2)

---

## 5. Concurrency Model

**전략**: 단일 전역 뮤텍스 (`pthread_mutex_t g_lock`)

모든 FUSE 콜백은 `g_lock` 획득 후 실행, 반환 전 해제한다. 동시 접근은 직렬화되며 동시성은 지원하지 않는다 (ADR-004).

```
Priority: Safety > Determinism > Performance
```

---

## 6. In-Scope Operations

### 기능 요구사항

| 카테고리 | 연산 | 요구사항 ID |
|---------|------|-----------|
| **파일** | 열기, 생성, 읽기, 쓰기, fsync, rename, 삭제 | FR-FILE-001 ~ FR-FILE-007 |
| **디렉토리** | fsync, 읽기(readdir), 생성, 삭제 | FR-DIR-001 ~ FR-DIR-004 |
| **메타데이터** | getattr (권한, 링크 수, 파일 크기) | FR-META-001 |

### 비기능 요구사항

| 속성 | 요구사항 ID |
|-----|-----------|
| fsync 이후 데이터 손실 0건 | NFR-REL-001 |

### Out of Scope

- Journaling, Copy-on-Write (단, Two-Phase Write 원리 사용)
- 성능 최적화, 동시성, ECC, 배드 블록 관리, 크래시 마운트 복구

---

## 7. Key Design Decisions

### 크래시 정합성 메커니즘: Two-Phase Write + seal_byte

```
Phase 1 (Unseal):  pwrite(data, seal_byte=0x00)   ← 크래시 시 무시
Phase 2 (Seal):    pwrite(seal_byte=0xFE)          ← 커밋 완료
복구:              seal_byte=0xFE 페이지만 신뢰
```

### 컴포넌트 구조

| 컴포넌트 | 소스 | 역할 |
|---------|-----|------|
| FUSE Adapter | `fuse_ops.c` | libfuse 콜백, Global Lock |
| File Manager | `file.c` | FR-FILE-001~007 |
| Directory Manager | `dir.c` | FR-DIR-001~004 |
| Meta Manager | `meta.c` | FR-META-001 |
| Tree Manager | `tree.c` | 인메모리 트리, 마운트 스캔 |
| Flash I/O Layer | `flash.c` | seal/unseal, fdatasync |

---

## 8. Verification Strategy

**검증 철학**: FUSE 인터페이스를 통한 end-to-end 관측 (구현 내부 직접 접근 금지)

**Oracle 정의 (VER-ORACLE-001)**: fsync 후 크래시 → 재마운트 → FUSE read로 데이터 비교 (`sha256sum`)

| 검증 유형 | 대상 | 테스트 케이스 |
|---------|------|------------|
| Integration Test | FR-FILE-001~004, 006~007, FR-DIR-002~004, FR-META-001 | TC-FILE-001~007, TC-DIR-002~004, TC-META-001 |
| Crash Test | FR-FILE-005, FR-DIR-001 | TC-CRASH-005, TC-CRASH-DIR-001 |
| Durability Test | NFR-REL-001 | TC-REL-001 (1,000회 반복) |
| Negative Test | FR-FILE-005-3, FR-DIR-001-2 (비보장 경계) | TC-CRASH-NEG-001 |

**Traceability**: SRS → RTM → Test Plan → test-result.md (구현 후 작성)

---

## 9. ADR Summary

| ADR ID | 결정 사항 | 상태 |
|-------|---------|------|
| ADR-001 | 요구사항 명세 기준으로 IEEE 29148 채택 | Accepted |
| ADR-002 | 요구사항 추적 도구로 GitHub Issues 채택 | Accepted |
| ADR-003 | 파일시스템 구현 방식으로 FUSE 채택 | Accepted |
| ADR-004 | 전역 락(Global Lock)으로 FUSE 콜백 직렬화 (동시성 비지원) | Accepted |
| ADR-005 | Crash Consistency 구현으로 Two-Phase Write (CoW 기반 seal 메커니즘) 채택 | Accepted |

---

## 10. What I Want From AI

AI에게 답변을 요청할 때:

- **Non-Negotiable Objectives를 항상 존중할 것**
- 명시적으로 요청하지 않은 범위 확장을 제안하지 말 것
- 설계 변경 시 RTM 추적성에 미치는 영향을 명시할 것
- 위험 요소가 있으면 명시적으로 식별할 것
- 테스트 가능하고 결정적인(deterministic) 설계를 우선 제안할 것
- fsync 보장 경계(FR-FILE-005, FR-DIR-001)를 침해하는 제안은 거부할 것
