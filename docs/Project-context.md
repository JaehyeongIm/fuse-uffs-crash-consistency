# PROJECT CONTEXT

## 1. Project Identity

- Project Name: UFFS 파일시스템 crash consistency 보장
- Domain: (e.g., User-space file system over FUSE)
- Target Industry: (Manufacturing / Defense / Finance-grade reliability)
- Primary Goal: Reliability over performance

---

## 2. Non-Negotiable Objectives (절대 목표)

1. Crash Consistency must be provable after fsync boundary.
2. Metadata integrity must never be violated.
3. Deterministic behavior under concurrent access.
4. All critical behaviors must be testable and traceable.

Anything violating above is unacceptable.

---

## 3. Explicit Non-Goals (하지 않는 것)

- No kernel-level optimization.
- No aggressive fine-grained locking unless formally justified.
- No performance-first trade-offs.
- No undefined behavior tolerance.

---

## 4. Crash Model Definition

Crash may occur at:
- Any instruction boundary
- During write
- During rename
- After fsync(file)
- After fsync(dir)

Persistence guarantee boundary:
- Only guaranteed after fsync returns successfully.

---

## 5. Concurrency Model

Current Strategy:
- Global lock
- Partial lock for rename (if applicable)

Priority:
Safety > Determinism > Performance

---

## 6. Current Project Level

Level:
(e.g., Level 1 - Single-threaded crash consistency validation)

Scope included:
- create
- write
- fsync
- rename
- unlink

Out of scope:
- Journaling
- Copy-on-write
- Fine-grained lock redesign

---

## 7. Verification Strategy

Verification Philosophy:
- Observational semantics + internal invariants
- Fault injection preferred
- Traceability from SRS → Test → Result mandatory

Oracle Types:
- System-call level oracle
- Metadata hash verification
- (Optional) Block-level bypass oracle

---

## 8. Risk Posture

Risk tolerance: Low

Prefer:
- Simpler design
- Easier to verify
- Lower change-cost

Avoid:
- Architecturally disruptive changes
- Design without testability

---

## 9. Decision History Snapshot

- D-001: FUSE over kernel module (for testability)
- D-002: Global lock for schedule feasibility
- D-003: fsync boundary defines durability guarantee

(Full ADR available separately)

---

## 10. What I Want From AI

When answering:
- Respect Non-Negotiable Objectives
- Do not suggest scope expansion unless explicitly asked
- Identify risks explicitly
- Prefer testable and deterministic designs
- Highlight traceability impact if design changes