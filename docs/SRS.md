# FUSE 기반 UFFS 파일시스템
## Software Requirements Specification (SRS)

**문서 표준**: ISO/IEC/IEEE 29148
**버전(Version)**: 1.0
**작성일(Date)**: 2026.03.04
**작성자(Author)**: 임재형

---

## 목차

- [FUSE 기반 UFFS 파일시스템](#fuse-기반-uffs-파일시스템)
  - [Software Requirements Specification (SRS)](#software-requirements-specification-srs)
  - [목차](#목차)
- [1. Introduction (개요)](#1-introduction-개요)
  - [1.1 Purpose (목적)](#11-purpose-목적)
  - [1.2 Scope (범위)](#12-scope-범위)
  - [1.3 Product overview (제품 개요)](#13-product-overview-제품-개요)
    - [1.3.1 Product perspective (제품 관점)](#131-product-perspective-제품-관점)
    - [1.3.2 Product functions (제품 기능)](#132-product-functions-제품-기능)
    - [1.3.3 User characteristics (사용자 특성)](#133-user-characteristics-사용자-특성)
    - [1.3.4 Limitations (제한사항)](#134-limitations-제한사항)
  - [1.4 Definitions (용어 정의)](#14-definitions-용어-정의)
- [2. References (참조 문서)](#2-references-참조-문서)
- [3. Specific requirements (상세 요구사항)](#3-specific-requirements-상세-요구사항)
  - [3.1 External interfaces (외부 인터페이스)](#31-external-interfaces-외부-인터페이스)
    - [3.1.1 User interfaces (사용자 인터페이스)](#311-user-interfaces-사용자-인터페이스)
      - [EXT-UI-001: 마운트 명령어](#ext-ui-001-마운트-명령어)
      - [EXT-UI-002: 언마운트 명령어](#ext-ui-002-언마운트-명령어)
    - [3.1.2 Software interfaces (소프트웨어 인터페이스)](#312-software-interfaces-소프트웨어-인터페이스)
      - [EXT-SW-001: FUSE API](#ext-sw-001-fuse-api)
  - [3.2 Functions (기능 요구사항)](#32-functions-기능-요구사항)
    - [3.2.1 File Operations (파일 연산)](#321-file-operations-파일-연산)
      - [FR-FILE-001 파일 열기](#fr-file-001-파일-열기)
      - [FR-FILE-002: 파일 생성](#fr-file-002-파일-생성)
      - [FR-FILE-003: 파일 읽기](#fr-file-003-파일-읽기)
      - [FR-FILE-004: 파일 쓰기](#fr-file-004-파일-쓰기)
      - [FR-FILE-004-1: 대용량 파일 쓰기 지원](#fr-file-004-1-대용량-파일-쓰기-지원)
      - [FR-FILE-004-2: 공간 부족 시 자동 공간 회수](#fr-file-004-2-공간-부족-시-자동-공간-회수)
      - [FR-FILE-004-3: 선제적 공간 회수 (임계치 기반)](#fr-file-004-3-선제적-공간-회수-임계치-기반)
      - [FR-FILE-004-4: 부분 쓰기 데이터 보존](#fr-file-004-4-부분-쓰기-데이터-보존)
      - [FR-FILE-005: 파일 동기화 (fsync)](#fr-file-005-파일-동기화-fsync)
      - [FR-FILE-006: 파일 이름 변경 (rename)](#fr-file-006-파일-이름-변경-rename)
      - [FR-FILE-007: 파일 삭제](#fr-file-007-파일-삭제)
    - [3.2.2 Directory Operations (디렉토리 연산)](#322-directory-operations-디렉토리-연산)
      - [FR-DIR-001: 디렉토리 동기화 (fsync)](#fr-dir-001-디렉토리-동기화-fsync)
      - [FR-DIR-002 디렉토리 읽기](#fr-dir-002-디렉토리-읽기)
      - [FR-DIR-003 디렉토리 생성](#fr-dir-003-디렉토리-생성)
      - [FR-DIR-004 디렉토리 삭제](#fr-dir-004-디렉토리-삭제)
      - [FR-META-001 메타데이터 조회](#fr-meta-001-메타데이터-조회)
    - [3.2.3 Mount/Unmount Operations (마운트 연산)](#323-mountunmount-operations-마운트-연산)
      - [FR-MNT-001: 마운트 시 파일시스템 복원](#fr-mnt-001-마운트-시-파일시스템-복원)
      - [FR-MNT-002: 마운트 시 미완료 쓰기 무시](#fr-mnt-002-마운트-시-미완료-쓰기-무시)
    - [3.2.4 Storage Management (저장 공간 관리)](#324-storage-management-저장-공간-관리)
      - [FR-STORE-001: 저장 공간 조회](#fr-store-001-저장-공간-조회)
      - [FR-STORE-002: 저장 공간 소진 시 오류 반환](#fr-store-002-저장-공간-소진-시-오류-반환)
  - [3.3 Usability requirements (사용성 요구사항)](#33-usability-requirements-사용성-요구사항)
  - [3.4 Performance requirements (성능 요구사항)](#34-performance-requirements-성능-요구사항)
  - [3.5 Design constraints (설계 제약사항)](#35-design-constraints-설계-제약사항)
    - [OTH-LEG-001: 오픈소스 라이선스](#oth-leg-001-오픈소스-라이선스)
    - [OTH-IMPL-001: 인메모리 쓰기 버퍼 미사용](#oth-impl-001-인메모리-쓰기-버퍼-미사용)
  - [3.6 Software system attributes (소프트웨어 시스템 속성)](#36-software-system-attributes-소프트웨어-시스템-속성)
    - [3.6.1 Reliability and Availability (신뢰성 및 가용성)](#361-reliability-and-availability-신뢰성-및-가용성)
      - [NFR-REL-001: 데이터 손실 방지](#nfr-rel-001-데이터-손실-방지)
  - [3.7 Supporting information (지원 정보)](#37-supporting-information-지원-정보)
    - [OTH-INST-001: 빌드 시스템](#oth-inst-001-빌드-시스템)
- [4. Verification (검증)](#4-verification-검증)
  - [4.1 Verification methods (검증 방법)](#41-verification-methods-검증-방법)
    - [검증 방법 분류](#검증-방법-분류)
    - [검증 매트릭스 (요약)](#검증-매트릭스-요약)
    - [VER-001: 크래시 정합성 검증 (핵심)](#ver-001-크래시-정합성-검증-핵심)
    - [VER-ORACLE-001: write/fsync 검증은 FUSE read 로 진행한다](#ver-oracle-001-writefsync-검증은-fuse-read-로-진행한다)
  - [4.2 Acceptance criteria (인수 기준)](#42-acceptance-criteria-인수-기준)
    - [기능 요구사항 인수 기준](#기능-요구사항-인수-기준)
    - [최종 릴리즈 인수 기준](#최종-릴리즈-인수-기준)
- [5. Appendices (부록)](#5-appendices-부록)
  - [5.1 Assumptions and dependencies (가정 및 의존성)](#51-assumptions-and-dependencies-가정-및-의존성)
    - [가정 (Assumptions)](#가정-assumptions)
    - [의존성 (Dependencies)](#의존성-dependencies)
  - [5.2 Acronyms and abbreviations (약어)](#52-acronyms-and-abbreviations-약어)
  - [5.3 Glossary (용어집)](#53-glossary-용어집)
  - [5.4 Use Cases and Scenarios (사용 사례)](#54-use-cases-and-scenarios-사용-사례)
    - [UC-001: 파일 생성 및 쓰기 (정상 시나리오)](#uc-001-파일-생성-및-쓰기-정상-시나리오)
    - [UC-002: 저장 공간 부족 시 자동 공간 회수](#uc-002-저장-공간-부족-시-자동-공간-회수)
    - [UC-003: 크래시 후 재마운트 복원](#uc-003-크래시-후-재마운트-복원)
  - [5.5 Requirements Traceability Matrix (요구사항 추적 매트릭스)](#55-requirements-traceability-matrix-요구사항-추적-매트릭스)
  - [5.6 Supporting Diagrams (지원 다이어그램)](#56-supporting-diagrams-지원-다이어그램)
    - [Diagram 1: 시스템 아키텍처](#diagram-1-시스템-아키텍처)
  - [문서 개정 이력](#문서-개정-이력)

---

# 1. Introduction (개요)

## 1.1 Purpose (목적)

본 Software Requirements Specification (SRS)는 FUSE(Filesystem in Userspace) 기반 UFFS(Ultra-low-cost Flash File System) 파일시스템의 Crash Consistency 를 중심으로 개발 범위와 테스트 기준을 명확히 설정합니다.
Crash Consistency를 보장할 데이터의 범위를 정하고 어느 시점부터 보장할 것인지 시점의 경계를 명확히 설정합니다.

**정의하는 제품:**
- **제품명**: FUSE 기반 UFFS 파일시스템
- **버전**: 1.0
- **릴리즈**: 첫 번째 안정 버전

## 1.2 Scope (범위)

본 시스템은 UFFS 파일시스템의 파일 입출력 기능을 FUSE 인터페이스를 통해 사용자 공간(Userspace)에서 구현한다. 특히 **크래시 정합성(Crash Consistency)**을 핵심 기능으로 한다. 본 시스템은 전원 차단, 커널 패닉, 또는 파일시스템 프로세스 비정상 종료와 같은 크래시 상황에서도 파일시스템 메타데이터 및 데이터의 일관성을 보장한다.

**주요 특징:**
- **POSIX 호환**: 표준 파일시스템 API 제공
- **크래시 정합성**: 전원 차단 시에도 데이터 일관성 보장 (durability는 명시한 범위 내에서만 보장)

**시스템이 하는 것 (In Scope):**

1. **파일 연산**: 생성, 읽기, 쓰기, 삭제, 동기화(fsync)
2. **디렉토리 연산**: 생성, 읽기, 삭제,  동기화(fsync)
3. **Crash Consistency** : 명시한 범위 내에서 파일 데이터, 메타데이터, 네임스페이스에 대한 crash consistency 보장

**시스템이 하지 않는 것 (Out of Scope):**

1. **플래시 관리**: 배드 블록 관리, 웨어 레벨링
2. **크래시 복구**: 마운트 시 자동 검증 및 복구
3. **오류 처리**: ECC 기반 비트 오류 자동 수정
4. **이식성** : 여러버전에 대한 지원 미제공 (FUSE, 우분투)
5. **성능 최적화**
6. **동시성 지원**

## 1.3 Product overview (제품 개요)

### 1.3.1 Product perspective (제품 관점)

본 시스템은 리눅스 커널의 FUSE 인터페이스를 통해 사용자 공간에서 동작하는 파일시스템입니다.

**시스템 아키텍처:**

```
┌─────────────────────────────────┐
│   Application Layer             │  ← 사용자 애플리케이션
│   (cp, mv, cat, ...)            │
└─────────────────────────────────┘
          ↓ POSIX API (open, read, write, fsync, ...)
┌─────────────────────────────────┐
│   Linux VFS (Kernel Space)      │  ← 가상 파일시스템 계층
└─────────────────────────────────┘
          ↓ FUSE Protocol (ioctl, read/write)
┌─────────────────────────────────┐
│   UFFS Implementation           │  ← 본 시스템 (Userspace)
│   (User Space)                  │
│   • File Management             │
└─────────────────────────────────┘
          ↓ Block I/O (read/write)
┌─────────────────────────────────┐
│   NAND Flash Device             │  ← 저장 장치
│   (Physical Storage)            │
└─────────────────────────────────┘
```

**시스템 경계:**

내부 컴포넌트:
1. **FUSE 어댑터**: VFS와의 인터페이스 제공
2. **파일 관리자**: 파일 생성, 읽기, 쓰기, 삭제
3. **메타데이터 관리자**: 파일 속성 및 시간 정보

외부 인터페이스:
- **상위**: Linux VFS (FUSE 프로토콜)
- **하위**: NAND Flash Device (블록 I/O)
- **사용자**: POSIX 시스템 콜

**기존 시스템과의 관계:**

- **POSIX 파일시스템**: 표준 호환 인터페이스 제공 (부분 호환)
- **기존 UFFS (uffs-reference)**: 핵심 개념 참조, 구현은 독자적
- **리눅스 VFS**: FUSE를 통해 통합
- **ext4, F2FS 등**: 대안 파일시스템 (본 시스템은 임베디드 특화)

**운영 환경:**

호스트 컴퓨터 환경:
- **CPU**: Macbook M4 Pro
- **RAM**: 48GB
- **저장 장치**: SSD 512GB

게스트 컴퓨터 환경:
- **CPU**: ARM64(aarch64)
- **RAM**: 4GB
- **저장 장치**: 12GB

소프트웨어 환경:
- **운영 체제**: Ubuntu 20.04

### 1.3.2 Product functions (제품 기능)

본 시스템은 다음 주요 기능을 제공합니다:

**1. 파일 연산 (File Operations)**
- 파일 열기 (FR-FILE-001)
- 파일 생성 (FR-FILE-002)
- 파일 읽기 (FR-FILE-003)
- 파일 쓰기 (FR-FILE-004)
- 파일 동기화 / fsync (FR-FILE-005)
- 파일 이름 변경 / rename (FR-FILE-006)
- 파일 삭제 (FR-FILE-007)

**2. 디렉토리 연산 (Directory Operations)**
- 디렉토리 동기화 / fsync (FR-DIR-001)
- 디렉토리 읽기 (FR-DIR-002)
- 디렉토리 생성 (FR-DIR-003)
- 디렉토리 삭제 (FR-DIR-004)

**3. 메타데이터 조회 (FR-META-001)**
- 파일 및 디렉토리의 권한, 링크 수, 파일 크기 조회

**4. 크래시 정합성 (Crash Consistency)**
- 전원 차단 시 데이터 일관성 보장
- fsync 성공 시점까지의 파일 데이터 및 디렉토리 엔트리 내구성 보장

### 1.3.3 User characteristics (사용자 특성)

**주요 사용자 유형:**

| 사용자 유형 | 기술 수준 | 사용 목적 | 예상 비율 |
|-----------|---------|----------|----------|
| **임베디드 시스템 개발자** | 고급 (리눅스 시스템 프로그래밍 경험) | 제품 개발에 UFFS 통합 | 40% |
| **파일시스템 연구자** | 전문가 (파일시스템 내부 구조 이해) | 연구 및 교육 목적 | 30% |
| **학생/교육자** | 중급~고급 | 학습 및 실습 | 20% |
| **시스템 관리자** | 중급 (리눅스 시스템 관리 경험) | 시스템 운영 및 유지보수 | 10% |

**사용자 요구사항:**
- **임베디드 개발자**: 신뢰성, 낮은 메모리 사용, 명확한 API
- **연구자**: 상세한 문서, 확장 가능한 아키텍처
- **학생**: 이해하기 쉬운 코드, 교육용 예제
- **관리자**: 간편한 설치, 명확한 에러 메시지

### 1.3.4 Limitations (제한사항)

**기술적 제약:**
1. **FUSE 제약**: 커널 내 파일시스템 대비 성능 오버헤드 존재 (약 10-30%)
2. **메모리 제약**: 임베디드 시스템을 위해 메모리 사용 최소화 필요
3. **POSIX 부분 호환**: 하드 링크, 확장 속성 등 일부 기능 미지원

**법적 제약:**
4. **오픈소스 라이선스**: GPL/MIT/BSD 등 오픈소스 라이선스 준수 필요

## 1.4 Definitions (용어 정의)

| 용어 | 정의 |
|-----|------|
| **FUSE** | Filesystem in Userspace. 사용자 공간에서 파일시스템을 구현할 수 있게 하는 인터페이스 |
| **UFFS** | Ultra-low-cost Flash File System. NAND 플래시 메모리를 위한 경량 파일시스템 |
| **fsync** | 파일 데이터를 저장 장치에 강제로 동기화하는 시스템 콜 |
| **Crash Consistency** | 크래시 정합성. 시스템 크래시나 전원 차단 후에도 파일시스템 일관성을 유지하는 특성 |
| **Metadata** | 메타데이터. 파일/디렉토리의 속성 정보 (크기, 타임스탬프, 권한 등) |
| **Mutex** | Mutual Exclusion. 상호 배제 락. 동시에 하나의 스레드만 임계 영역에 접근하도록 보장하는 동기화 메커니즘 |
| **Global Lock** | 전역 락. 파일시스템 전체에 대해 단일 락을 사용하여 모든 연산을 직렬화하는 동시성 제어 방식 |
| **Atomicity** | 원자성. 연산이 모두 성공하거나 모두 실패하는 특성 (all-or-nothing) |
| **VFS** | Virtual File System. 커널의 파일시스템 추상화 계층 |

---

# 2. References (참조 문서)


**참조:**
1. **표준 문서:** ISO/IEC/IEEE 29148
2. **FUSE Documentation** - libfuse 3.x API Reference
   URL: https://github.com/libfuse/libfuse
3. **UFFS core Library** - https://github.com/rickyzheng/uffs

---

# 3. Specific requirements (상세 요구사항)

**요구사항 작성 형식:**
각 요구사항은 다음 정보를 포함합니다:
- **ID**: 고유 식별자
- **Description**: 요구사항 설명
- **Priority**: Critical / High / Medium / Low
- **Verification**: 검증 방법 (섹션 4과 연계)

## 3.1 External interfaces (외부 인터페이스)

### 3.1.1 User interfaces (사용자 인터페이스)

#### EXT-UI-001: 마운트 명령어
**Description**: 시스템은 표준 FUSE 마운트 명령어를 통해 마운트할 수 있어야 한다.

**Interface**:
```bash
uffs_mount <device_path> <mount_point> [options]
```

**Parameters**:
- `device_path`: 플래시 디바이스 경로 (예: `/dev/mtd0`)
- `mount_point`: 마운트 포인트 (예: `/mnt/uffs`)
- `options`: 마운트 옵션 (예: `-o ro` - 읽기 전용)

**Output**:
- 성공 시: 마운트 완료 메시지
- 실패 시: 에러 메시지 및 상세 정보

**Priority**: Critical

---

#### EXT-UI-002: 언마운트 명령어
**Description**: 시스템은 표준 umount 명령어를 통해 언마운트할 수 있어야 한다.

**Interface**:
```bash
umount <mount_point>
```

**Priority**: Critical

---

### 3.1.2 Software interfaces (소프트웨어 인터페이스)

#### EXT-SW-001: FUSE API
**Description**: 시스템은 libfuse 3.18.1 를 사용하여 VFS와 통신해야 한다.

**Library**:  libfuse 3.18.1

**Key Operations**:
- `fuse_main()`: FUSE 메인 루프
- `fuse_operations` 구조체:
  - `getattr`: FR-META-001과 매핑
  - `open`: FR-FILE-001과 매핑
  - `create`:FR-FILE-002과 매핑
  - `read`: FR-FILE-003와 매핑
  - `write`: FR-FILE-004과 매핑
  - `fsync`: FR-FILE-005와 매핑
  - `rename`: FR-FILE-006과 매핑
  - `unlink`: FR-FILE-007과 매핑
  - `fsync(디렉토리)`: FR-DIR-001과 매핑
  - `readdir`: FR-DIR-002과 매핑
  - `mkdir`: FR-DIR-003과 매핑
  - `rmdir`: FR-DIR-004과 매핑
  - `statfs`: FR-STORE-001과 매핑

**Priority**: Critical

---

## 3.2 Functions (기능 요구사항)

### 3.2.1 File Operations (파일 연산)

#### FR-FILE-001 파일 열기
**Description**: 시스템은 파일을 열 수 있어야한다.

**Priority**: Critical

#### FR-FILE-002: 파일 생성
**Description**: 시스템은 사용자가 지정한 경로에 새로운 빈 파일을 생성할 수 있어야 한다.

**Priority**: Critical

**Verification**: 단위 테스트, 통합 테스트 (섹션 4.1 참조)

---

#### FR-FILE-003: 파일 읽기
**Description**: 시스템은 파일의 지정된 오프셋부터 지정된 크기만큼 데이터를 읽을 수 있어야 한다.

**Priority**: Critical

**Verification**: 단위 테스트, 성능 테스트

---

#### FR-FILE-004: 파일 쓰기
**Description**: 시스템은 파일의 지정된 오프셋에 데이터를 쓸 수 있어야 한다.

**Priority**: Critical

**Verification**: 단위 테스트, 통합 테스트

---

#### FR-FILE-004-1: 대용량 파일 쓰기 지원

**Description**: 시스템은 단일 파일에 대해 최소 64KB 이상의 데이터를 저장하고 읽을 수 있어야 한다. 데이터 양이 첫 번째 파일 저장 블록의 용량을 초과할 경우, 추가 블록을 할당하여 연속적으로 저장해야 한다.

**Acceptance Criteria**:
- 64KB 크기의 데이터를 write → fsync → read 시 읽은 데이터와 쓴 데이터가 바이트 단위로 일치한다.

**Priority**: Critical

**Verification**: 단위 테스트 (TC-FILE-004-1)

---

#### FR-FILE-004-2: 공간 부족 시 자동 공간 회수

**Description**: 시스템은 전체 저장 용량이 소진되지 않은 상태에서 개별 저장 단위(블록)의 공간 부족을 이유로 쓰기 연산이 실패해서는 안 된다. 시스템은 자동으로 회수 가능한 공간을 확보하여 쓰기를 완료해야 하며, 이 과정에서 이전에 읽을 수 있던 파일 데이터는 보존되어야 한다.

**Acceptance Criteria**:
- 특정 저장 블록이 가득 찬 상태에서 해당 파일에 추가 write → fsync → 성공(반환값 0)
- 공간 회수 전후로 기존 파일의 read 결과가 동일하다.

**Priority**: Critical

**Verification**: 단위 테스트 (TC-GC-001)

---

#### FR-FILE-004-3: 선제적 공간 회수 (임계치 기반)

**Description**: 시스템의 여유 저장 블록 수가 내부 임계치 이하로 감소하면, 시스템은 다음 쓰기 연산 전에 자동으로 회수 가능한 블록을 확보해야 한다. 이 과정에서 이전에 읽을 수 있던 파일 데이터는 변경되어서는 안 된다.

**Acceptance Criteria**:
- 여유 블록이 임계치 근처인 상태에서 write → fsync → 성공(반환값 0)
- 선제적 공간 회수 전후로 기존 파일의 read 결과가 동일하다.

**Priority**: Critical

**Verification**: 단위 테스트 (TC-GC-002)

---

#### FR-FILE-004-4: 부분 쓰기 데이터 보존

**Description**: write() 요청 범위가 저장 최소 단위(페이지)의 경계와 정렬되지 않거나 최소 단위보다 작은 경우, 해당 파일에서 요청한 범위 외에 위치한 기존 데이터는 해당 write() 완료 후에도 변경되지 않아야 한다.

**Acceptance Criteria**:
- 파일의 특정 오프셋에 1B를 write 한 후 read 시, 쓴 바이트는 새 값이고 나머지 바이트는 write 이전 값과 동일하다.

**Priority**: Critical

**Verification**: 단위 테스트 (TC-FILE-004-4)

---

#### FR-FILE-005: 파일 동기화 (fsync)

**Description**: 시스템은 fsync(fd) 성공 시점 기준으로 해당 파일 객체 (file object) 의 데이터 및 파일 메타데이터 내구성을 보장해야한다.

**Requirements**
- FR-FILE-005-1 파일 데이터 내구성: fsync(fd)가 0을 반환하면, 해당 호출 이전에 같은 파일디스크립터에 대해 완료된 write, pwrite로 인해 변경된 파일데이터 내용은, 이후 비정상 종료 및 재마운트 후에도 동일하게 관측되어야한다.
- FR-FILE-005-2 (파일 사이즈 내구성): fsync(fd) 가 0을 반환하면, 해당 호출 이전에 완료된 쓰기로 인해 변경된 파일 크기는 이후 비정상 종료 및 재마운트 후에도 동일하게 관측되어야한다.
- FR-FILE-005-3 내구성 보장의 경계: fsync의 내구성 보장은 파일 객체 자체에 한정되며, 파일의 이름/경로 관측가능성은 FR-DIR-001에 의해 정의된다.

**Priority**: Critical

#### FR-FILE-006: 파일 이름 변경 (rename)

**Description**: 시스템은 지정된 파일의 이름 변경을 지원 해야한다.
**Priority**: High

#### FR-FILE-007: 파일 삭제

**Description**: 시스템은 지정된 파일을 삭제할 수 있다.

**Priority**: Medium

### 3.2.2 Directory Operations (디렉토리 연산)

#### FR-DIR-001: 디렉토리 동기화 (fsync)
**Description**: 시스템은 fsync(dirfd) 성공 시점 기준으로 해당 디렉토리의 이름 매핑 변경 내구성을 보장해야한다.

**Requirements**
- FR-DIR-001-1 이름 매핑 변화 내구성: fsync(dirfd) 가 0을 반환하면, 해당 호출 이전에 완료된 디렉토리 엔트리 변경은 이후 비정상 종료 및 재마운트 후에도 동일하게 관측되어야 한다.
- FR-DIR-001-2 fsync 없는 범위에 대한 비보장: FR-DIR-001이 충족되지 않은 경우 (fsync(dirfd) 성공이 없었던 경우, 비정상 종료 및 재마운트 후에 디렉토리 엔트리 변경 결과가 유지되는지 여부는 보장하지 않는다.

**Notes**
UFFS 파일시스템은 디렉토리 구조가 스캔 기반 메타레코드로 재구성되므로 디렉토리 엔트리 블록으로 판단하지 않고 재마운트 후 관측되는 name과 object 관계의 동일성으로 요구사항을 판단한다.

**Priority**: High

**Verification**: Crash consistency 테스트


#### FR-DIR-002 디렉토리 읽기
**Priority**: High
**Description**: 시스템은 지정된 디렉토리 내 파일 및 하위 디렉토리의 이름 목록을 반환해야 한다.

#### FR-DIR-003 디렉토리 생성

**Description**: 시스템은 지정된 경로에 빈 디렉토리를 생성해야한다
**Priority**: High

#### FR-DIR-004 디렉토리 삭제

**Description**: 시스템은 지정된 이름의 디렉토리를 삭제할 수 있다.
**Priority**: Medium

#### FR-META-001 메타데이터 조회

**Description**: 시스템은 파일, 폴더의 메타데이터를 조회할 수 있다. (권한, 링크, 파일의 길이)
**Priority**: Medium

---

### 3.2.3 Mount/Unmount Operations (마운트 연산)

#### FR-MNT-001: 마운트 시 파일시스템 복원

**Description**: 시스템은 마운트 시 플래시 저장소를 스캔하여 이전에 성공적으로 동기화(fsync)가 완료된 파일 및 디렉토리 데이터를 복원해야 한다. 마운트 완료 후 사용자는 이전 세션에서 fsync가 완료된 파일을 읽을 수 있어야 한다.

**Acceptance Criteria**:
- 파일 생성 → fsync → 파일시스템 프로세스 종료 → 재마운트 → 동일 파일 read 성공

**Priority**: Critical

**Verification**: 통합 테스트 (TC-MNT-001)

---

#### FR-MNT-002: 마운트 시 미완료 쓰기 무시

**Description**: 시스템은 마운트 시 fsync 완료 이전에 중단된 쓰기 작업의 결과를 유효한 파일 데이터로 인식하지 않아야 한다. 마운트 후 해당 쓰기가 관측되더라도 그 내용은 정의되지 않으며 시스템은 이로 인한 파일시스템 불일치 상태가 되어서는 안 된다.

**Rationale**: 쓰기 중단(전원 차단 등)으로 인해 부분적으로 기록된 데이터가 유효한 데이터로 취급되면 파일시스템 일관성이 깨진다. 미완료 쓰기를 무시하는 것이 크래시 정합성의 핵심 메커니즘이다.

**Acceptance Criteria**:
- 파일 write 직후 (fsync 이전) 프로세스 강제 종료 → 재마운트 → 해당 write 이전 상태가 관측되거나 파일시스템이 정상 동작 상태를 유지한다.

**Priority**: Critical

**Verification**: Crash consistency 테스트 (TC-CRASH-MNT-001)

---

### 3.2.4 Storage Management (저장 공간 관리)

#### FR-STORE-001: 저장 공간 조회

**Description**: 시스템은 마운트된 파일시스템의 총 저장 용량과 현재 사용 가능한 여유 용량을 조회하는 기능을 제공해야 한다. 이 정보는 표준 POSIX 인터페이스(statvfs)를 통해 접근할 수 있어야 한다.

**Priority**: Medium

**Verification**: 단위 테스트 — statvfs() 호출 후 반환된 여유 블록 수가 실제 파일 생성 가능 횟수와 일관성이 있음을 확인

---

#### FR-STORE-002: 저장 공간 소진 시 오류 반환

**Description**: 시스템의 저장 공간이 완전히 소진된 상태에서 write() 또는 파일 생성 요청이 발생하면 시스템은 ENOSPC(저장 공간 부족) 오류를 반환해야 한다. 이 상황에서 기존에 존재하던 파일 데이터는 손상되지 않아야 한다.

**Acceptance Criteria**:
- 저장 공간이 100% 사용된 상태에서 write() → errno == ENOSPC
- ENOSPC 반환 이후 기존 파일 read → 이전 데이터와 동일

**Priority**: High

**Verification**: 단위 테스트 (TC-STORE-002)

---

## 3.3 Usability requirements (사용성 요구사항)

해당 없음 — 본 프로젝트에서 사용성 요구사항은 별도로 정의하지 않는다.

---

## 3.4 Performance requirements (성능 요구사항)

해당 없음 — 본 프로젝트에서 성능 요구사항의 정의와 테스트는 제외한다 (1.2 Scope 참조).

---


## 3.5 Design constraints (설계 제약사항)

### OTH-LEG-001: 오픈소스 라이선스
**Description**: 시스템은 오픈소스 라이선스를 준수해야 한다.

**License Options**:
- MIT License
- BSD License

**Requirements**:
- 소스 코드 공개
- 라이선스 고지 포함
- 저작권 표시

**Priority**: Critical

---

### OTH-IMPL-001: 인메모리 쓰기 버퍼 미사용

**Description**: 시스템은 파일 데이터를 위한 인메모리 쓰기 버퍼(dirty page cache)를 사용하지 않는다. write() 호출 시 데이터는 즉시 영구 저장소에 기록된다.

**Rationale**: 인메모리 버퍼를 제거하면 fsync() 없이도 단일 write 호출의 플래시 반영 시점을 예측 가능하게 만들며, 임베디드 환경의 제한된 메모리 사용에 부합한다. 단, 파일 메타데이터(크기 등)의 내구성 보장 경계는 FR-FILE-005(fsync)에 의해 정의된다.

**Priority**: High

---

## 3.6 Software system attributes (소프트웨어 시스템 속성)

### 3.6.1 Reliability and Availability (신뢰성 및 가용성)

#### NFR-REL-001: 데이터 손실 방지
**Description**: 시스템은 fsync 호출 후 전원 차단 시 데이터 손실이 발생하지 않아야 한다.

**Measurement**:
- 테스트: fsync 후 즉시 power-cut
- 측정 항목: 데이터 손실 횟수
- 목표: 0건 (100% 보장)

**Priority**: Critical

**Verification**: fsync durability 테스트

---

## 3.7 Supporting information (지원 정보)

### OTH-INST-001: 빌드 시스템
**Description**: 시스템은 CMake 기반 빌드 시스템을 제공해야 한다.

**Build Commands**:
```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

**Priority**: High

---

# 4. Verification (검증)

## 4.1 Verification methods (검증 방법)

### 검증 방법 분류

| 검증 방법 | 설명 | 적용 요구사항 예시 |
|----------|------|-------------------|
| **Unit Test** | 개별 모듈의 기능 검증 | FR-FILE-001~007, FR-DIR-002~004, FR-META-001 |
| **Crash Test** | 전원 차단 시뮬레이션 후 재마운트하여 데이터 정합성 검증 | FR-FILE-005, FR-DIR-001 |
| **Durability Test (Stress Test)** | 반복 power-cut 시나리오(1,000회+)로 내구성 및 데이터 손실 부재 검증 | NFR-REL-001 |

### 검증 매트릭스 (요약)

| 요구사항 ID | 검증 방법 | 도구/기법 |
|-----------|----------|----------|
| FR-FILE-001~007 | Unit Test | Google Test |
| FR-DIR-001 | Crash Test | Power-cut simulator |
| FR-DIR-002~004 | Unit Test | Google Test |
| FR-META-001 | Unit Test | Google Test |
| NFR-REL-001 | Stress Test, Crash Test | Power-cut simulator (1,000회+) |

### VER-001: 크래시 정합성 검증 (핵심)
**Target Requirements**: FR-FILE-005, FR-DIR-001, NFR-REL-001

1. **Recovery Verification**:
   - fsync(file) 이후 파일 데이터 손실 검증
   - rename 이후 파일 덮어쓰기 확인
   - fsync(dir) 이후 폴더-파일 매핑구조 확인

2. **Success Criteria**:
   - fsync(file) 이후 파일 데이터 손실 0건
   - rename 이후 메타데이터 손실 0건
   - fsync(dir) 이후 부모-자식 매핑 구조 오류 0건
   - 메타데이터 일관성 100%

**Priority**: Critical

---

### VER-ORACLE-001: write/fsync 검증은 FUSE read 로 진행한다

**Target Requirements**: FR-FILE-004, FR-FILE-005

**Verification Approach**:
write/fsync 연산의 내구성 검증 시, 검증 오라클(기준값)은 원시 플래시 블록 직접 읽기가 아닌 **FUSE read 인터페이스**를 통해 획득한다.
즉, fsync 이후 전원 차단 및 재마운트를 수행한 뒤, 동일 파일에 대해 FUSE read를 호출하여 반환된 데이터를 쓰기 이전에 기록한 기대값과 비교한다.

**Rationale**:
- 플래시 블록 레이아웃은 UFFS 내부 구현에 종속되므로 테스트가 구현 세부사항에 결합되지 않도록 한다.
- 사용자 관점에서의 end-to-end 정합성(쓰기 → fsync → 크래시 → 재마운트 → 읽기)을 검증한다.

**Test Procedure**:
1. 대상 파일에 알려진 데이터를 write/pwrite로 기록한다.
2. fsync(fd)를 호출하고 반환값이 0임을 확인한다.
3. 파일시스템 프로세스를 비정상 종료(SIGKILL 또는 power-cut 시뮬레이션)한다.
4. 파일시스템을 재마운트한다.
5. FUSE read를 통해 동일 파일을 읽어 기록한 데이터와 바이트 단위로 비교한다.

**Success Criteria**:
- FUSE read로 반환된 데이터가 fsync 이전 write로 기록한 데이터와 100% 일치해야 한다.
- 파일 크기 역시 fsync 완료 시점의 크기와 동일해야 한다.

**Priority**: Critical

---

## 4.2 Acceptance criteria (인수 기준)

### 기능 요구사항 인수 기준

**AC-001: 파일 연산 인수 기준**
- [ ] 모든 파일 연산 단위 테스트 통과 (FR-FILE-001~007)

**AC-002: 크래시 정합성 인수 기준 (핵심)**
- [ ] fsync 이후 데이터 손실 0건
- [ ] 메타데이터 일관성 100%

### 최종 릴리즈 인수 기준

**AC-003: 릴리즈 준비 완료**
- [ ] 모든 기능 요구사항 인수 기준 통과 (AC-001~002)
- [ ] 문서화 완료 (SRS, 설계서, 사용자 매뉴얼, API 문서)
- [ ] 라이선스 고지 및 저작권 표시 완료

---

# 5. Appendices (부록)

## 5.1 Assumptions and dependencies (가정 및 의존성)

### 가정 (Assumptions)

1. **하드웨어 가정**: NAND 플래시 디바이스가 기본적인 ECC를 지원한다고 가정
2. **운영 체제 가정**: 리눅스 커널 4.x 이상에서 FUSE 3.x 사용 가능
3. **사용 패턴 가정**: 대부분의 파일 크기가 1MB 이하 (임베디드 환경)
4. **전원 가정**: 전원 차단은 언제든 발생할 수 있음 (예측 불가)

### 의존성 (Dependencies)

**외부 라이브러리:**
1. **libfuse 3.x**: FUSE 인터페이스 제공
2. **libc**: 표준 C 라이브러리

**운영 체제:**
3. **Linux Kernel**: FUSE 모듈이 활성화된 커널

**하드웨어:**
4. **NAND Flash**: 블록 디바이스 인터페이스 제공

## 5.2 Acronyms and abbreviations (약어)

| 약어 | 전체 명칭 |
|-----|----------|
| **API** | Application Programming Interface |
| **SRS** | Software Requirements Specification |
| **I/O** | Input/Output |

## 5.3 Glossary (용어집)

| 용어 | 정의 |
|-----|------|
| **Atomic Operation** | 원자적 연산. 모두 성공하거나 모두 실패하는 연산 (중간 상태 없음) |
| **Block Device** | 블록 단위로 데이터를 읽고 쓰는 저장 장치 |
| **Cache** | 캐시. 자주 사용되는 데이터를 빠르게 접근하기 위해 메모리에 저장 |
| **Checksum** | 체크섬. 데이터 무결성을 검증하기 위한 해시 값 |
| **Copy-on-Write (COW)** | 쓰기 시 복사. 데이터 수정 시 원본을 보존하고 복사본을 수정하는 기법 |
| **Crash Consistency** | 크래시 정합성. 시스템 크래시 후에도 데이터 일관성을 유지하는 특성 |
| **Data Race** | 데이터 레이스. 둘 이상의 스레드가 동기화 없이 동일 메모리를 동시에 접근하며, 그 중 하나 이상이 쓰기인 경우 발생하는 정의되지 않은 동작 |
| **Deadlock** | 데드락. 둘 이상의 스레드가 서로 상대방이 보유한 자원을 대기하며 영구적으로 진행이 중단되는 상태 |
| **Dirty Page** | 더티 페이지. 메모리에서 수정되었지만 디스크에 기록되지 않은 데이터 |
| **ECC (Error Correcting Code)** | 오류 정정 코드. 비트 오류를 감지하고 수정하는 코드 |
| **Erase Count** | 삭제 횟수. 플래시 블록이 삭제된 횟수 (수명 측정에 사용) |
| **fsync** | 파일 동기화 시스템 콜. 메모리 버퍼를 디스크에 강제로 쓰기 |
| **FUSE (Filesystem in Userspace)** | 사용자 공간에서 파일시스템을 구현할 수 있게 하는 인터페이스 |
| **Garbage Collection** | 가비지 컬렉션. 삭제된 데이터 블록을 회수하여 여유 공간 확보 |
| **Inode** | Index node. 파일의 메타데이터를 저장하는 자료구조 |
| **Journaling** | 저널링. 메타데이터 변경사항을 로그에 기록하여 복구를 용이하게 하는 기법 |
| **Metadata** | 메타데이터. 데이터에 대한 데이터 (파일 크기, 시간, 권한 등) |
| **Mutex (Mutual Exclusion)** | 뮤텍스. 동시에 하나의 스레드만 임계 영역에 접근하도록 보장하는 동기화 프리미티브 |
| **MTBF (Mean Time Between Failures)** | 평균 고장 간격. 시스템의 신뢰성 지표 |
| **NAND Flash** | NAND 플래시 메모리. 비휘발성 저장 장치 |
| **Page** | 페이지. 플래시 메모리의 읽기/쓰기 최소 단위 (일반적으로 2KB~4KB) |
| **POSIX** | Portable Operating System Interface. UNIX 표준 API |
| **Power-Cut Simulation** | 전원 차단 시뮬레이션. 크래시 정합성 테스트 기법 |
| **Superblock** | 슈퍼블록. 파일시스템의 전역 메타정보를 저장하는 블록 |
| **UFFS (Ultra-low-cost Flash File System)** | NAND 플래시용 경량 파일시스템 |
| **VFS (Virtual File System)** | 가상 파일시스템. 리눅스 커널의 파일시스템 추상화 계층 |
| **Write-Ahead Logging (WAL)** | 선행 기록 로깅. 데이터 변경 전에 로그를 기록하여 복구를 보장하는 기법 |
| **헤더 블록 (Header Block)** | UFFS에서 파일 당 1개 할당되는 블록. 파일 메타데이터(이름, 크기 등)와 초기 파일 데이터를 함께 저장 |
| **데이터 블록 (Data Block)** | 헤더 블록 용량을 초과한 파일 데이터를 저장하기 위해 추가 할당되는 블록 |
| **Victim Block** | 가비지 컬렉션 시 회수 대상으로 선정된 블록. 회수 가능한 공간(무효 데이터)이 가장 많은 블록을 선정 |
| **Garbage Collection (GC)** | 가비지 컬렉션. 무효화된 저장 공간을 회수하여 새 쓰기를 위한 여유 공간을 확보하는 작업 |
| **Write Durability Boundary** | 쓰기 내구성 경계. 시스템이 데이터 영속성을 보장하는 시작 시점. 본 시스템에서는 fsync() 성공 반환 시점 |

## 5.4 Use Cases and Scenarios (사용 사례)

### UC-001: 파일 생성 및 쓰기 (정상 시나리오)

**Actor**: 사용자 애플리케이션

**Preconditions**:
- 파일시스템이 마운트됨
- 충분한 여유 공간 존재

**Main Flow**:
1. 애플리케이션이 `open("/mnt/uffs/test.txt", O_CREAT|O_WRONLY)` 호출
2. 시스템이 새 파일 생성 (FR-FILE-002)
3. 애플리케이션이 `write(fd, data, size)` 호출
4. 시스템이 데이터를 영구 저장소에 기록 (FR-FILE-004, OTH-IMPL-001)
5. 애플리케이션이 `fsync(fd)` 호출
6. 시스템이 파일 메타데이터(크기 등)를 플래시에 동기화하여 내구성 보장 (FR-FILE-005)
7. 애플리케이션이 `rename("/mnt/uffs/test.txt", "/mnt/uffs/final.txt")` 호출
8. 파일 이름 성공적으로 변경 (FR-FILE-006)
9. 애플리케이션이 `fsync(dirfd)` 호출
10. 부모-자식 매핑 구조 성공적으로 동기화 (FR-DIR-001)

**Postconditions**:
- 파일이 생성되고 데이터가 안전하게 저장됨
- 전원 차단 시에도 fsync 시점까지의 데이터는 보장됨

**Related Requirements**: FR-FILE-002, FR-FILE-004, FR-FILE-005, FR-FILE-006, FR-DIR-001

---

### UC-002: 저장 공간 부족 시 자동 공간 회수

**Actor**: 사용자 애플리케이션

**Preconditions**:
- 파일시스템이 마운트됨
- 저장 공간이 거의 소진된 상태 (여유 블록이 임계치 이하)

**Main Flow**:
1. 애플리케이션이 기존 파일에 `write(fd, data, size)` 호출
2. 시스템이 내부적으로 가용 공간 부족을 감지
3. 시스템이 자동으로 회수 가능한 공간을 확보 (FR-FILE-004-2, FR-FILE-004-3)
4. 시스템이 쓰기를 완료하고 성공 반환
5. 애플리케이션이 `fsync(fd)` 호출
6. 시스템이 메타데이터 동기화 완료 (FR-FILE-005)

**Alternative Flow** (저장 공간 완전 소진):
- 3단계에서 회수 가능한 공간이 없으면 시스템이 ENOSPC 반환 (FR-STORE-002)

**Postconditions**:
- 쓰기 성공 시: write한 데이터가 저장되고 기존 파일 데이터는 보존됨
- ENOSPC 시: 기존 파일 데이터는 변경되지 않음

**Related Requirements**: FR-FILE-004-2, FR-FILE-004-3, FR-FILE-005, FR-STORE-002

---

### UC-003: 크래시 후 재마운트 복원

**Actor**: 시스템 (자동)

**Preconditions**:
- 파일시스템이 이전에 마운트되어 사용 중이었음
- 파일시스템 프로세스가 비정상 종료됨 (전원 차단, SIGKILL 등)

**Main Flow** (fsync 이전 크래시):
1. 크래시 이전: 애플리케이션이 `write(fd, data)` 완료, `fsync()` 미호출
2. 크래시 발생
3. 재마운트 시 시스템이 플래시 저장소 스캔 (FR-MNT-001)
4. 시스템이 미완료 쓰기를 유효한 데이터로 인식하지 않음 (FR-MNT-002)
5. 파일이 write 이전 상태 또는 파일시스템이 일관된 상태로 복원됨

**Alternative Flow** (fsync 이후 크래시):
1. 크래시 이전: `write(fd, data)` + `fsync()` 모두 완료
2. 크래시 발생
3. 재마운트 시 시스템이 fsync 완료된 데이터를 복원 (FR-MNT-001)
4. 파일 read 시 fsync 시점의 데이터가 반환됨 (FR-FILE-005)

**Postconditions**:
- 파일시스템이 일관된 상태로 마운트됨
- fsync 완료된 데이터는 보존, 미완료 데이터는 관측되지 않음

**Related Requirements**: FR-MNT-001, FR-MNT-002, FR-FILE-005, NFR-REL-001

---

## 5.5 Requirements Traceability Matrix (요구사항 추적 매트릭스)

| 요구사항 ID | 요구사항 이름 | 우선순위 | 검증 방법 | 테스트 케이스 ID | 상태 |
|-----------|-------------|---------|----------|-----------------|------|
| FR-FILE-001 | 파일 열기 | Critical | Unit Test | TC-FILE-001 | TBD |
| FR-FILE-002 | 파일 생성 | Critical | Unit Test | TC-FILE-002 | TBD |
| FR-FILE-003 | 파일 읽기 | Critical | Unit Test | TC-FILE-003 | TBD |
| FR-FILE-004 | 파일 쓰기 | Critical | Unit Test | TC-FILE-004 | TBD |
| FR-FILE-004-1 | 대용량 파일 쓰기 지원 | Critical | Unit Test | TC-FILE-004-1 | TBD |
| FR-FILE-004-2 | 공간 부족 시 자동 공간 회수 | Critical | Unit Test | TC-GC-001 | TBD |
| FR-FILE-004-3 | 선제적 공간 회수 (임계치 기반) | Critical | Unit Test | TC-GC-002 | TBD |
| FR-FILE-004-4 | 부분 쓰기 데이터 보존 | Critical | Unit Test | TC-FILE-004-4 | TBD |
| FR-FILE-005 | 파일 동기화 (fsync) | Critical | Crash Test | TC-CRASH-005 | TBD |
| FR-FILE-006 | 파일 이름 변경 (rename) | High | Unit Test | TC-FILE-006 | TBD |
| FR-FILE-007 | 파일 삭제 | Medium | Unit Test | TC-FILE-007 | TBD |
| FR-DIR-001 | 디렉토리 동기화 (fsync) | High | Crash Test | TC-CRASH-DIR-001 | TBD |
| FR-DIR-002 | 디렉토리 읽기 | High | Unit Test | TC-DIR-002 | TBD |
| FR-DIR-003 | 디렉토리 생성 | High | Unit Test | TC-DIR-003 | TBD |
| FR-DIR-004 | 디렉토리 삭제 | Medium | Unit Test | TC-DIR-004 | TBD |
| FR-META-001 | 메타데이터 조회 | Medium | Unit Test | TC-META-001 | TBD |
| FR-MNT-001 | 마운트 시 파일시스템 복원 | Critical | 통합 테스트 | TC-MNT-001 | TBD |
| FR-MNT-002 | 마운트 시 미완료 쓰기 무시 | Critical | Crash Test | TC-CRASH-MNT-001 | TBD |
| FR-STORE-001 | 저장 공간 조회 | Medium | Unit Test | TC-STORE-001 | TBD |
| FR-STORE-002 | 저장 공간 소진 시 오류 반환 | High | Unit Test | TC-STORE-002 | TBD |
| **NFR-REL-001** | **데이터 손실 방지** | **Critical** | **Durability Test** | **TC-REL-001** | **TBD** |

**Note**: TBD = To Be Determined (구현 단계에서 결정)

**상태 값**:
- TBD: 구현 전
- In Progress: 구현 중
- Implemented: 구현 완료
- Verified: 검증 완료

## 5.6 Supporting Diagrams (지원 다이어그램)

### Diagram 1: 시스템 아키텍처

```
┌─────────────────────────────────────────────────────────┐
│                   Application Layer                      │
│         (user applications: cp, cat, vim, ...)          │
└─────────────────────────────────────────────────────────┘
                           ↓
                   POSIX System Calls
          (open, read, write, fsync, stat, ...)
                           ↓
┌─────────────────────────────────────────────────────────┐
│            Linux VFS (Kernel Space)                      │
│         (Virtual File System Interface)                 │
└─────────────────────────────────────────────────────────┘
                           ↓
                    FUSE Protocol
                (ioctl, read/write)
                           ↓
┌─────────────────────────────────────────────────────────┐
│          UFFS Implementation (User Space)                │
│  ┌──────────────┬──────────────┬──────────────┐        │
│  │ File Manager │ Dir Manager  │ Meta Manager │        │
│  └──────────────┴──────────────┴──────────────┘        │
└─────────────────────────────────────────────────────────┘
                           ↓
                    Block I/O API
            (read_page, write_page, erase_block)
                           ↓
┌─────────────────────────────────────────────────────────┐
│              NAND Flash Device                           │
│           (Physical Storage Medium)                      │
└─────────────────────────────────────────────────────────┘
```

---

## 문서 개정 이력

| 버전 | 날짜 | 작성자 | 변경 내용 |
|-----|------|--------|----------|
| 1.0 | 2026.03.04 | 임재형 | 초안 작성 |
| 1.1 | 2026.03.11 | 임재형 | 1차 구현 기반 파일 쓰기 요구사항 추가 (FR-FILE-004-1~6 초안) |
| 1.2 | 2026.03.11 | 임재형 | IEEE 29148 준거 보완: FR-FILE-004-1~4 형식 정규화; FR-MNT-001~002, FR-STORE-001~002 신규 추가; OTH-IMPL-001 설계 제약 추가; UC-002~003, RTM 갱신, 용어집 보완 |

---

**문서 끝 (End of SRS)**
