# Codebase Concerns

**Analysis Date:** 2026-03-17

## Critical Bugs

### Shadow Variable in main.c (Line 33)

**Issue:** Local variable `httpserver_ptr` shadows the global variable of the same name.

**Files:** `src/main.c:33`

**Impact:** Assignment on line 33 creates a new local variable instead of assigning to the global. The global remains `NULL`, causing dereference failures in `handle_signal()` which tries to use the global pointer.

**Fix approach:** Remove the type qualifier from the assignment:
```c
// Before (creates shadow variable):
HTTPServer *httpserver_ptr = httpserver_constructor(...);

// After (assigns to global):
httpserver_ptr = httpserver_constructor(...);
```

---

### Tautology in Connection State Check (http/server.c:222)

**Issue:** Condition `if (conn->state != CONN_CLOSING || conn->state != CONN_ERROR)` is always true.

**Files:** `src/http/server.c:222`

**Impact:** The condition cannot distinguish between valid and invalid states. Any state value will satisfy at least one of the two OR branches, making the check meaningless.

**Fix approach:** Change OR to AND:
```c
// Before (always true):
if (conn->state != CONN_CLOSING || conn->state != CONN_ERROR)

// After (correctly checks both):
if (conn->state != CONN_CLOSING && conn->state != CONN_ERROR)
```

---

### Double Close of Client File Descriptor

**Issue:** `free_connection()` closes `conn->socket`, then the caller closes the same `client_fd` again.

**Files:** `src/http/server.c` (connection handling logic)

**Impact:** Closes the same file descriptor twice, which can close a different connection's socket if a new connection reuses the fd. This is a resource safety violation that can cause data corruption or crashes.

**Fix approach:** Remove the close() call from `free_connection()`. The caller owns the fd lifetime and is responsible for closing it. `free_connection()` should only clean internal buffers:
```c
int free_connection(Connection *conn) {
    free(conn->buffer);
    conn->buffer = NULL;
    conn->buffer_size = 0;
    conn->socket = -1;  // sentinel, don't close
    return 0;
}
```

---

### Invalid Socket Error Check (sock/server.c:30)

**Issue:** Code checks `if (server_ptr->socket < 0)` on line 30, but should check `< 0` consistently.

**Files:** `src/sock/server.c:30`

**Impact:** File descriptor 0 (stdin) is technically a valid socket value. The check should use `< 0` (which is the standard sentinel for invalid fd).

**Fix approach:** Ensure all socket validity checks use `< 0` not `== 0`.

---

### NULL Check After Dereference (response.c)

**Issue:** In `response_builder()` line 114, `malloc()` is called and the pointer is used without NULL check before checking.

**Files:** `src/http/response.c:114-121`

**Impact:** If `malloc()` returns NULL, `memset()` on line 121 will dereference a NULL pointer before the NULL check.

**Fix approach:** Check malloc result before use, or use `calloc()` which zeroes memory:
```c
// Better approach (calloc handles zero-fill):
response->body = calloc(1, body_length);
if (!response->body) {
    httpresponse_free(response);
    return NULL;
}
```

---

### Memory Leak from realpath() (http/server.c)

**Issue:** Multiple calls to `realpath(BASE_DIR, NULL)` are embedded in `snprintf()` calls without freeing the returned allocation.

**Files:** `src/http/server.c` (file serving logic)

**Impact:** Every static file request leaks dynamically allocated memory from `realpath()`. Under load, this accumulates unbounded memory consumption.

**Fix approach:** Store `realpath()` result in a temporary variable, use it, then free:
```c
char *base = realpath(BASE_DIR, NULL);
if (!base) { /* error */ }
snprintf(filepath, sizeof(filepath), "%s%.*s", base, ...);
free(base);
```

---

### reset_connection() Incomplete Cleanup (http/server.c)

**Issue:** `reset_connection()` does not initialize all fields that were set during connection setup, particularly `buffer_len` and `curr_request`.

**Files:** `src/http/server.c` (reset_connection function)

**Impact:** Reused connections retain stale state from previous requests. Old request data can leak into new requests or cause parser crashes.

**Fix approach:** Initialize all critical fields:
```c
int reset_connection(Connection *conn) {
    memset(conn->buffer, 0, conn->buffer_size);
    conn->buffer_len = 0;            // Add this
    conn->state = CONN_ESTABLISHED;
    if (conn->curr_request) {
        free_http_request(conn->curr_request);
        conn->curr_request = NULL;   // Add this
    }
    return 0;
}
```

---

