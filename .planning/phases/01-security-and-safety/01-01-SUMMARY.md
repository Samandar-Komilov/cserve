---
phase: 01-security-and-safety
plan: 01
subsystem: server
tags: [c, security, fd-lifecycle, integer-overflow, buffer-limits]

# Dependency graph
requires: []
provides:
  - "MAX_HEADER_SIZE and MAX_BODY_SIZE constants in common.h"
  - "size_t body_length/content_length in HTTPResponse"
  - "fd sentinel -1 connection lifecycle (no double-close)"
  - "Buffer size enforcement in recv loop"
affects: [01-security-and-safety, 03-http-protocol]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "calloc instead of malloc+memset for NULL-safe allocation"
    - "fd sentinel -1 for empty connection slots (not 0)"
    - "Caller owns fd lifetime; free_connection only cleans state"

key-files:
  created: []
  modified:
    - "src/common.h"
    - "src/http/response.h"
    - "src/http/response.c"
    - "src/http/server.c"

key-decisions:
  - "SEC-06 strtoull deferred to Phase 3: no Content-Length atoi exists in server.c; body parsing is commented out in parsers.c"
  - "free_connection owns curr_request cleanup to prevent double-free"
  - "epoll_ctl DEL before free_connection, close(fd) after free_connection"

patterns-established:
  - "fd sentinel: connection slots use -1 as empty marker, not 0"
  - "Caller-owns-fd: free_connection cleans state, caller calls close()"
  - "Buffer limit enforcement: reject requests exceeding MAX_HEADER_SIZE"

requirements-completed: [SEC-05, SEC-06]

# Metrics
duration: 2min
completed: 2026-03-18
---

# Phase 1 Plan 1: Foundational Security Fixes Summary

**fd sentinel changed from 0 to -1, response integer types fixed to size_t, buffer size limits enforced at 8KB/1MB**

## Performance

- **Duration:** 2 min
- **Started:** 2026-03-18T11:07:46Z
- **Completed:** 2026-03-18T11:10:02Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Established correct fd sentinel (-1) for connection pool, eliminating double-close and stdin-confusion bugs
- Changed body_length/content_length from int to size_t, preventing integer overflow on large payloads
- Added MAX_HEADER_SIZE (8KB) and MAX_BODY_SIZE (1MB) constants for downstream use
- Added buffer size enforcement in recv loop to reject oversized requests
- Fixed NULL-dereference-before-check in response_builder (calloc replaces malloc+memset)

## Task Commits

Each task was committed atomically:

1. **Task 1: Add size limit constants and fix response type** - `ff9133f` (fix)
2. **Task 2: Fix fd sentinel, adjacent bugs, and Content-Length parsing** - `50012f5` (fix)

## Files Created/Modified
- `src/common.h` - Added MAX_HEADER_SIZE and MAX_BODY_SIZE defines
- `src/http/response.h` - Changed body_length and content_length to size_t
- `src/http/response.c` - Replaced malloc+memset with calloc in response_builder
- `src/http/server.c` - fd sentinel fix, connection lifecycle cleanup, buffer enforcement

## Decisions Made
- SEC-06 strtoull replacement deferred to Phase 3: no Content-Length atoi() call exists in server.c (body parsing is commented out in parsers.c). The size_t type change in Task 1 satisfies the integer overflow concern for now.
- free_connection now owns curr_request cleanup to centralize resource management and prevent double-free in the closing block.
- Connection close ordering: epoll_ctl DEL first, free_connection second, close(fd) last.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed buffer null-termination on negative recv**
- **Found during:** Task 2 (recv loop changes)
- **Issue:** `conn->buffer[conn->buffer_len + bytes_read] = '\0'` executed before checking if bytes_read < 0, writing to wrong memory location
- **Fix:** Moved null-termination into the success path (after buffer_len += bytes_read)
- **Files modified:** src/http/server.c
- **Verification:** Build succeeds, tests pass
- **Committed in:** 50012f5 (Task 2 commit)

**2. [Rule 1 - Bug] Fixed double-free of curr_request in closing block**
- **Found during:** Task 2 (adding curr_request cleanup to free_connection)
- **Issue:** Closing block freed curr_request manually, then called free_connection which now also frees it
- **Fix:** Removed manual free_http_request from closing block; free_connection handles it
- **Files modified:** src/http/server.c
- **Verification:** Build succeeds, no double-free path
- **Committed in:** 50012f5 (Task 2 commit)

---

**Total deviations:** 2 auto-fixed (2 bugs, Rule 1)
**Impact on plan:** Both auto-fixes necessary for correctness. No scope creep.

## Issues Encountered

- realpath() leak, tautology fix, shadow variable, and accept error handling were all verified already correct in the current codebase -- no changes needed for those items.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Size limit constants (MAX_HEADER_SIZE, MAX_BODY_SIZE) ready for SEC-01 and SEC-02 fixes in subsequent plans
- fd lifecycle is now correct for all connection management work
- size_t types in response ready for proper Content-Length serialization

---
*Phase: 01-security-and-safety*
*Completed: 2026-03-18*

## Self-Check: PASSED
