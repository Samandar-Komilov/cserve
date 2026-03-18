---
phase: 1
slug: security-and-safety
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-18
---

# Phase 1 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Shell scripts (curl + bash assertions) — no test framework until Phase 2 |
| **Config file** | none — smoke tests are standalone scripts |
| **Quick run command** | `bash tests/smoke/run_all.sh` |
| **Full suite command** | `bash tests/smoke/run_all.sh` |
| **Estimated runtime** | ~5 seconds |

---

## Sampling Rate

- **After every task commit:** Run `bash tests/smoke/run_all.sh`
- **After every plan wave:** Run `bash tests/smoke/run_all.sh`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 5 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 01-03-03 | 03 | 2 | SEC-01 | smoke | `bash tests/smoke/test_sec01_path_traversal.sh` | created by 01-03 Task 3 | pending |
| 01-03-03 | 03 | 2 | SEC-02 | smoke | `bash tests/smoke/test_sec02_buffer_overflow.sh` | created by 01-03 Task 3 | pending |
| 01-02-01 | 02 | 1 | SEC-03 | build | `cmake --build build && valgrind ./build/cserve --help` | yes | pending |
| 01-03-03 | 03 | 2 | SEC-04 | smoke | `bash tests/smoke/test_sec04_signal.sh` | created by 01-03 Task 3 | pending |
| 01-01-02 | 01 | 1 | SEC-05 | code review | `grep -rn 'socket = -1' src/` | yes | pending |
| 01-01-01 | 01 | 1 | SEC-06 | build | `cmake --build build` (strtoull + size_t compiles clean) | yes | pending |

*Status: pending / green / red / flaky*

---

## Wave 0 Requirements

- [ ] `tests/smoke/run_all.sh` — runner script that executes all smoke tests (created by Plan 01-03 Task 3)
- [ ] `tests/smoke/test_sec01_path_traversal.sh` — curl-based path traversal attack attempts (created by Plan 01-03 Task 3)
- [ ] `tests/smoke/test_sec02_buffer_overflow.sh` — oversized request/response tests (created by Plan 01-03 Task 3)
- [ ] `tests/smoke/test_sec04_signal.sh` — signal delivery and clean shutdown check (created by Plan 01-03 Task 3)

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Parser field ownership | SEC-03 | Requires valgrind to verify no use-after-free | Build with debug, run server, send requests, check valgrind output for invalid reads |
| Signal handler safety | SEC-04 | Requires sending signal during active request | Start server, send concurrent requests, kill -TERM, verify no crash/deadlock |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 5s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