### Failed accept() Not Handled (http/server.c:76-80)

**Issue:** When `accept()` fails and returns -1, the code tries to `close(-1)`.

**Files:** `src/http/server.c:76-80`

**Impact:** Attempting to close fd -1 is harmless but incorrect. The proper action is to log the error and continue the loop.

**Fix approach:** Remove the close() call:
```c
if (client_fd == -1) {
    LOG("ERROR", "Failed to accept connection: %s", strerror(errno));
    continue;  // don't close(-1)
}
```

---

## Security Issues

### Path Traversal Vulnerability (http/server.c)

**Issue:** Static file serving concatenates URIs directly onto `BASE_DIR` without validating that the final path stays within the static directory.

**Files:** `src/http/server.c` (static file handler)

**Impact:** Attacker can request `GET /../../../etc/passwd` and escape the intended directory. This is a **critical vulnerability** allowing arbitrary file read.

**Fix approach:** After resolving the full filepath, validate it starts with the base directory:
```c
char *resolved = realpath(filepath, NULL);
if (!resolved) {
    return error_response(404, "Not Found");
}
char *root = realpath(BASE_DIR, NULL);
if (!root || strncmp(resolved, root, strlen(root)) != 0) {
    free(resolved);
    free(root);
    return error_response(403, "Forbidden");
}
free(root);
// proceed with resolved path
```

---

### Proxy Request Buffer Overflow (http/server.c:436)

**Issue:** Proxy request building uses a fixed-size stack buffer with `strncat()` without bounds checking. If the body exceeds available space, buffer overflow occurs.

**Files:** `src/http/server.c:436` (proxy handler)

**Impact:** Stack corruption. Attacker sending a large POST body to `/api/` endpoint can corrupt return addresses or other stack data.

**Fix approach:** Allocate dynamically and calculate total size before building:
```c
size_t header_len = snprintf(NULL, 0, "...format...", args);
size_t total_len = header_len + request_ptr->body_len;
char *proxy_request = malloc(total_len + 1);
if (!proxy_request) { /* OOM */ }
// Build header then copy body
memcpy(proxy_request + header_len, request_ptr->body, request_ptr->body_len);
proxy_request[total_len] = '\0';
```

---

### Proxy Response Buffer Overflow (http/server.c)

**Issue:** Proxy response is read into a fixed-size stack buffer with no size limit.

**Files:** `src/http/server.c` (proxy response handling)

**Impact:** A backend returning a large response will overflow the buffer. Stack corruption identical to proxy request overflow.

**Fix approach:** Implement a dynamic read loop, reallocating as needed, matching the pattern used for client recv().

---

### Signal Handler Safety Violation (main.c:55-81)

**Issue:** `handle_signal()` calls `fprintf()`, `exit()`, and other async-signal-unsafe functions inside a signal handler.

**Files:** `src/main.c:55-81`

**Impact:** POSIX signals can be delivered at any time. Calling non-async-safe functions (allocators, locks, buffered I/O) can deadlock or corrupt heap state. The signal handler calls `httpserver_destructor()` which frees memory.

**Fix approach:** Only perform async-safe operations. Use an atomic flag:
```c
#include <stdatomic.h>
static volatile atomic_int shutdown_requested = 0;

void handle_signal(int sig) {
    const char *msg = "[!] Signal received, shutting down...\n";
    write(STDERR_FILENO, msg, strlen(msg));  // async-safe
    atomic_store(&shutdown_requested, 1);
}

// Main loop:
while (!atomic_load(&shutdown_requested)) {
    int n_ready = epoll_wait(...);
    ...
}
// Clean up after loop exits
```

---

## Protocol Correctness Issues

### Missing Content-Length and Content-Type Headers

**Issue:** HTTP responses do not include `Content-Length` and `Content-Type` headers. Clients cannot determine response size or how to parse the body.

**Files:** `src/http/response.c` (response_builder function)

**Impact:** HTTP/1.1 requires `Content-Length` for body responses. Without it, clients may hang waiting for more data. Browsers may not render responses correctly without `Content-Type`.

**Fix approach:** Add headers to `response_builder()`:
```c
char content_length_str[32];
snprintf(content_length_str, sizeof(content_length_str), "%zu", body_length);
httpresponse_add_header(res, "Content-Length", content_length_str);
httpresponse_add_header(res, "Content-Type", content_type);
httpresponse_add_header(res, "Server", "cserve/0.1");
```

---

### Request Body Parsing Disabled

**Issue:** Body parsing code in `parsers.c:112-137` is commented out. POST/PUT requests with bodies are silently ignored.

