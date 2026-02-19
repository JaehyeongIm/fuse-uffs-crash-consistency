# FUSE 기반 UFFS 파일시스템
## Software Requirements Specification (SRS)

**문서 표준**: ISO/IEC/IEEE 29148:2018
**버전(Version)**: 1.0
**작성일(Date)**: 2026.02.16
**작성자(Author)**: 임재형
**상태(Status)**: 초안

---

## 문서 정보

**본 문서의 목적:**
- 관측가능한 요구사항을 정의하여 검증 가능성을 확보한다
- write -> fsync(file) -> rename -> fsync(dir) 패턴을 사용하여 crash 에 대한 durability의 범위를 명시하고 이에 대한 crash consistency를 테스트한다.

## 목차

- [FUSE 기반 UFFS 파일시스템](#fuse-기반-uffs-파일시스템)
  - [Software Requirements Specification (SRS)](#software-requirements-specification-srs)
  - [문서 정보](#문서-정보)
  - [목차](#목차)
- [1. Introduction (개요)](#1-introduction-개요)
  - [1.1 Purpose (목적)](#11-purpose-목적)
  - [1.2 Scope (범위)](#12-scope-범위)
    - [제품 개요](#제품-개요)
    - [시스템이 하는 것 (In Scope)](#시스템이-하는-것-in-scope)
    - [시스템이 하지 않는 것 (Out of Scope)](#시스템이-하지-않는-것-out-of-scope)
    - [비즈니스 목표](#비즈니스-목표)
  - [1.3 References (참조 문서)](#13-references-참조-문서)
    - [프로젝트 내부 문서](#프로젝트-내부-문서)
    - [참조 구현](#참조-구현)
  - [1.5 Definitions, Acronyms, and Abbreviations (용어 정의 및 약어)](#15-definitions-acronyms-and-abbreviations-용어-정의-및-약어)
    - [용어 정의](#용어-정의)
    - [약어](#약어)
- [2. Overall Description (전체 설명)](#2-overall-description-전체-설명)
  - [2.1 System or Product Perspective (시스템 관점)](#21-system-or-product-perspective-시스템-관점)
    - [시스템 아키텍처](#시스템-아키텍처)
    - [시스템 경계](#시스템-경계)
    - [기존 시스템과의 관계](#기존-시스템과의-관계)
  - [2.2 System or Product Functions (주요 기능)](#22-system-or-product-functions-주요-기능)
    - [1. 파일 시스템 기본 연산](#1-파일-시스템-기본-연산)
    - [2. 크래시 정합성 (Crash Consistency) ⭐](#2-크래시-정합성-crash-consistency-)
  - [2.3 User Characteristics (사용자 특성)](#23-user-characteristics-사용자-특성)
    - [주요 사용자 유형](#주요-사용자-유형)
    - [사용자 요구사항](#사용자-요구사항)
  - [2.4 Constraints, Assumptions, and Dependencies (제약사항, 가정, 의존성)](#24-constraints-assumptions-and-dependencies-제약사항-가정-의존성)
    - [제약사항 (Constraints)](#제약사항-constraints)
    - [가정 (Assumptions)](#가정-assumptions)
    - [의존성 (Dependencies)](#의존성-dependencies)
  - [2.5 Operational Environment (운영 환경)](#25-operational-environment-운영-환경)
    - [하드웨어 환경](#하드웨어-환경)
    - [소프트웨어 환경](#소프트웨어-환경)
- [3. Functional Requirements (기능 요구사항)](#3-functional-requirements-기능-요구사항)
  - [3.1 File Operations (파일 연산)](#31-file-operations-파일-연산)
    - [FR-FILE-001 파일 열기](#fr-file-001-파일-열기)
    - [FR-FILE-002: 파일 생성](#fr-file-002-파일-생성)
    - [FR-FILE-003: 파일 읽기](#fr-file-003-파일-읽기)
    - [FR-FILE-004: 파일 쓰기](#fr-file-004-파일-쓰기)
    - [FR-FILE-005: 파일 동기화 (fsync)](#fr-file-005-파일-동기화-fsync)
    - [FR-FILE-006: 파일 이름 변경 (rename)](#fr-file-006-파일-이름-변경-rename)
    - [FR-FILE-007: 파일 삭제](#fr-file-007-파일-삭제)
    - [FR-FILE-008: 파일, 폴더 동시성 지원](#fr-file-008-파일-폴더-동시성-지원)
  - [3.2 Directory Operations (파일 연산)](#32-directory-operations-파일-연산)
    - [FR-DIR-001: 디렉토리 동기화 (fsync)](#fr-dir-001-디렉토리-동기화-fsync)
    - [FR-DIR-002 디렉토리 읽기](#fr-dir-002-디렉토리-읽기)
    - [FR-DIR-003 디렉토리 생성](#fr-dir-003-디렉토리-생성)
    - [FR-DIR-004 디렉토리 삭제](#fr-dir-004-디렉토리-삭제)
    - [FR-META-001 메타데이터 조회](#fr-meta-001-메타데이터-조회)
  - [3.3 Concurrency Requirements (동시성 요구사항)](#33-concurrency-requirements-동시성-요구사항)
    - [FR-CONC-001: 동시 접근 안전성](#fr-conc-001-동시-접근-안전성)
- [4. Non-Functional Requirements (비기능 요구사항)](#4-non-functional-requirements-비기능-요구사항)
  - [4.1 Reliability and Availability (신뢰성 및 가용성)](#41-reliability-and-availability-신뢰성-및-가용성)
    - [NFR-REL-001: 데이터 손실 방지](#nfr-rel-001-데이터-손실-방지)
- [5. External Interface Requirements (외부 인터페이스 요구사항)](#5-external-interface-requirements-외부-인터페이스-요구사항)
  - [5.1 User Interfaces (사용자 인터페이스)](#51-user-interfaces-사용자-인터페이스)
    - [EXT-UI-001: 마운트 명령어](#ext-ui-001-마운트-명령어)
    - [EXT-UI-002: 언마운트 명령어](#ext-ui-002-언마운트-명령어)
  - [5.3 Software Interfaces (소프트웨어 인터페이스)](#53-software-interfaces-소프트웨어-인터페이스)
    - [EXT-SW-001: FUSE API](#ext-sw-001-fuse-api)
- [6. Other Requirements (기타 요구사항)](#6-other-requirements-기타-요구사항)
  - [6.1 Legal or Regulatory Requirements (법적 요구사항)](#61-legal-or-regulatory-requirements-법적-요구사항)
    - [OTH-LEG-001: 오픈소스 라이선스](#oth-leg-001-오픈소스-라이선스)
  - [6.2 Packaging, Installation, and Deployment (패키징 및 설치)](#62-packaging-installation-and-deployment-패키징-및-설치)
    - [OTH-INST-001: 빌드 시스템](#oth-inst-001-빌드-시스템)
  - [6.3 Data Requirements (데이터 요구사항)](#63-data-requirements-데이터-요구사항)
    - [OTH-DATA-001: 파일시스템 포맷](#oth-data-001-파일시스템-포맷)
    - [OTH-DATA-002: 메타데이터 구조](#oth-data-002-메타데이터-구조)
- [7. Verification and Validation (검증 및 확인)](#7-verification-and-validation-검증-및-확인)
  - [7.1 Requirements Verification Methods (요구사항 검증 방법)](#71-requirements-verification-methods-요구사항-검증-방법)
    - [검증 방법 분류](#검증-방법-분류)
    - [검증 매트릭스 (요약)](#검증-매트릭스-요약)
    - [VER-001: 크래시 정합성 검증 (핵심)](#ver-001-크래시-정합성-검증-핵심)
    - [VER-ORACLE-001: write/fsync 검증은 FUSE read 단독에 의존하지 않는다](#ver-oracle-001-writefsync-검증은-fuse-read-단독에-의존하지-않는다)
    - [VER-ORACLE-002: raw scan 기반 독립 oracle로 교차검증한다](#ver-oracle-002-raw-scan-기반-독립-oracle로-교차검증한다)
  - [7.2 Acceptance Criteria (인수 기준)](#72-acceptance-criteria-인수-기준)
    - [기능 요구사항 인수 기준](#기능-요구사항-인수-기준)
    - [최종 릴리즈 인수 기준](#최종-릴리즈-인수-기준)
- [8. Appendices (부록)](#8-appendices-부록)
  - [8.1 Glossary (용어집)](#81-glossary-용어집)
  - [8.2 Use Cases and Scenarios (사용 사례)](#82-use-cases-and-scenarios-사용-사례)
    - [UC-001: 파일 생성 및 쓰기 (정상 시나리오)](#uc-001-파일-생성-및-쓰기-정상-시나리오)
  - [8.3 Requirements Traceability Matrix (요구사항 추적 매트릭스)](#83-requirements-traceability-matrix-요구사항-추적-매트릭스)
  - [8.4 Supporting Diagrams (지원 다이어그램)](#84-supporting-diagrams-지원-다이어그램)
    - [Diagram 1: 시스템 아키텍처 (재게시)](#diagram-1-시스템-아키텍처-재게시)
  - [문서 개정 이력](#문서-개정-이력)

---

# 1. Introduction (개요)

## 1.1 Purpose (목적)

본 Software Requirements Specification (SRS)는 **FUSE(Filesystem in Userspace) 기반 UFFS(Ultra-low-cost Flash File System) 파일시스템**의 요구사항을 명확하고 검증 가능한 형태로 정의합니다.

**정의하는 제품:**
- **제품명**: FUSE 기반 UFFS 파일시스템
- **버전**: 2.0 (Crash Consistency 추가)
- **릴리즈**: 첫 번째 안정 버전


## 1.2 Scope (범위)

### 제품 개요

본 시스템은 **NAND 플래시 메모리를 위한 경량 파일시스템**을 FUSE 인터페이스를 통해 사용자 공간(Userspace)에서 구현합니다. 특히 **크래시 정합성(Crash Consistency)**을 핵심 기능으로 하여, 전원 차단이나 시스템 크래시 상황에서도 파일시스템의 무결성을 보장합니다.

**주요 특징:**
- **POSIX 호환**: 표준 파일시스템 API 제공
- **크래시 정합성**: 전원 차단 시에도 데이터 일관성 보장 (durability는 명시한 범위 내에서만 보장)

### 시스템이 하는 것 (In Scope)

1. **파일 연산**: 생성, 읽기, 쓰기, 삭제, 동기화(fsync)
2. **디렉토리 연산**: 생성,읽기,삭제, 동기화(fsync)
3. **Crash Consistency** : 명시한 범위 내에서 파일 데이터, 메타데이터, 네임스페이스에 대한 crash consistency 보장


### 시스템이 하지 않는 것 (Out of Scope)

1. **플래시 관리**: 배드 블록 관리, 웨어 레벨링
2. **크래시 복구**: 마운트 시 자동 검증 및 복구
3. **오류 처리**: ECC 기반 비트 오류 자동 수정
4. 비기능적 요구사항 정의 및 테스트 : 이 프로젝트에서 성능,이식성과 같은 비기능적 요구사항의 정의와 테스트는 제외한다.



### 비즈니스 목표

- NAND Flash 환경에 적합한 UFFS 파일시스템의 파일 입출력에 대한 테스트 환경 제공
- 깃허브 리포지터리에 교육용 레퍼런스 구현 제공


## 1.3 References (참조 문서)

### 프로젝트 내부 문서 
1. **설계 명세서(Design Specification)**: `/docs/design-spec.md`
2. **테스트 계획서(Test Plan)**: `/docs/test-plan.md`
3. **테스트 결과서** : `/docs/test-result.md`
4. **프로젝트 README**: `/README.md`

### 참조 구현
1. **FUSE Documentation** - libfuse 3.x API Reference
   URL: https://github.com/libfuse/libfuse
2. **UFFS core Library** - https://github.com/rickyzheng/uffs

## 1.5 Definitions, Acronyms, and Abbreviations (용어 정의 및 약어)

### 용어 정의

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

### 약어

| 약어 | 전체 명칭 |
|-----|----------|
| **API** | Application Programming Interface |
| **SRS** | Software Requirements Specification |
| **I/O** | Input/Output |

# 2. Overall Description (전체 설명)

## 2.1 System or Product Perspective (시스템 관점)

> **IEEE 29148 가이드**: 시스템의 컨텍스트와 다른 시스템과의 관계를 설명합니다.

### 시스템 아키텍처

본 시스템은 리눅스 커널의 FUSE 인터페이스를 통해 사용자 공간에서 동작하는 파일시스템입니다.

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
          ↓ Block I/O (read/write/erase)
┌─────────────────────────────────┐
│   NAND Flash Device             │  ← 저장 장치
│   (Physical Storage)            │
└─────────────────────────────────┘
```

### 시스템 경계

**내부 컴포넌트:**
1. **FUSE 어댑터**: VFS와의 인터페이스 제공
2. **파일 관리자**: 파일 생성, 읽기, 쓰기, 삭제
3. **메타데이터 관리자**: 파일 속성 및 시간 정보

**외부 인터페이스:**
- **상위**: Linux VFS (FUSE 프로토콜)
- **하위**: NAND Flash Device (블록 I/O)
- **사용자**: POSIX 시스템 콜

### 기존 시스템과의 관계

- **POSIX 파일시스템**: 표준 호환 인터페이스 제공 (부분 호환)
- **기존 UFFS (uffs-reference)**: 핵심 개념 참조, 구현은 독자적
- **리눅스 VFS**: FUSE를 통해 통합
- **ext4, F2FS 등**: 대안 파일시스템 (본 시스템은 임베디드 특화)

## 2.2 System or Product Functions (주요 기능)

> **IEEE 29148 가이드**: 시스템의 주요 기능을 고수준에서 요약합니다. 상세 내용은 섹션 3에서 다룹니다.

본 시스템은 다음 주요 기능을 제공합니다:

### 1. 파일 시스템 기본 연산
- 파일 생성, 읽기, 쓰기
- 메타데이터 조회

### 2. 크래시 정합성 (Crash Consistency) ⭐
- 전원 차단 시 데이터 일관성 보장
- fsync 시점까지의 데이터 보장


## 2.3 User Characteristics (사용자 특성)

> **IEEE 29148 가이드**: 예상 사용자의 특성을 기술합니다.

### 주요 사용자 유형

| 사용자 유형 | 기술 수준 | 사용 목적 | 예상 비율 |
|-----------|---------|----------|----------|
| **임베디드 시스템 개발자** | 고급 (리눅스 시스템 프로그래밍 경험) | 제품 개발에 UFFS 통합 | 40% |
| **파일시스템 연구자** | 전문가 (파일시스템 내부 구조 이해) | 연구 및 교육 목적 | 30% |
| **학생/교육자** | 중급~고급 | 학습 및 실습 | 20% |
| **시스템 관리자** | 중급 (리눅스 시스템 관리 경험) | 시스템 운영 및 유지보수 | 10% |

### 사용자 요구사항
- **임베디드 개발자**: 신뢰성, 낮은 메모리 사용, 명확한 API
- **연구자**: 상세한 문서, 확장 가능한 아키텍처
- **학생**: 이해하기 쉬운 코드, 교육용 예제
- **관리자**: 간편한 설치, 명확한 에러 메시지

## 2.4 Constraints, Assumptions, and Dependencies (제약사항, 가정, 의존성)

> **IEEE 29148 가이드**: 설계 및 구현에 영향을 미치는 제약사항을 명시합니다.

### 제약사항 (Constraints)

**기술적 제약:**
1. **FUSE 제약**: 커널 내 파일시스템 대비 성능 오버헤드 존재 (약 10-30%)
2. **메모리 제약**: 임베디드 시스템을 위해 메모리 사용 최소화 필요
3. **POSIX 부분 호환**: 하드 링크, 확장 속성 등 일부 기능 미지원

**법적 제약:**
5. **오픈소스 라이선스**: GPL/MIT/BSD 등 오픈소스 라이선스 준수 필요

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

## 2.5 Operational Environment (운영 환경)

> **IEEE 29148 가이드**: 시스템이 동작할 환경을 명시합니다.

### 하드웨어 환경

**호스트 컴퓨터 환경**
- **CPU**: Macbook M4 Pro
- **RAM**: 48GB
- **저장 장치**: SSD 512GB

**게스트 컴퓨터 환경**
- **CPU**: ARM64(aarch64)
- **RAM**: 4GB
- **저장 장치**: 12GB
<!-- - **플래시 규격**: 페이지 크기 2KB~4KB, 블록 크기 128KB~256KB -->

### 소프트웨어 환경

**운영 체제:**
- **배포판**: Ubuntu 20.04

---

# 3. Functional Requirements (기능 요구사항)

**요구사항 작성 형식:**
각 요구사항은 다음 정보를 포함합니다:
- **ID**: 고유 식별자
- **Description**: 요구사항 설명
- **Priority**: Critical / High / Medium / Low
- **Verification**: 검증 방법 (섹션 7과 연계)

## 3.1 File Operations (파일 연산)

### FR-FILE-001 파일 열기
**Description**: 시스템은 파일을 열 수 있어야한다.

**Priority**: Critical

### FR-FILE-002: 파일 생성
**Description**: 시스템은 사용자가 지정한 경로에 새로운 빈 파일을 생성할 수 있어야 한다.

**Priority**: Critical

**Verification**: 단위 테스트, 통합 테스트 (섹션 7.1 참조)

---

### FR-FILE-003: 파일 읽기
**Description**: 시스템은 파일의 지정된 오프셋부터 지정된 크기만큼 데이터를 읽을 수 있어야 한다.

**Priority**: High

**Verification**: 단위 테스트, 성능 테스트

---

### FR-FILE-004: 파일 쓰기
**Description**: 시스템은 파일의 지정된 오프셋에 데이터를 쓸 수 있어야 한다.

**Priority**: Critical

**Verification**: 단위 테스트, 통합 테스트

---

### FR-FILE-005: 파일 동기화 (fsync)

**Description**: 시스템은 fsync(fd) 성공 시점 기준으로 해당 파일 객체 (file object) 의 데이터 및 파일 메타데이터 내구성을 보장해야한다.

**Requirements**
- FR-FILE-005-1 파일 데이터 내구성: fsync(fd)가 0을 반환하면, 해당 호출 이전에 같은 파일디스크립터에 대해 완료된 write, pwrite로 인해 변경된 파일데이터 내용은, 이후 비정상 종료 및 재마운트 후에도 동일하게 관측되어야한다.
- FR-FILE-005-2 (파일 사이즈 내구성): fsync(fd) 가 0을 반환하면, 해당 호출 이전에 완료된 쓰기로 인해 변경된 파일 크기는 이후 비정상 종료 및 재마운트 후에도 동일하게 관측되어야한다.
- FR-FILE-005-3 내구성 보장의 경계: fsync의 내구성 보장은 파일 객체 자체에 한정되며, 파일의 이름/경로 관측가능성은 FR-DIR-001에 의해 정의된다.

**Priority**: Critical

### FR-FILE-006: 파일 이름 변경 (rename)

**Description**: 시스템은 지정된 파일의 이름 변경을 지원 해야한다.
**Priority**: Critical

### FR-FILE-007: 파일 삭제

**Description**: 시스템은 지정된 파일을 삭제할 수 있다.

**Priority**: Critical


### FR-FILE-008: 파일, 폴더 동시성 지원

**Description**: 시스템은 모든 파일 및 디렉토리 연산에 대해 전역 뮤텍스 락(Global Mutex Lock)을 사용하여 멀티스레드 환경에서의 동시 접근 안전성을 보장해야 한다. 상세 동시성 요구사항은 FR-CONC-001에서 정의한다.

**Priority**: Critical

## 3.2 Directory Operations (파일 연산)

### FR-DIR-001: 디렉토리 동기화 (fsync)
**Description**: 시스템은 fsync(dirfd) 성공 시점 기준으로 해당 디렉토리의 이름 매핑 변경 내구성을 보장해야한다.

**Requirements**
- FR-DIR-001-1 이름 매핑 변화 내구성: fsync(dirfd) 가 0을 반환하면, 해당 호출 이전에 완료된 디렉토리 엔트리 변경은 이후 비정상 종료 및 재마운트 후에도 동일하게 관측되어야 한다.
- FR-DIR-001-2 fsync 없는 범위에 대한 비보장: FR-DIR-001이 충족되지 않은 경우 (fsync(dirfd) 성공이 없었던 경우, 비정상 종료 및 재마운트 후에 디렉토리 엔트리 변경 결과가 유지되는지 여부는 보장하지 않는다.

**Notes**
UFFS 파일시스템은 디렉토리 구조가 스캔 기반 메타레코드로 재구성되므로 디렉토리 엔트리 블록으로 판단하지 않고 재마운트 후 관측되는 name과 object 관계의 동일성으로 요구사항을 판단한다.

**Priority**: Critical

**Verification**: Crash consistency 테스트


### FR-DIR-002 디렉토리 읽기
**Priority**: High
**Description**: 시스템은 지정된 디렉토리 내 파일 및 하위 디렉토리의 이름 목록을 반환해야 한다.

### FR-DIR-003 디렉토리 생성

**Description**: 시스템은 지정된 경로에 빈 디렉토리를 생성해야한다
**Priority**: High

### FR-DIR-004 디렉토리 삭제

**Description**: 시스템은 지정된 이름의 디렉토리를 삭제할 수 있다.
**Priority**: Medium

### FR-META-001 메타데이터 조회

**Description**: 시스템은 파일, 폴더의 메타데이터를 조회할 수 있다. (권한, 링크, 파일의 길이)
**Priority**: High

## 3.3 Concurrency Requirements (동시성 요구사항)

**Scope Note**:
본 섹션은 "동시성 성능 최적화"가 아니라, **동시 접근 상황에서의 무결성(Integrity), 원자성(Atomicity), 데드락 프리(Deadlock-free)**를 신뢰성 범위에 포함하기 위한 요구사항을 정의한다.

### FR-CONC-001: 동시 접근 안전성

**Description**: 시스템은 전역 뮤텍스 락(Global Mutex Lock)을 사용하여 모든 FUSE API 콜백 함수에 대해 스레드 간 동시 접근을 직렬화(serialize)하고, 이를 통해 파일시스템 내부 상태의 무결성을 보장해야 한다.

**Rationale**: FUSE는 멀티스레드 모드에서 동작할 수 있으며, 커널로부터 여러 요청이 동시에 전달될 수 있다. 전역 뮤텍스 락을 사용하면 구현 복잡도를 낮추면서도 동시 접근으로 인한 데이터 레이스(data race)와 파일시스템 상태 손상을 방지할 수 있다.

**Requirements**:

- **FR-CONC-001-1 (전역 뮤텍스 락 적용 범위)**: 시스템은 다음 FUSE API 콜백 함수 진입 시 전역 뮤텍스를 획득(lock)하고, 함수 완료 시 해제(unlock)해야 한다:
  - `getattr` (FR-META-001)
  - `open` (FR-FILE-001)
  - `create` (FR-FILE-002)
  - `read` (FR-FILE-003)
  - `write` (FR-FILE-004)
  - `fsync` (FR-FILE-005)
  - `rename` (FR-FILE-006)
  - `unlink` (FR-FILE-007)
  - `readdir` (FR-DIR-002)
  - `mkdir` (FR-DIR-003)
  - `rmdir` (FR-DIR-004)
  - `fsyncdir` (FR-DIR-001)

- **FR-CONC-001-2 (상호 배제 보장)**: 전역 뮤텍스 락에 의해 동일 시점에 최대 하나의 FUSE API 콜백만 실행되어야 하며, 이를 통해 파일시스템 내부 자료구조(메타데이터, 블록 할당 테이블 등)에 대한 동시 접근이 발생하지 않아야 한다.

- **FR-CONC-001-3 (데드락 프리)**: 전역 뮤텍스 락은 단일 락이므로 데드락이 발생하지 않아야 한다. 락 획득 순서 문제(lock ordering)가 존재하지 않음을 보장한다.

- **FR-CONC-001-4 (스레드 안전성)**: 전역 뮤텍스 락을 통해 보호되는 모든 FUSE API 콜백 함수는 멀티스레드 환경에서 호출되더라도 파일시스템 상태의 일관성을 유지해야 한다.

**Priority**: Medium

**Verification**: 동시성 테스트 — 복수 스레드에서 파일/디렉토리 연산을 동시 수행하여 데이터 레이스, 데드락, 메타데이터 불일치가 발생하지 않음을 검증한다.


# 4. Non-Functional Requirements (비기능 요구사항)


## 4.1 Reliability and Availability (신뢰성 및 가용성)


### NFR-REL-001: 데이터 손실 방지
**Description**: 시스템은 fsync 호출 후 전원 차단 시 데이터 손실이 발생하지 않아야 한다.

**Measurement**:
- 테스트: fsync 후 즉시 power-cut
- 측정 항목: 데이터 손실 횟수
- 목표: 0건 (100% 보장)

**Priority**: Critical

**Verification**: fsync durability 테스트
<!-- 
## 4.2 Non-Functional Requirements – Concurrency Reliability
NFR-CONC-001: 동시성 스트레스 안정성

Description
시스템은 동시성 스트레스 환경에서 크래시, 데드락, 메타데이터 손상 없이 동작해야 한다.

Measurement

테스트: N개 프로세스(예: 8~32)가 파일 생성/쓰기/rename/unlink/readdir를 랜덤 수행

측정 항목:

데드락/행(hang) 발생 횟수

비정상 종료 횟수

oracle 기준 메타데이터 불일치 횟수

목표: 0건

Priority: High
Verification: Stress Test + Oracle 검증 -->

---

# 5. External Interface Requirements (외부 인터페이스 요구사항)

> **IEEE 29148 가이드**: 시스템의 모든 외부 인터페이스를 명확히 정의합니다.

## 5.1 User Interfaces (사용자 인터페이스)

### EXT-UI-001: 마운트 명령어
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

### EXT-UI-002: 언마운트 명령어
**Description**: 시스템은 표준 umount 명령어를 통해 언마운트할 수 있어야 한다.

**Interface**:
```bash
umount <mount_point>
```

**Priority**: Critical

---

## 5.3 Software Interfaces (소프트웨어 인터페이스)

### EXT-SW-001: FUSE API
**Description**: 시스템은 libfuse 3.x API를 사용하여 VFS와 통신해야 한다.

**Library**: libfuse 3.2.0 이상

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
**Priority**: Critical

---

# 6. Other Requirements (기타 요구사항)

## 6.1 Legal or Regulatory Requirements (법적 요구사항)

### OTH-LEG-001: 오픈소스 라이선스
**Description**: 시스템은 오픈소스 라이선스를 준수해야 한다.

**License Options**:
- GPL v2 (libfuse 호환)
- MIT License
- BSD License

**Requirements**:
- 소스 코드 공개
- 라이선스 고지 포함
- 저작권 표시

**Priority**: Critical

---


## 6.2 Packaging, Installation, and Deployment (패키징 및 설치)

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

## 6.3 Data Requirements (데이터 요구사항)

### OTH-DATA-001: 파일시스템 포맷
**Description**: 시스템은 UFFS 전용 온디스크(on-disk) 포맷을 정의해야 한다.

**Format Structure**:
```
+-----------------+
| Superblock      | (파일시스템 메타정보)
+-----------------+
| Inode Table     | (파일/디렉토리 메타데이터)
+-----------------+
| Data Blocks     | (실제 파일 데이터)
+-----------------+
```

**Superblock Fields**:
- Magic number (파일시스템 식별)
- Version
- Block size
- Total blocks
- Free blocks
- Inode count
- Mount count
- Last mount time

**Priority**: Critical

---

### OTH-DATA-002: 메타데이터 구조
**Description**: 시스템은 파일 메타데이터를 정의된 구조로 저장해야 한다.

**Inode Structure**:
```c
struct uffs_inode {
    uint32_t inode_number;
    uint16_t mode;           // 파일 타입 및 권한
    uint16_t uid;            // 소유자 UID
    uint16_t gid;            // 그룹 GID
    uint64_t size;           // 파일 크기
    uint64_t atime;          // 접근 시간
    uint64_t mtime;          // 수정 시간
    uint64_t ctime;          // 변경 시간
    uint32_t block_pointers[N]; // 데이터 블록 포인터
    uint32_t checksum;       // 메타데이터 체크섬
};
```

**Priority**: Critical

---

# 7. Verification and Validation (검증 및 확인)

## 7.1 Requirements Verification Methods (요구사항 검증 방법)

### 검증 방법 분류

| 검증 방법 | 설명 | 적용 요구사항 예시 |
|----------|------|-------------------|
| **Unit Test** | 개별 모듈의 기능 검증 | FR-FILE-001~008, FR-DIR-001~004, FR-META-001 |
| **Concurrency Test** | 멀티스레드 환경에서의 동시 접근 안전성 검증 | FR-CONC-001, FR-FILE-008 |

### 검증 매트릭스 (요약)

| 요구사항 ID | 검증 방법 | 도구/기법 |
|-----------|----------|----------|
| FR-FILE-001~008| Unit Test | Google Test |
| FR-DIR-001~004 | Unit Test | Google Test |
| FR-META-001 | Unit Test | Google Test |
| FR-CONC-001 | Concurrency Test | 멀티스레드 동시 연산 테스트 |
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

### VER-ORACLE-001: write/fsync 검증은 FUSE read 단독에 의존하지 않는다

**Target Requirements**: FR-FILE-004, 005



### VER-ORACLE-002: raw scan 기반 독립 oracle로 교차검증한다


**Target Requirements**: FR-FILE-004, 005

---

## 7.2 Acceptance Criteria (인수 기준)

### 기능 요구사항 인수 기준

**AC-001: 파일 연산 인수 기준**
- [ ] 모든 파일 연산 단위 테스트 통과 (FR-FILE-001~007)

**AC-002: 크래시 정합성 인수 기준 (핵심)**
- [ ] fsync 이후 데이터 손실 0건
- [ ] 메타데이터 일관성 100%

### 최종 릴리즈 인수 기준

**AC-004: 릴리즈 준비 완료**
- [ ] 모든 기능 요구사항 인수 기준 통과 (AC-001~002)
- [ ] 문서화 완료 (SRS, 설계서, 사용자 매뉴얼, API 문서)
- [ ] 라이선스 고지 및 저작권 표시 완료

---

# 8. Appendices (부록)

## 8.1 Glossary (용어집)

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

## 8.2 Use Cases and Scenarios (사용 사례)

### UC-001: 파일 생성 및 쓰기 (정상 시나리오)

**Actor**: 사용자 애플리케이션

**Preconditions**:
- 파일시스템이 마운트됨
- 충분한 여유 공간 존재

**Main Flow**:
1. 애플리케이션이 `open("/mnt/uffs/test.txt", O_CREAT|O_WRONLY)` 호출
2. 시스템이 새 파일 생성 (FR-FILE-002)
3. 애플리케이션이 `write(fd, data, size)` 호출
4. 시스템이 데이터를 메모리 버퍼에 기록 (FR-FILE-004)
5. 애플리케이션이 `fsync(fd)` 호출
6. 시스템이 데이터를 플래시에 동기화 (FR-FILE-005)
7. 애플리케이션이 `rename("/mnt/uffs/test.txt", "/mnt/uffs/final.txt")` 호출
8. 파일 이름 성공적으로 변경 (FR-FILE-006)
9. 애플리케이션이 `fsync(dirfd)` 호출
10. 부모-자식 매핑 구조 성공적으로 동기화 (FR-DIR-001)

**Postconditions**:
- 파일이 생성되고 데이터가 안전하게 저장됨
- 전원 차단 시에도 fsync 시점까지의 데이터는 보장됨

**Related Requirements**: FR-FILE-002, FR-FILE-004, FR-FILE-005, FR-FILE-006, FR-DIR-001

---

## 8.3 Requirements Traceability Matrix (요구사항 추적 매트릭스)

| 요구사항 ID | 요구사항 이름 | 우선순위 | 검증 방법 | 테스트 케이스 ID | 상태 |
|-----------|-------------|---------|----------|-----------------|------|
| FR-FILE-001 | 파일 열기 | Critical | Unit Test | TC-FILE-001 | TBD |
| FR-FILE-002 | 파일 생성 | Critical | Unit Test | TC-FILE-002 | TBD |
| FR-FILE-003 | 파일 읽기 | High | Unit Test | TC-FILE-003 | TBD |
| FR-FILE-004 | 파일 쓰기 | Critical | Unit Test | TC-FILE-004 | TBD |
| FR-FILE-005 | 파일 동기화 (fsync) | Critical | Crash Test | TC-CRASH-005 | TBD |
| FR-FILE-006 | 파일 이름 변경 (rename) | Critical | Unit Test | TC-FILE-006 | TBD |
| FR-FILE-007 | 파일 삭제 | Critical | Unit Test | TC-FILE-007 | TBD |
| FR-FILE-008 | 파일, 폴더 동시성 지원 | Critical | Concurrency Test | TC-CONC-008 | TBD |
| FR-DIR-001 | 디렉토리 동기화 (fsync) | Critical | Crash Test | TC-CRASH-DIR-001 | TBD |
| FR-DIR-002 | 디렉토리 읽기 | High | Unit Test | TC-DIR-002 | TBD |
| FR-DIR-003 | 디렉토리 생성 | High | Unit Test | TC-DIR-003 | TBD |
| FR-DIR-004 | 디렉토리 삭제 | Medium | Unit Test | TC-DIR-004 | TBD |
| FR-META-001 | 메타데이터 조회 | High | Unit Test | TC-META-001 | TBD |
| FR-CONC-001 | 동시 접근 안전성 | Medium | Concurrency Test | TC-CONC-001 | TBD |
| **NFR-REL-001** | **데이터 손실 방지** | **Critical** | **Durability Test** | **TC-REL-001** | **TBD** |

**Note**: TBD = To Be Determined (구현 단계에서 결정)

**상태 값**:
- TBD: 구현 전
- In Progress: 구현 중
- Implemented: 구현 완료
- Verified: 검증 완료

## 8.4 Supporting Diagrams (지원 다이어그램)

### Diagram 1: 시스템 아키텍처 (재게시)

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
│  ┌──────────────┬──────────────┬──────────────┐        │
│  │Crash Recovery│ ECC Module   │Flash Manager │        │
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
| 1.0 | 2026.02.14 | 임재형 | 초안 작성 |

---

**문서 끝 (End of SRS)**


