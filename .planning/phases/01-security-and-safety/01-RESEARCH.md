# Phase 1: Security and Safety - Research

**Researched:** 2026-03-18
**Domain:** C security hardening -- path traversal, buffer overflow, use-after-free, signal safety, fd management, integer overflow
**Confidence:** HIGH

## Summary

Phase 1 is a surgical security fix phase targeting six specific vulnerability classes in an existing epoll-based C HTTP server (~600 lines of core logic). The codebase compiles cleanly with GCC C11 and has a minimal test suite (8 passing tests). All vulnerabilities and their fix approaches are well-documented in CONCERNS.md with concrete code snippets. Additionally, seven Critical/High severity bugs share code paths with the SEC-* fixes and must be addressed.

The fixes are standard C security patterns -- `realpath()` prefix checking, `strndup()` for field copying, `volatile sig_atomic_t` for signal flags, `strtoull()` for integer parsing, heap allocation with size caps, and `-1` fd sentinels. No external libraries are needed. The primary complexity is ordering fixes to avoid conflicts (e.g., parser strndup changes must happen before or alongside free_http_request changes, since strndup'd fields need freeing).

**Primary recommendation:** Fix in dependency order: fd sentinel (SEC-05) and adjacent bugs first (they touch connection lifecycle), then parser use-after-free (SEC-03, changes struct ownership semantics), then path traversal (SEC-01), buffer overflows (SEC-02), Content-Length parsing (SEC-06), and signal handling (SEC-04, touches main.c independently). Write smoke tests after each fix to verify.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Minimal in-place fixes -- smallest possible changes to existing code, no function extraction or refactoring
- Phase 2 handles the real restructuring; Phase 1 just makes the code safe
- Include basic smoke tests (curl-based or small C programs) to verify each fix works, no test framework needed yet
- For SEC-03 (parser use-after-free): strndup() ALL parsed fields (method, URI, protocol, all header names/values), add corresponding free() calls
- Fix all Critical and High severity bugs from CONCERNS.md, not just SEC-* requirements
- Additional fixes beyond SEC-01 through SEC-06:
  - Shadow variable in main.c (httpserver_ptr local shadows global)
  - Tautology in connection state check (OR to AND)
  - Double close of client fd (remove close from free_connection)
  - NULL check after dereference in response.c (check malloc before use)
  - realpath() memory leak in file serving (store, use, free)
  - Incomplete reset_connection() (initialize buffer_len and curr_request)
  - Failed accept() handling (don't close(-1))
- Code quality issues (debug printfs, unused macros, hardcoded path) left for Phase 2
- Signal handling: volatile sig_atomic_t flag, handler sets flag, main loop checks after epoll_wait()
- Handle SIGTERM + SIGINT (both set shutdown flag); ignore SIGPIPE
- On shutdown: immediate close -- break event loop, close all connections, free resources, exit
- No connection draining (graceful drain is v2 OPS-02)
- MAX_HEADER_SIZE = 8192 (8KB), reject with 431 if exceeded
- MAX_BODY_SIZE = 1048576 (1MB), reject with 413 if exceeded
- Content-Length max = same as body limit (1MB)
- Proxy response buffer also capped at 1MB, backend exceeds -> 502 Bad Gateway
- All limits as #define constants in common.h alongside existing MAX_CONNECTIONS
- Content-Length parsing: replace atoi() with strtoull(), body_length from int to size_t

### Claude's Discretion
- Exact order of fixing the 6 SEC-* vulns + adjacent bugs (dependency-aware sequencing)
- Smoke test implementation details (shell scripts vs C test programs)
- Whether to use calloc() vs malloc()+memset() for new allocations
- Temporary variable naming and code style within patches

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| SEC-01 | Fix path traversal vulnerability -- URL-decode first, realpath() both paths, verify resolved path starts with resolved root | realpath() prefix-check pattern documented below; current code at server.c:352-432 already uses realpath() for base_dir but does NOT validate the resolved filepath stays within root |
| SEC-02 | Fix buffer overflows in proxy code -- heap-allocate all request data with explicit size limits, no stack buffers for request data | Current proxy_request[INITIAL_BUFFER_SIZE] and proxy_response[INITIAL_BUFFER_SIZE] are stack buffers (4096 bytes); replace with malloc+size cap pattern; size limits defined in user constraints |
| SEC-03 | Fix use-after-free in parser -- copy all parsed fields with strndup(), no raw pointers into read buffer | parsers.c stores raw pointers at lines 26, 33, 40, 62, 73; strndup pattern documented below; free_http_request() must be updated to free all strndup'd strings |
| SEC-04 | Fix signal handler safety -- handler sets only volatile sig_atomic_t flag, all cleanup in main loop | Current handler at main.c:55-81 calls fprintf, httpserver_destructor, exit; replace with flag-only pattern using volatile sig_atomic_t |
| SEC-05 | Fix fd sentinel value -- use -1 (not 0) after close(), remove from epoll before freeing | Current code checks socket == 0 at server.c:86; init must set socket = -1; slot search must use == -1 |
| SEC-06 | Fix integer overflow in Content-Length parsing -- replace atoi with strtoull, body_length int to size_t | response.h body_length is int; must change to size_t; Content-Length parsing in commented body code uses atoi |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| POSIX C (glibc) | System | strndup, realpath, strtoull, sig_atomic_t | All fixes use standard POSIX/C11 functions already available |
| Linux epoll | System | Event loop (unchanged) | Existing infrastructure, no changes needed |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| valgrind | System | Verify no memory leaks or use-after-free post-fix | Smoke test validation of SEC-03 and adjacent memory fixes |
| curl | System | HTTP-level smoke tests | Verify path traversal blocked, size limits enforced, correct status codes |

No new dependencies required. All fixes use C standard library and POSIX functions already linked.

## Architecture Patterns

### Current File Layout (Unchanged by Phase 1)
```
src/
  main.c              # SEC-04 (signal handling), shadow variable fix
  common.h            # SEC-02/SEC-06 (new size limit #defines)
  http/
    server.c           # SEC-01 (path traversal), SEC-02 (proxy buffers), SEC-05 (fd sentinel), adjacent bugs
    server.h           # Connection struct unchanged
    parsers.c          # SEC-03 (strndup all fields)
    request.c          # SEC-03 (free strndup'd fields in free_http_request)
    request.h          # Struct unchanged (pointers now owned, not borrowed)
    response.c         # SEC-06 (body_length type), NULL-check-before-use fix
    response.h         # SEC-06 (body_length int -> size_t)
```

### Pattern 1: realpath() Prefix Check (SEC-01)
**What:** After constructing a filepath from user input, resolve it with realpath() and verify the resolved path begins with the resolved document root.
**When to use:** Any time a URI or user-supplied path is used to access the filesystem.
**Example:**
```c
// 1. Resolve the document root
char *root = realpath(BASE_DIR, NULL);
if (!root) { /* 500 error */ }

// 2. Build candidate path from URI
char filepath[PATH_MAX];
snprintf(filepath, sizeof(filepath), "%s%.*s", root, (int)uri_len, uri);

// 3. Resolve candidate (follows symlinks, normalizes ../)
char *resolved = realpath(filepath, NULL);
if (!resolved) {
    free(root);
    return error_response(404, "Not Found");
}

// 4. Verify resolved path starts with root
if (strncmp(resolved, root, strlen(root)) != 0) {
    free(resolved);
    free(root);
    return error_response(403, "Forbidden");
}

// 5. Use resolved path for file operations
free(root);
// ... open(resolved, O_RDONLY) ...
free(resolved);
```

### Pattern 2: strndup() Field Copying (SEC-03)
**What:** Replace raw pointer storage into a shared buffer with independent heap copies of each parsed field.
**When to use:** Whenever the parser extracts a substring from the receive buffer.
**Example:**
```c
// Before (dangling pointer if buffer realloc'd):
req_t->request_line.method = (char *)ptr;
req_t->request_line.method_len = space - ptr;

// After (independent copy):
req_t->request_line.method = strndup(ptr, space - ptr);
if (!req_t->request_line.method) return -1;
req_t->request_line.method_len = space - ptr;
```

Corresponding free in `free_http_request()`:
```c
void free_http_request(HTTPRequest *req) {
    if (!req) return;
    free(req->request_line.method);
    free(req->request_line.uri);
    free(req->request_line.protocol);
    for (int i = 0; i < req->header_count; i++) {
        free(req->headers[i].name);
        free(req->headers[i].value);
    }
    free(req->headers);
    free(req);
}
```

### Pattern 3: volatile sig_atomic_t Signal Flag (SEC-04)
**What:** Signal handler only sets an atomic flag. Main loop polls the flag and performs cleanup outside the handler.
**When to use:** Any signal handler that needs to trigger shutdown.
**Example:**
```c
#include <signal.h>
#include <stdatomic.h>

static volatile sig_atomic_t shutdown_flag = 0;

void handle_signal(int sig) {
    (void)sig;
    shutdown_flag = 1;
}

// In main():
struct sigaction sa;
memset(&sa, 0, sizeof(sa));
sa.sa_handler = handle_signal;
sigemptyset(&sa.sa_mask);
sigaction(SIGINT, &sa, NULL);
sigaction(SIGTERM, &sa, NULL);
signal(SIGPIPE, SIG_IGN);
// Do NOT handle SIGSEGV -- let it crash with core dump

// In event loop:
while (!shutdown_flag) {
    int n_ready = epoll_wait(self->epoll_fd, events, MAX_EPOLL_EVENTS, 1000);
    if (n_ready == -1) {
        if (errno == EINTR) continue;  // signal interrupted epoll_wait
        break;
    }
    // ... handle events ...
}
// Cleanup here (outside signal handler)
```

### Pattern 4: strtoull() Safe Integer Parsing (SEC-06)
**What:** Replace atoi() with strtoull() and validate the result against defined limits.
**When to use:** Parsing Content-Length or any numeric header value.
**Example:**
```c
char cl_str[32];  // extracted Content-Length value
snprintf(cl_str, sizeof(cl_str), "%.*s", (int)value_len, value);

char *endptr;
errno = 0;
unsigned long long cl = strtoull(cl_str, &endptr, 10);
if (errno != 0 || *endptr != '\0' || cl > MAX_BODY_SIZE) {
    // Reject: 413 Payload Too Large (or 400 Bad Request for parse error)
}
req->body_len = (size_t)cl;
```

### Pattern 5: Heap-Allocated Proxy Buffers with Size Cap (SEC-02)
**What:** Replace stack-allocated fixed buffers with heap allocation and enforce maximum size.
**When to use:** Proxy request building and proxy response reading.
**Example:**
```c
// Proxy request: calculate size first, then allocate
size_t header_len = (size_t)snprintf(NULL, 0, "...format...", args);
size_t total_len = header_len + request_ptr->body_len;
if (total_len > MAX_BODY_SIZE) {
    return error_response(413, "Payload Too Large");
}
char *proxy_request = malloc(total_len + 1);
if (!proxy_request) { /* 500 */ }
snprintf(proxy_request, header_len + 1, "...format...", args);
if (request_ptr->body && request_ptr->body_len > 0) {
    memcpy(proxy_request + header_len, request_ptr->body, request_ptr->body_len);
}
proxy_request[total_len] = '\0';
// ... send, then free(proxy_request) ...

// Proxy response: dynamic read loop with cap
size_t resp_cap = INITIAL_BUFFER_SIZE;
size_t resp_len = 0;
char *proxy_response = malloc(resp_cap);
while (1) {
    ssize_t n = recv(backend_fd, proxy_response + resp_len, resp_cap - resp_len, 0);
    if (n <= 0) break;
    resp_len += n;
    if (resp_len > MAX_BODY_SIZE) {
        free(proxy_response);
        close(backend_fd);
        return error_response(502, "Bad Gateway");
    }
    if (resp_len >= resp_cap) {
        resp_cap *= 2;
        if (resp_cap > MAX_BODY_SIZE + INITIAL_BUFFER_SIZE)
            resp_cap = MAX_BODY_SIZE + INITIAL_BUFFER_SIZE;
        char *tmp = realloc(proxy_response, resp_cap);
        if (!tmp) { free(proxy_response); /* 500 */ }
        proxy_response = tmp;
    }
}
```

### Anti-Patterns to Avoid
- **Refactoring during security fixes:** Do NOT extract functions, reorganize modules, or rename APIs. Phase 2 handles restructuring.
- **Catching SIGSEGV:** The current code catches SIGSEGV. Remove this -- SIGSEGV should crash with a core dump for debugging, not attempt cleanup.
- **Using signal() instead of sigaction():** `signal()` behavior is implementation-defined across calls. Use `sigaction()` for reliable semantics.
- **Freeing response_str after error:** Current code at server.c:293 calls `free(response_str)` unconditionally but it may already be NULL from a failed serialize. Guard with NULL check or ensure it is only reached when non-NULL (current code structure already guards this via else chain, but verify carefully).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Path normalization | Custom `..` stripping regex | `realpath()` + prefix check | realpath resolves symlinks, double-encoded sequences, null bytes; hand-rolled stripping always misses edge cases |
| Integer parsing with overflow detection | Custom digit loop | `strtoull()` with errno + endptr check | strtoull handles ULLONG_MAX overflow, leading whitespace, sign; hand-rolled parsers miss edge cases |
| String field extraction | Manual malloc+memcpy | `strndup()` | strndup is POSIX standard, null-terminates, handles allocation; fewer lines means fewer bugs |
| Async-signal-safe messaging | Custom write() wrappers | Simply set flag, no messaging | Any I/O in signal handlers is risky; just set the flag |

**Key insight:** Every vulnerability in this phase exists because the original code hand-rolled something instead of using the standard POSIX solution. The fixes are literally "use the standard function."

## Common Pitfalls

### Pitfall 1: strndup Changes Ownership Semantics
**What goes wrong:** After SEC-03, request fields are heap-allocated strings owned by the HTTPRequest. Code that currently uses length-delimited access (`%.*s` format) still works, but `free_http_request()` MUST free every strndup'd field. Missing a single free causes a leak; double-freeing causes a crash.
**Why it happens:** The existing `free_http_request()` only frees `req->headers` array and `req`. It does NOT free individual field strings because they were borrowed pointers.
**How to avoid:** Update `free_http_request()` FIRST (or simultaneously with parser changes). Free method, uri, protocol, and each header name+value. Use NULL checks before each free.
**Warning signs:** Valgrind reports "definitely lost" blocks equal to the number of parsed fields.

### Pitfall 2: Buffer Size Cap Interacts with recv() Loop
**What goes wrong:** Adding MAX_HEADER_SIZE cap to the recv buffer means the realloc growth in server.c:206-216 must stop at the cap. But the current code `realloc(buffer, new_size)` with `new_size = buffer_size * 2` will exceed 8KB quickly.
**Why it happens:** The cap must be checked BEFORE attempting realloc, and the recv() call must not request more bytes than (cap - buffer_len).
**How to avoid:** Add size check: if `buffer_len >= MAX_HEADER_SIZE`, reject with 431. Do this check right after `conn->buffer_len += bytes_read`.
**Warning signs:** Server accepts multi-megabyte headers without rejection.

### Pitfall 3: epoll_wait Returns EINTR on Signal
**What goes wrong:** After installing signal handlers, `epoll_wait()` can return -1 with errno=EINTR when a signal is delivered. The current code logs an error and continues, which is correct, but the shutdown flag check must happen in the while condition.
**Why it happens:** Signal delivery interrupts blocking syscalls.
**How to avoid:** Change `while(1)` to `while(!shutdown_flag)` and handle EINTR explicitly (just continue the loop).
**Warning signs:** Server logs "Failed to wait for epoll events" on every SIGINT before shutting down.

### Pitfall 4: fd Sentinel Change Breaks Existing Slot Search
**What goes wrong:** Changing the sentinel from 0 to -1 requires updating BOTH the slot search (`socket == 0` -> `socket == -1`) AND the initialization. If `calloc()` is used for the connections array (current code, line 34), all sockets start as 0, which is now a VALID fd, not the sentinel.
**Why it happens:** calloc zero-initializes, and 0 was the old sentinel.
**How to avoid:** After calloc for connections array, loop through and set each `connections[j].socket = -1`. Or add an explicit initialization loop.
**Warning signs:** All connection slots appear "occupied" immediately after server startup.

### Pitfall 5: Proxy Code Uses Uninitialized Body Fields
**What goes wrong:** The proxy request builder at server.c:448 references `request_ptr->body_len` and server.c:459 references `request_ptr->body`. But body parsing is commented out (parsers.c:112-137), so body is always NULL and body_len is always 0.
**Why it happens:** Phase 1 does not re-enable body parsing (that is a Phase 3 concern). The strncat of NULL body is undefined behavior.
**How to avoid:** Guard the body append: `if (request_ptr->body && request_ptr->body_len > 0)`. This makes it safe regardless of whether body parsing is enabled.
**Warning signs:** Crash on POST /api/ requests even after SEC-02 fix.

### Pitfall 6: realpath() Returns NULL for Non-Existent Files
**What goes wrong:** If the requested file does not exist, `realpath(filepath, NULL)` returns NULL. This is correct behavior -- return 404. But the code must not dereference the NULL pointer.
**Why it happens:** realpath only succeeds for paths that actually exist on disk.
**How to avoid:** Check `resolved == NULL` immediately and return 404 before attempting the prefix comparison.
**Warning signs:** SIGSEGV when requesting a non-existent file under /static/.

## Code Examples

### SEC-05: fd Sentinel Initialization After calloc
```c
// In launch(), after allocating connections array:
self->connections = calloc(MAX_CONNECTIONS, sizeof(Connection));
if (!self->connections) { /* error handling */ }
for (size_t j = 0; j < MAX_CONNECTIONS; j++) {
    self->connections[j].socket = -1;
}
self->active_count = 0;

// Slot search:
for (size_t j = 0; j < MAX_CONNECTIONS; j++) {
    if (self->connections[j].socket == -1) {
        conn = &self->connections[j];
        break;
    }
}

// In free_connection():
conn->socket = -1;  // already correct in current code

// Error paths in accept handling:
conn->socket = -1;  // not 0
```

### Adjacent Bug: Shadow Variable Fix (main.c)
```c
// Current code at main.c:33 is ALREADY CORRECT:
httpserver_ptr = httpserver_constructor(...);
// The CONCERNS.md described the original issue, but it appears
// the code was already fixed. Verify by checking if the type
// qualifier 'HTTPServer *' is present on line 33.
```
**NOTE:** Reading main.c:33 shows `httpserver_ptr =` (no type qualifier). This bug appears to already be fixed in the current code. Verify during implementation.

### Adjacent Bug: Tautology Fix (server.c:221)
```c
// Current code at server.c:221:
if (conn->state != CONN_CLOSING && conn->state != CONN_ERROR)
// This is ALREADY the correct AND form. Verify during implementation.
```
**NOTE:** Reading server.c:221 shows `&&` not `||`. This bug also appears to already be fixed. The CONCERNS.md may describe the original state before prior fixes. Verify all "adjacent bugs" during implementation -- some may already be resolved.

### Size Limit Constants (common.h)
```c
// Add to common.h alongside existing MAX_CONNECTIONS:
#define MAX_HEADER_SIZE  8192       /* 8 KB - reject with 431 */
#define MAX_BODY_SIZE    1048576    /* 1 MB - reject with 413 */
```

### Buffer Size Enforcement in recv Loop
```c
// After conn->buffer_len += bytes_read (server.c ~line 200):
if (conn->buffer_len > MAX_HEADER_SIZE) {
    LOG("ERROR", "Request headers exceed %d bytes, rejecting", MAX_HEADER_SIZE);
    // Send 431 response, then set CONN_CLOSING
    conn->state = CONN_CLOSING;
    break;
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| signal() for handler registration | sigaction() | POSIX.1-2001 | signal() has undefined re-registration semantics; sigaction() is portable |
| atoi() for integer parsing | strtol()/strtoull() | C89 (strtol), C99 (strtoull) | atoi cannot detect overflow or invalid input |
| Raw pointer into shared buffer | strndup() copies | POSIX.1-2008 | Eliminates use-after-free from buffer reallocation |
| Stack buffers for variable-size data | Heap allocation with size cap | Always best practice | Stack overflow is unexploitable on modern systems but still causes crashes |

**Deprecated/outdated:**
- `signal()`: Use `sigaction()` instead -- `signal()` behavior varies across implementations
- `atoi()`: Never use for untrusted input -- cannot detect overflow or non-numeric input
- Catching SIGSEGV for cleanup: Let SIGSEGV generate a core dump; cleanup in SIGSEGV handler is undefined behavior

## Open Questions

1. **Are some "adjacent bugs" already fixed?**
   - What we know: Reading main.c and server.c suggests the shadow variable (line 33 has no type qualifier) and tautology (line 221 uses &&) may already be corrected.
   - What's unclear: Whether these were fixed in a commit not reflected in CONCERNS.md, or whether I'm misreading the code.
   - Recommendation: Verify each adjacent bug by reading the exact current code during implementation. If already fixed, skip and note as "verified correct."

2. **Should the recv buffer cap apply to headers only or headers+body?**
   - What we know: MAX_HEADER_SIZE = 8KB for headers, MAX_BODY_SIZE = 1MB for body. Body parsing is currently disabled (commented out).
   - What's unclear: Since body parsing is disabled, the recv buffer only accumulates headers. The 8KB cap is sufficient for Phase 1. When body parsing is re-enabled in Phase 3, body data will also flow through this buffer.
   - Recommendation: Apply the 8KB cap in Phase 1 (headers only). Phase 3 will need a two-stage buffer approach (headers capped at 8KB, then body capped at 1MB based on Content-Length).

3. **URL decoding before path traversal check**
   - What we know: SEC-01 says "URL-decode first" but the codebase has no URL decode function.
   - What's unclear: Whether to add a URL decoder in Phase 1 (minimal approach) or defer to Phase 3 parsing improvements.
   - Recommendation: Add a minimal percent-decode function (decode `%XX` sequences) before the realpath() call. This is essential for SEC-01 -- without it, `%2e%2e%2f` bypasses the traversal check. Keep it minimal: a static helper function in server.c, not a new module.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Custom ASSERT macro + CTest (existing in tests/test_parsers.c) |
| Config file | CMakeLists.txt (lines 39-45) |
| Quick run command | `cd build && ctest --output-on-failure` |
| Full suite command | `cd build && cmake .. && make && ctest --output-on-failure` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SEC-01 | Path traversal sequences blocked | smoke (shell) | `bash tests/smoke/test_sec01_path_traversal.sh` | No -- Wave 0 |
| SEC-02 | Oversized requests rejected (431/413) | smoke (shell) | `bash tests/smoke/test_sec02_buffer_overflow.sh` | No -- Wave 0 |
| SEC-03 | Parser fields are independent copies | unit (C) | `cd build && ctest --output-on-failure -R unit_tests` | Partial -- existing parser tests verify parsing but not buffer-independence |
| SEC-04 | Clean shutdown on SIGTERM/SIGINT | smoke (shell) | `bash tests/smoke/test_sec04_signal.sh` | No -- Wave 0 |
| SEC-05 | fd set to -1 after close, no double-close | unit (C) | `cd build && ctest --output-on-failure -R unit_tests` | No -- Wave 0 |
| SEC-06 | Content-Length overflow rejected | smoke (shell) | `bash tests/smoke/test_sec06_content_length.sh` | No -- Wave 0 |

### Sampling Rate
- **Per task commit:** `cd build && make && ctest --output-on-failure`
- **Per wave merge:** `cd build && cmake .. && make && ctest --output-on-failure` + all smoke tests
- **Phase gate:** Full suite green + all smoke tests pass + valgrind clean run

### Wave 0 Gaps
- [ ] `tests/smoke/` directory -- create for shell-based smoke tests
- [ ] `tests/smoke/test_sec01_path_traversal.sh` -- curl requests with `/../`, `%2e%2e`, verify 403
- [ ] `tests/smoke/test_sec02_buffer_overflow.sh` -- send oversized headers (>8KB), verify 431; oversized body (>1MB), verify 413
- [ ] `tests/smoke/test_sec04_signal.sh` -- start server, send request, SIGTERM, verify clean exit (no valgrind errors)
- [ ] `tests/smoke/test_sec06_content_length.sh` -- send Content-Length: 99999999999999999, verify rejection
- [ ] Update `free_http_request()` in request.c -- must free strndup'd fields (SEC-03 prerequisite)
- [ ] Add SEC-03 regression test to test_parsers.c -- parse, realloc buffer, verify fields still valid

## Sources

### Primary (HIGH confidence)
- Direct source code inspection of all files in src/ -- verified current state of each vulnerability
- POSIX.1-2008 specification -- realpath(), strndup(), sigaction(), strtoull(), sig_atomic_t semantics
- CONCERNS.md -- comprehensive vulnerability catalog with severity ratings and fix approaches

### Secondary (MEDIUM confidence)
- CERT C Coding Standard SIG30-C -- "Call only asynchronous-safe functions within signal handlers"
- CERT C Coding Standard FIO34-C -- "Distinguish between characters read from a file and EOF or WEOF"

### Tertiary (LOW confidence)
- None -- all findings verified against source code and POSIX specifications

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all fixes use standard POSIX/C11 functions already available on the system
- Architecture: HIGH -- no architectural changes; surgical fixes in known file locations with documented line numbers
- Pitfalls: HIGH -- all pitfalls identified from direct code inspection of actual vulnerability sites
- Validation: MEDIUM -- smoke test approach is straightforward but requires server to be running; some tests need a backend service

**Research date:** 2026-03-18
**Valid until:** 2026-04-18 (stable -- C/POSIX APIs do not change)
