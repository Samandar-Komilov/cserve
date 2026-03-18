---
phase: 01-security-and-safety
plan: 02
subsystem: http
tags: [memory-safety, strndup, use-after-free, http-parser, c]

# Dependency graph
requires: []
provides:
  - "Heap-owned parsed HTTP request fields via strndup()"
  - "Complete free_http_request() covering all owned strings"
affects: [01-security-and-safety, 02-core-http-engine]

# Tech tracking
tech-stack:
  added: []
  patterns: ["strndup for all buffer-extracted strings", "NULL check after every strndup", "free loop for dynamic header arrays"]

key-files:
  created: []
  modified:
    - src/http/parsers.c
    - src/http/request.c

key-decisions:
  - "Used strndup over manual malloc+memcpy for clarity and fewer error paths"
  - "Added NULL guard to free_http_request entry for safety on partially-parsed requests"

patterns-established:
  - "Owned-copy pattern: all parsed string fields are heap copies, never raw buffer pointers"
  - "Symmetric alloc/free: every strndup in parsers.c has a corresponding free in request.c"

requirements-completed: [SEC-03]

# Metrics
duration: 2min
completed: 2026-03-18
---

# Phase 1 Plan 2: Fix use-after-free in HTTP Parser Summary

**Replaced all raw buffer pointers with strndup() heap copies in HTTP parser, eliminating use-after-free on buffer reallocation**

## Performance

- **Duration:** 2 min
- **Started:** 2026-03-18T11:07:41Z
- **Completed:** 2026-03-18T11:09:26Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments
- All 5 parsed field types (method, uri, protocol, header name, header value) now use strndup() heap copies
- free_http_request() correctly frees all owned strings including per-header loop
- Buffer reallocation after parsing can no longer create dangling pointers
- Existing test suite passes unchanged

## Task Commits

Each task was committed atomically:

1. **Task 1: Replace raw pointer storage with strndup() in parsers.c** - `67a1ab2` (fix)
2. **Task 2: Update free_http_request() to free all strndup'd fields** - `380cd8f` (fix)

## Files Created/Modified
- `src/http/parsers.c` - Replaced 5 raw pointer assignments with strndup() + NULL checks
- `src/http/request.c` - Updated free_http_request() to free method, uri, protocol, header names/values, body; added NULL guard

## Decisions Made
- Used strndup over manual malloc+memcpy for clarity and fewer error paths
- Added NULL guard to free_http_request entry for safety on partially-parsed requests
- Kept _len fields alongside strndup'd strings (strndup null-terminates, but _len preserves O(1) length access)

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- HTTP parser fields are now memory-safe owned copies
- Ready for further security fixes (01-03) or core HTTP engine work
- Valgrind verification possible once server binary runs end-to-end

---
*Phase: 01-security-and-safety*
*Completed: 2026-03-18*