**Files:** `src/http/parsers.c:112-137`

**Impact:** Request bodies are never parsed. POST endpoints receive empty bodies. Proxy requests lack the original client body.

**Fix approach:** Uncomment and fix the body parsing logic. Use `strtol()` instead of `atoi()` for `Content-Length` to avoid integer overflow with large bodies.

---

### EAGAIN Handling Incomplete (http/server.c:160-161)

**Issue:** When `send()` returns EAGAIN, the code breaks the send loop but doesn't register EPOLLOUT for write readiness.

**Files:** `src/http/server.c:160-161`

**Impact:** If the send buffer fills, the response is truncated. The client receives incomplete data and the connection closes.

**Fix approach:** Register EPOLLOUT event so epoll notifies when socket is writable again. Or buffer the response and try again on next event.

---

### Incorrect Proxy Response Body Length (http/server.c)

**Issue:** Proxy response body length is calculated as `proxy_request_len` (request length) instead of actual response body length.

**Files:** `src/http/server.c` (proxy response body calculation)

**Impact:** Response includes wrong Content-Length header value. Client reads incorrect amount of data.

**Fix approach:** Calculate body length as `response_body_end - response_body_start`, not from request.

---

### Case-Sensitive Header Comparison

**Issue:** Header names and values use `strncmp()` for comparison (case-sensitive), but HTTP headers are case-insensitive.

**Files:** `src/http/parsers.c`, `src/http/server.c` (header parsing and matching)

**Impact:** Headers like `Content-length` (mixed case) won't match `Content-Length`. Keep-Alive detection fails if client sends `keep-alive` instead of `Keep-Alive`.

**Fix approach:** Use `strncasecmp()` for header name and value comparisons.

---

## Architectural Issues

### Monolithic launch() Function (http/server.c:11)

**Issue:** The `launch()` function contains all event loop logic: accept, read, parse, dispatch, send. It is 635+ lines with multiple nested loops and complex state management.

**Files:** `src/http/server.c:11`

**Impact:** Single function with >5 responsibilities makes code hard to test, modify, and reason about. Bugs hide in deep nesting. Cannot test individual stages (parsing, dispatch) without running the full server.

**Fix approach:** Decompose into:
- `accept_new_connection()`
- `handle_readable()`
- `recv_into_buffer()`
- `process_connection()`
- `parse_request()`
- `dispatch_request()`
- `send_response()`

Each function fits on one screen and has testable contract.

---

### Dangling Pointers in Parser (parsers.c:11-46)

**Issue:** `parse_request_line()` stores direct pointers into the input buffer (e.g., line 26: `req_t->request_line.method = (char *)ptr`).

**Files:** `src/http/parsers.c:11-46`

**Impact:** If the buffer is reallocated (lines 206-216 in server.c), all stored pointers become invalid. Subsequent use (e.g., logging, serialization) reads garbage memory. This is a **use-after-free bug**.

**Fix approach:** Copy field values at parse time instead of storing buffer pointers:
```c
req->request_line.method = strndup(ptr, method_len);
req->request_line.method_len = method_len;
```

Free these strings in `free_http_request()`.

---

### Invalid Socket Sentinel Value (http/server.c)

**Issue:** Code checks `conn->socket == 0` to detect unused slots (lines 86, 117, 130), but 0 is a valid file descriptor (stdin).

**Files:** `src/http/server.c:86, 117, 130`

**Impact:** If stdin/stdout/stderr are redirected away, a legitimate socket could have fd 0 and be confused with an empty slot. Or an empty slot check could incorrectly match a valid connection.

**Fix approach:** Use -1 as the sentinel for "no socket":
```c
// Initialization:
conn->socket = -1;

// Slot search:
if (self->connections[j].socket == -1) { /* empty */ }
```

---

### Missing Include Guards (parsers.h, logger.h)

**Issue:** Header files `src/http/parsers.h` and `src/utils/logger.h` lack `#ifndef` include guards.

**Files:** `src/http/parsers.h`, `src/utils/logger.h`

**Impact:** Multiple includes of the same header in a translation unit cause redefinition errors. C preprocessor includes produce duplicate definitions.

**Fix approach:** Add guards to all headers:
```c
#ifndef CSERVE_PARSERS_H
#define CSERVE_PARSERS_H
// ...
#endif /* CSERVE_PARSERS_H */
```

---

### common.h Over-Inclusion (common.h)

**Issue:** `common.h` includes 15+ system headers as a catch-all and is included everywhere. This creates implicit hidden dependencies and makes it hard to refactor.

