# Phase 1: Security and Safety - Context

**Gathered:** 2026-03-18
**Status:** Ready for planning

<domain>
## Phase Boundary

Eliminate all known vulnerability classes in the existing codebase: path traversal, buffer overflows, use-after-free, signal handler safety, fd sentinel misuse, and integer overflow. Also fix all Critical and High severity bugs from CONCERNS.md that share code paths with SEC-* fixes. No architectural restructuring (Phase 2), no new features, no code quality cleanup.

</domain>

<decisions>
## Implementation Decisions

### Fix strategy
- Minimal in-place fixes — smallest possible changes to existing code, no function extraction or refactoring
- Phase 2 handles the real restructuring; Phase 1 just makes the code safe
- Include basic smoke tests (curl-based or small C programs) to verify each fix works, no test framework needed yet
- For SEC-03 (parser use-after-free): strndup() ALL parsed fields (method, URI, protocol, all header names/values), add corresponding free() calls

### Adjacent bug scope
- Fix all Critical and High severity bugs from CONCERNS.md, not just SEC-* requirements
- Additional fixes beyond SEC-01 through SEC-06:
  - Shadow variable in main.c (httpserver_ptr local shadows global)
  - Tautology in connection state check (OR → AND)
  - Double close of client fd (remove close from free_connection)
  - NULL check after dereference in response.c (check malloc before use)
  - realpath() memory leak in file serving (store, use, free)
  - Incomplete reset_connection() (initialize buffer_len and curr_request)
  - Failed accept() handling (don't close(-1))
- Code quality issues (debug printfs, unused macros, hardcoded path) left for Phase 2

### Signal handling
- Use volatile sig_atomic_t flag — handler sets flag, main loop checks after epoll_wait()
- Handle SIGTERM + SIGINT (both set shutdown flag); ignore SIGPIPE
- On shutdown: immediate close — break event loop, close all connections, free resources, exit
- No connection draining (graceful drain is v2 OPS-02)

### Size limits
- MAX_HEADER_SIZE = 8192 (8KB) — reject with 431 if exceeded
- MAX_BODY_SIZE = 1048576 (1MB) — reject with 413 if exceeded
- Content-Length max = same as body limit (1MB) — consistent single limit
- Proxy response buffer also capped at 1MB — backend exceeds → 502 Bad Gateway
- All limits as #define constants in common.h alongside existing MAX_CONNECTIONS
- Content-Length parsing: replace atoi() with strtoull(), body_length from int to size_t

### Claude's Discretion
- Exact order of fixing the 6 SEC-* vulns + adjacent bugs (dependency-aware sequencing)
- Smoke test implementation details (shell scripts vs C test programs)
- Whether to use calloc() vs malloc()+memset() for new allocations
- Temporary variable naming and code style within patches

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Security requirements
- `.planning/REQUIREMENTS.md` — SEC-01 through SEC-06 define the 6 vulnerability fixes with specific fix approaches

### Known bugs and fix approaches
- `.planning/codebase/CONCERNS.md` — Full inventory of ~30 issues with severity ratings, affected files, and suggested fix code. Critical and High items are in scope.

### Architecture context
- `.planning/codebase/ARCHITECTURE.md` — Current code structure, data flow, entry points. Needed to understand where fixes land.

### Code structure
- `.planning/codebase/STRUCTURE.md` — File locations and module boundaries

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- LOG() macro in logger.h — use for error logging in new error paths
- Connection state machine (CONN_ESTABLISHED → CONN_CLOSING) — SEC-05 fd sentinel fix integrates here
- Existing realpath() usage in file serving — SEC-01 fix extends this pattern

### Established Patterns
- Error codes as negative return values (e.g., INVALID_REQUEST_LINE = -711)
- Connection pool as fixed array indexed by slot number
- recv() into dynamic buffer with realloc — SEC-02 fix adds size cap to this pattern

### Integration Points
- main.c signal handler (lines 55-81) — SEC-04 rewrites this
- parsers.c parse_request_line/parse_headers — SEC-03 changes pointer storage to strndup copies
- http/server.c request_handler — SEC-01 path traversal fix in file serving section
- http/server.c proxy handling (~line 436) — SEC-02 buffer overflow fixes
- common.h — new MAX_HEADER_SIZE and MAX_BODY_SIZE constants added here

</code_context>

<specifics>
## Specific Ideas

No specific requirements — standard security fix approaches per CONCERNS.md suggested fixes.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 01-security-and-safety*
*Context gathered: 2026-03-18*
