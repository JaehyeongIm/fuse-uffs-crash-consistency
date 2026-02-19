# ADR-001: 요구사항 명세 기준으로 IEEE 29148 채택

Status: Accepted  
Date: 2026-02-19

## Context

본 프로젝트는 crash consistency를 포함한 파일시스템 신뢰성을 요구사항 수준에서 명확하게 정의하고,
요구사항–설계–테스트를 산출물로 추적 가능하게 남기는 것을 목표로 한다.

요구사항 문서의 기준 표준을 선택해야 하며, 다음 후보를 검토하였다:

- IEEE 29148
- IEEE 830
- IEEE 12207
- IEEE 15288

IEEE 830은 SRS 템플릿 중심이며 요구사항 품질 특성 정의가 제한적이다.  
IEEE 12207은 소프트웨어 생명주기 프로세스에 초점이 있으며 요구사항 자체보다는 프로세스에 집중한다.  
IEEE 15288은 시스템 생명주기 표준으로 시스템 엔지니어링 관점에 집중되어 있다.

## Decision

요구사항 명세 기준으로 IEEE 29148을 채택한다.

## Consequences

- 요구사항의 명확성, 검증가능성, 추적성이 향상된다.
- 요구사항 문서 구조가 체계화된다.
- 문서 작성 비용이 증가할 수 있다.
- 오버엔지니어링 위험이 존재한다.


# ADR-002: 요구사항 추적 도구로 Github issues 채택

Status: Accepted  
Date: 2026-02-19

## Context

요구사항–설계–테스트 간의 추적성을 유지하기 위한 도구 선택이 필요하다.
본 프로젝트는 단일 개발자 환경에서 Git 기반으로 진행된다.

검토한 후보:

- GitHub Issues
- Jira

Jira는 강력한 워크플로우와 추적 기능을 제공하지만,
프로젝트 규모 대비 오버헤드가 크고 관리 비용이 높다.

## Decision

요구사항 추적 도구로 GitHub Issues를 사용한다.

## Consequences

- 코드(Commit/PR)와 요구사항의 직접 연결이 가능하다.
- 운영 오버헤드가 낮다.
- 고급 추적 기능은 제한적이다.
- 추적 체계는 프로젝트 내 규칙 설계에 의존한다.


# ADR-003: 파일시스템 구현 방식으로 FUSE 채택

Status: Accepted  
Date: 2026-02-19

## Context

파일시스템 구현 환경을 결정해야 한다.
본 프로젝트의 1차 목표는 성능 최적화가 아니라 crash consistency 검증 프레임워크 구축이다.

검토한 후보:

- FUSE 기반 사용자 공간 구현
- 커널 모듈 기반 구현

커널 모듈 방식은 panic 발생 시 시스템 전체에 영향을 주며,
반복적인 crash 테스트 및 디버깅 비용이 높다.

## Decision

FUSE 기반 사용자 공간 구현 방식을 채택한다.

## Consequences

- 반복적인 crash 테스트와 로그 수집이 용이하다.
- 개발 및 디버깅 생산성이 높다.
- 커널 대비 context switch 오버헤드가 발생한다.
- 성능 평가는 참고 수준으로 제한된다.


# ADR-004: 전역  락을 사용하여 동시성 지원

Status: Accepted  
Date: 2026-02-19

## Context

파일시스템의 동시성 관리 전략을 결정해야 한다.
프로젝트 기간은 제한적이며, crash consistency 검증이 1차 목표이다.

검토한 후보:

- 전역 락
- 전체 fine-grained locking
- 전역 락 + 부분적 fine-grained locking

Fine-grained locking은 deadlock-free 설계와 검증에 높은 복잡도와 리스크가 존재한다.

## Decision

전역 락 전략을 채택한다.

## Consequences

- 설계 복잡도가 감소한다.
- 테스트 구조가 단순화된다.
- 병렬 처리 성능이 제한된다.
- 향후 확장 시 락 구조 재설계가 필요할 수 있다.