**Files:** `src/common.h:12-32`

**Impact:** Every source file depends on every header included by common.h, even if it doesn't use them. Changing common.h rebuilds everything. Module isolation is lost.

**Fix approach:** Each `.h` should include only what it directly uses:
- Move connection/HTTP constants to `http/server.h`
- Move error codes to `error.h`
- Move HTTP method/state enums to `http/types.h`
- Only include `logger.h` where logging is used

---

### Hardcoded Personal Path in common.h (common.h:44)

**Issue:** `#define DEFAULT_CONFIG_PATH "/home/voidp/Projects/samandar/1lang1server/cserver"`

**Files:** `src/common.h:44`

**Impact:** Hardcoded absolute path to developer's machine. Code cannot be used on any other system. Path will not exist in CI/CD or production.

**Fix approach:** Remove the macro. Set config path via command-line argument, environment variable, or CMake define:
```bash
cmake -DDEFAULT_CONFIG_PATH=/etc/cserve.ini ..
```

---

### Type Mismatch: body_length (response.h)

**Issue:** `HTTPResponse.body_length` is declared as `int` but holds a size value and is compared with `size_t`.

**Files:** `src/http/response.h` (struct definition)

**Impact:** Size values that exceed INT_MAX are truncated or negative, causing silent data loss. Compiler warnings on size_t comparisons.

**Fix approach:** Change to `size_t`:
```c
typedef struct {
    // ...
    size_t body_length;  // not int
} HTTPResponse;
```

---

## Code Quality Issues

### Debug printf() Statements (parsers.c, server.c)

**Issue:** Uncontrolled `printf()` calls appear throughout the codebase (parsers.c:15, 53, 86, 94, 108, 150-152; server.c:150-152).

**Files:** `src/http/parsers.c`, `src/http/server.c`

**Impact:** Debug output goes to stdout alongside actual responses. Pollutes logs. Cannot be disabled. Makes testing harder.

**Fix approach:** Remove all `printf()` calls or convert to `LOG("DEBUG", ...)` behind a debug level setting.

---

### Empty Function Body (main.c:83)

**Issue:** `init_config()` function on line 83 is empty and never called.

**Files:** `src/main.c:83`

**Impact:** Dead code. Confuses readers about initialization flow.

**Fix approach:** Remove the function.

---

### Unused Macros (common.h:34-35)

**Issue:** Macros `str(x)` and `xstr(x)` are defined but never used.

**Files:** `src/common.h:34-35`

**Impact:** Dead code clutters the header.

**Fix approach:** Remove unused macros.

---

### Unused Enum (sock/server.h)

**Issue:** `TransportType` enum is defined but never instantiated or used.

**Files:** `src/sock/server.h`

**Impact:** Dead code increases complexity.

**Fix approach:** Remove the enum.

---

### Unsafe String Parsing (config.c:61)

**Issue:** Config parser uses `atoi()` for port parsing. `atoi()` silently returns 0 on invalid input.

**Files:** `src/utils/config.c:61`

**Impact:** Malformed config file with invalid port value silently becomes port 0 (invalid). No error is reported.

**Fix approach:** Use `strtol()` with error checking:
```c
char *endptr;
errno = 0;
long val = strtol(value, &endptr, 10);
if (errno || *endptr != '\0' || val <= 0 || val > 65535) {
    LOG("ERROR", "Invalid port: %s", value);
    return NULL;
}
cfg->port = (int)val;
```

---

## Testing Gaps

### No Unit Tests for Core Logic

**Issue:** Test file `tests/test_parsers.c` exists but contains only placeholder test. No meaningful tests for parsing, request/response handling, or routing logic.

**Files:** `tests/test_parsers.c`, `tests/test_sqrt.c`

**Impact:** Bugs in critical paths (parsing, dispatching) have no automated catch. Regressions go undetected. Refactoring is risky.

**Priority:** High - these are security-critical code paths.

**Approach:** Write comprehensive unit tests using libcheck:
- Parser tests against valid/invalid/edge-case HTTP
- Response builder tests for header generation
- Router tests for URI matching
- Config parser tests for valid/invalid INI files

---

### No Integration Tests

**Issue:** No end-to-end tests. No way to verify a complete HTTP request/response cycle works.

**Files:** None exist

**Impact:** Integration bugs (like malformed response headers) go undetected until production.

**Approach:** Add shell script using curl to test real HTTP requests and validate responses.

---

### No Error Path Testing

**Issue:** Code paths for malloc failures, socket errors, and invalid input are never exercised.

