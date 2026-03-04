<!--
파일시스템 프로젝트 PR 템플릿
목적:
- 요구사항(SRS) → 설계(SDD) → 테스트(Test Plan) → 코드 → RTM 추적성 유지
- 1 PR = 1 기능 또는 1 Change Request
-->

# 1. 변경 요약 (Summary)

## 무엇을 변경했는가
-

## 왜 변경했는가
-

## 변경 범위
-

---

# 2. Traceability (추적성)

관련 ID를 작성하세요.

- Change Request: CR-
- Requirement: REQ-
- Design: SDD-
- Test Case: TC-
- Architecture Decision: ADR-

---

# 3. 문서 변경 여부

아래 문서 변경 여부를 체크하세요.

- [ ] SRS 수정
- [ ] SDD 수정
- [ ] Test Plan 수정
- [ ] RTM 수정
- [ ] CHANGELOG 수정
- [ ] ADR 추가 또는 수정
- [ ] 문서 변경 없음

문서 위치

docs/SRS.md  
docs/SDD.md  
docs/TEST_PLAN.md  
docs/RTM.md  
docs/CHANGELOG.md  
docs/ADR/

---

# 4. 설계 및 구현 설명

## 기능 동작 설명
-

## 에러 처리
-

## 주요 변경 코드 위치

src/

변경된 핵심 파일

- 
- 
- 

---

# 6. 테스트

## 테스트 방법

테스트 실행 방법을 작성하세요.

예

make test

또는

./run_tests.sh

---

## 테스트 결과

- [ ] unit test 통과
- [ ] integration test 통과
- [ ] regression test 통과

---

## RTM 매핑

| Requirement | Test Case |
|-------------|-----------|
| REQ-        | TC-       |

---

# 8. 리스크 분석

리스크 수준

- [ ] 낮음
- [ ] 중간
- [ ] 높음

---


# 11. Merge 전 체크리스트

- [ ] PR은 하나의 기능 변경만 포함
- [ ] Commit message에 REQ / TC / ADR 포함
- [ ] RTM 업데이트 완료
- [ ] CHANGELOG 업데이트 완료
- [ ] 테스트 통과
- [ ] 문서와 코드 일치