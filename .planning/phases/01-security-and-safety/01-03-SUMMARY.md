---
phase: 01-security-and-safety
plan: 03
subsystem: security
tags: [path-traversal, realpath, buffer-overflow, signal-safety, sigaction, smoke-tests]

requires:
  - phase: 01-security-and-safety/01
    provides: "fd sentinel fix, MAX_HEADER_SIZE/MAX_BODY_SIZE constants in common.h"
  - phase: 01-security-and-safety/02
    provides: "strndup-based parser, free_http_request cleanup"
provides:
  - "Path traversal prevention via percent-decode + realpath + prefix check"
  - "Heap-allocated proxy buffers with MAX_BODY_SIZE cap"
  - "Async-signal-safe shutdown via volatile sig_atomic_t flag"
  - "Smoke test suite for SEC-01, SEC-02, SEC-04"
affects: [07-proxy, 02-http-protocol]

tech-stack:
  added: []
  patterns: ["realpath-based path validation", "heap proxy buffers with size cap", "sig_atomic_t shutdown flag pattern"]

key-files:
  created:
    - tests/smoke/test_sec01_path_traversal.sh
    - tests/smoke/test_sec02_buffer_overflow.sh
    - tests/smoke/test_sec04_signal.sh
    - tests/smoke/run_all.sh
  modified:
    - src/http/server.c
    - src/http/server.h
    - src/main.c

key-decisions:
  - "percent_decode applied once before realpath -- double-encoded sequences become literal after single decode"
  - "shutdown_flag defined in main.c with extern in server.h for cross-TU sharing"
  - "SIGSEGV handler removed -- crash with core dump for debugging"

patterns-established:
  - "Path validation: always percent-decode URI then realpath() then prefix-check against resolved root"
  - "Heap buffers for untrusted input with MAX_BODY_SIZE cap and proper free on all exit paths"
  - "Signal handler only sets flag; cleanup in main loop"

requirements-completed: [SEC-01, SEC-02, SEC-04]

duration: 4min
completed: 2026-03-18
---

# Phase 1 Plan 3: Security Fixes Summary

**Path traversal blocked via percent-decode + realpath + prefix check; proxy buffers moved to heap with 1MB cap; signal handler rewritten to only set atomic flag**

## Performance

- **Duration:** 4 min
- **Started:** 2026-03-18T11:12:23Z
- **Completed:** 2026-03-18T11:16:51Z
- **Tasks:** 3
- **Files modified:** 7

## Accomplishments
- Path traversal prevented: URI percent-decoded, candidate path resolved with realpath(), verified to start with document root
- Proxy buffers moved from stack to heap with MAX_BODY_SIZE (1MB) cap on both request and response
- Signal handler rewritten: only sets volatile sig_atomic_t flag, event loop checks it, EINTR handled
- Smoke test suite created for all three security fixes

## Task Commits

Each task was committed atomically:

1. **Task 1: Fix path traversal and proxy buffer overflows** - `bdf44eb` (fix)
2. **Task 2: Fix signal handler safety** - `c70458e` (fix)
3. **Task 3: Write smoke tests** - `6e8e109` (test)

## Files Created/Modified
- `src/http/server.c` - percent_decode helper, realpath-based file serving, heap proxy buffers with dynamic read loop
- `src/http/server.h` - extern volatile sig_atomic_t shutdown_flag declaration
- `src/main.c` - shutdown_flag definition, sigaction registration, removed unsafe handler code
- `tests/smoke/test_sec01_path_traversal.sh` - Tests /../, %2e%2e, double-encoded traversal
- `tests/smoke/test_sec02_buffer_overflow.sh` - Tests oversized headers, server resilience
- `tests/smoke/test_sec04_signal.sh` - Tests SIGTERM clean shutdown
- `tests/smoke/run_all.sh` - Runner for all smoke test suites

## Decisions Made
- Applied single percent-decode pass before realpath -- double-encoded sequences (%252e) become literal %2e after decode, which realpath does not interpret as ".."
- Defined shutdown_flag in main.c (not static) with extern in server.h for cross-translation-unit access
- Removed SIGSEGV handler entirely -- segfaults should produce core dumps for debugging
- Proxy response uses dynamic read loop with doubling realloc, capped at MAX_BODY_SIZE

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed proxy response body length calculation**
- **Found during:** Task 1 (proxy buffer rewrite)
- **Issue:** Original code passed proxy_request_len as body length to response_builder (wrong variable)
- **Fix:** Calculate actual body_len from proxy_response pointer arithmetic
- **Files modified:** src/http/server.c
- **Verification:** Build succeeds, logic correct
- **Committed in:** bdf44eb (Task 1 commit)

**2. [Rule 1 - Bug] Fixed file buffer memory leak in static file serving**
- **Found during:** Task 1 (path traversal fix)
- **Issue:** Original code did not free the file content buffer after building response
- **Fix:** Added free(buffer) after response_builder call
- **Files modified:** src/http/server.c
- **Verification:** Build succeeds
- **Committed in:** bdf44eb (Task 1 commit)

---

**Total deviations:** 2 auto-fixed (2 bugs)
**Impact on plan:** Both fixes necessary for correctness. No scope creep.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- All Phase 1 security vulnerabilities (SEC-01 through SEC-06 where applicable) are now fixed
- Smoke test suite ready for regression testing
- Server builds cleanly and all unit tests pass
- Ready to proceed to Phase 2 (HTTP Protocol Compliance)

---
*Phase: 01-security-and-safety*
*Completed: 2026-03-18*