**Files:** All error handling code

**Impact:** Error handling may be broken and only discovered under stress or when actually needed.

**Approach:** Write tests that inject failures (mock malloc returning NULL, etc.).

---

## Fragility

### Reused Connections Without Full Reset

**Issue:** Keep-alive connections are reset with `reset_connection()` which doesn't fully initialize state. Combined with parser bugs storing pointers into the buffer, reused connections are fragile.

**Files:** `src/http/server.c` (keep-alive logic), `src/http/parsers.c` (dangling pointers)

**Impact:** Second and subsequent requests on keep-alive connections may crash or corrupt state.

**Safe modification:** Don't reuse connections until parser is fixed to copy strings, and reset_connection() properly clears all state.

---

### Global httpserver_ptr in main.c

**Issue:** Global mutable state shared between main thread and signal handler.

**Files:** `src/main.c:17`

**Impact:** Vulnerable to race conditions. If main thread is freeing httpserver_ptr while signal handler tries to use it, result is undefined. Compound bug with signal handler safety issues.

**Safe modification:** Use atomic flag to signal shutdown, clean up outside handler.

---

## Dependencies at Risk

### No Memory Bounds (recv buffer)

**Issue:** Buffer is reallocated as needed (server.c:206-216) but reallocations can fail, leaving conn->buffer in an inconsistent state.

**Files:** `src/http/server.c:206-216`

**Impact:** If realloc fails, conn->buffer points to old memory which may be freed. Next read writes to freed memory or fails with SIGSEGV.

**Fix approach:** Validate realloc success before updating pointers. Consider maximum buffer size to prevent unbounded growth (DoS vector).

---

### Config File Parsing Fragility

**Issue:** `parse_config()` reads 512-byte lines. Longer lines are silently truncated.

**Files:** `src/utils/config.c:48`

**Impact:** Long config values (like file paths) silently truncate. Subtle configuration errors.

**Fix approach:** Use dynamic line reading or reject lines longer than buffer. Log warning on truncation.

---

## Scaling Limits

### MAX_CONNECTIONS = 1000 (Hard Limit)

**Issue:** Hardcoded in common.h:39. No graceful handling when limit is reached.

**Files:** `src/common.h:39`, `src/http/server.c:92`

**Impact:** Server rejects legitimate connections with "No free connection slots" when it reaches 1000. No mechanism to increase at runtime or handle > 1000 clients.

**Scaling path:** Make limit configurable via config file or CMake. Consider connection pooling or dynamic allocation.

---

### Fixed Array for Headers (MAX_HEADERS = 50)

**Issue:** Hardcoded in common.h:40.

**Files:** `src/common.h:40`, `src/http/parsers.c:101`

**Impact:** Requests with > 50 headers are rejected. No error message. Parser silently stops.

**Scaling path:** Allocate header array dynamically.

---

### MAX_BACKENDS = 16 (Hard Limit)

**Issue:** Hardcoded in common.h:41.

**Files:** `src/common.h:41`

**Impact:** Only 16 backend servers can be configured.

**Scaling path:** Dynamically allocate backend array from config.

---

## Summary — Priority Order

| Severity | Category | Issue | Files |
|----------|----------|-------|-------|
| **Critical** | Security | Path traversal in static file serving | `src/http/server.c` |
| **Critical** | Security | Proxy request/response buffer overflow | `src/http/server.c` |
| **Critical** | Bug | Use-after-free from parser dangling pointers | `src/http/parsers.c` |
| **Critical** | Bug | Signal handler async-unsafe calls | `src/main.c` |
| **High** | Bug | Shadow variable in main | `src/main.c:33` |
| **High** | Bug | Tautology in state check | `src/http/server.c:222` |
| **High** | Bug | Double close of socket | `src/http/server.c` |
| **High** | Testing | Zero meaningful unit tests | `tests/` |
| **Medium** | Protocol | Missing Content-Length/Content-Type headers | `src/http/response.c` |
| **Medium** | Protocol | Request body parsing disabled | `src/http/parsers.c` |
| **Medium** | Architecture | Monolithic launch() function | `src/http/server.c` |
| **Medium** | Architecture | Over-inclusion via common.h | `src/common.h` |
| **Low** | Quality | Debug printf statements | `src/http/parsers.c`, `src/http/server.c` |
| **Low** | Quality | Hardcoded personal path | `src/common.h:44` |
| **Low** | Quality | Type mismatch (int vs size_t) | `src/http/response.h` |

---

*Concerns audit: 2026-03-17*
