# CServe — Grand Refactoring Plan

**Companion to:** [REPORT.md](REPORT.md)

This document describes the full restructuring of CServe: build system migration, bug fixes, security patches, architectural changes, and the order of operations. Items are grouped by phase so that each phase produces a buildable, testable artifact.

---

## Build System Decision: CMake (drop Zig)

**Decision: Restore CMake, delete `build.zig`.**

The Zig build script cannot compile the project today (see REPORT §5 for specifics). Zig's C-build API is unstable and has broken backward compatibility across minor versions. For a C project, CMake is the professional standard. It provides:

- Stable API, maintained since 2000
- CTest for test integration
- `find_package` / `FetchContent` for dependencies (libcheck, etc.)
- First-class VSCode and CLion support
- Clean install/package targets
- Existing `Doxyfile.in` in the repo integrates naturally

**Files to remove:** `build.zig`, `.zig-cache/`

---

## Phase 0 — Build System (prerequisite for everything else)

### 0.1 Restore CMakeLists.txt

Root `CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.20)
project(cserve C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

add_subdirectory(src)
add_subdirectory(tests)

find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(DOXYGEN_FOUND)
    configure_file(Doxyfile.in Doxyfile @ONLY)
    add_custom_target(docs doxygen ${CMAKE_BINARY_DIR}/Doxyfile)
endif()
```

`src/CMakeLists.txt`:
```cmake
add_library(cserve_core STATIC
    sock/server.c
    http/request.c
    http/response.c
    http/parsers.c
    http/router.c         # new
    http/server.c
    utils/config.c
    utils/logger.c
)
target_include_directories(cserve_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(cserve main.c)
target_link_libraries(cserve PRIVATE cserve_core)
```

`tests/CMakeLists.txt`:
```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(CHECK REQUIRED check)

file(GLOB TEST_SOURCES "*.c")
add_executable(test_runner ${TEST_SOURCES})
target_include_directories(test_runner PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_runner PRIVATE cserve_core ${CHECK_LIBRARIES})

add_test(NAME unit_tests COMMAND test_runner)
enable_testing()
```

Build commands (replacing all Zig commands):
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## Phase 1 — Critical Bug Fixes (safety and correctness)

### 1.1 Fix shadow variable in `main.c`

**Problem:** Local `HTTPServer *httpserver_ptr` at line 33 shadows global at line 17.
**Fix:** Remove the type qualifier from the assignment.

```c
// Before
HTTPServer *httpserver_ptr = httpserver_constructor(...);

// After
httpserver_ptr = httpserver_constructor(...);
```

### 1.2 Fix connection state tautology

**Problem:** `http/server.c:222` — condition is always true.

```c
// Before (always true — tautology)
if (conn->state != CONN_CLOSING || conn->state != CONN_ERROR)

// After (correct)
if (conn->state != CONN_CLOSING && conn->state != CONN_ERROR)
```

### 1.3 Fix double-close of client file descriptor

**Problem:** `free_connection()` closes `conn->socket`; caller closes `client_fd` again.
**Fix:** `free_connection()` should NOT close the socket — the caller owns the fd lifetime.

```c
// free_connection: remove the close() call
int free_connection(Connection *conn) {
    // do NOT close conn->socket here — caller is responsible
    free(conn->buffer);
    conn->buffer      = NULL;
    conn->buffer_size = 0;
    conn->socket      = -1;   // sentinel
    return 0;
}
```

Caller becomes the single close site:
```c
epoll_ctl(self->epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
close(client_fd);
free_connection(conn);
self->active_count--;
```

### 1.4 Fix `socket()` error check

**Problem:** `sock/server.c` checks `socket == 0` instead of `< 0`.

```c
// Before
if (server_ptr->socket == 0) { ... }

// After
if (server_ptr->socket < 0) { ... }
```

### 1.5 Fix NULL check order for `malloc` in file serving

**Problem:** `memset` called before NULL check.

```c
// Before
char *buffer = malloc(filesize);
memset(buffer, 0, filesize);
if (!buffer) { ... }

// After
char *buffer = calloc(1, filesize);  // calloc zeroes memory, no memset needed
if (!buffer) { ... }
```

### 1.6 Fix `accept()` failure handler

```c
// Before
close(client_fd);   // client_fd == -1, meaningless

// After (just continue, no close)
LOG("ERROR", "Failed to accept connection: %s", strerror(errno));
continue;
```

### 1.7 Fix `reset_connection`

```c
int reset_connection(Connection *conn) {
    memset(conn->buffer, 0, conn->buffer_size);
    conn->buffer_len   = 0;       // add this
    conn->state        = CONN_ESTABLISHED;
    if (conn->curr_request) {
        free_http_request(conn->curr_request);
        conn->curr_request = NULL;  // add this
    }
    return 0;
}
```

### 1.8 Fix `realpath()` memory leaks

Every `realpath(BASE_DIR, NULL)` call leaks the returned allocation.

```c
// Before (leaks)
snprintf(filepath, sizeof(filepath), "%s%.*s", realpath(BASE_DIR, NULL), ...);

// After
char *base = realpath(BASE_DIR, NULL);
if (!base) { /* error */ }
snprintf(filepath, sizeof(filepath), "%s%.*s", base, ...);
free(base);
```

---

## Phase 2 — Security Fixes

### 2.1 Path traversal mitigation

**Problem:** URI is concatenated directly onto BASE_DIR without canonicalization.

After building `filepath`, validate it starts with the resolved `static_dir`:

```c
char *resolved = realpath(filepath, NULL);
if (!resolved) {
    /* file not found or bad path */
    return error_response(404, "Not Found");
}

char *root = realpath(static_dir, NULL);
if (!root || strncmp(resolved, root, strlen(root)) != 0) {
    free(resolved);
    free(root);
    return error_response(403, "Forbidden");
}
free(root);
/* proceed with resolved path */
```

### 2.2 Fix proxy request buffer overflow

Replace fixed-size stack buffer with dynamic allocation:

```c
// Instead of:
char proxy_request[INITIAL_BUFFER_SIZE];
strncat(proxy_request, body, body_len);  // overflow-prone

// Use:
size_t header_part_len = /* snprintf result */;
size_t total_len       = header_part_len + request_ptr->body_len;
char  *proxy_request   = malloc(total_len + 1);
if (!proxy_request) { /* OOM error */ }
memcpy(proxy_request + header_part_len, request_ptr->body, request_ptr->body_len);
proxy_request[total_len] = '\0';
```

### 2.3 Fix proxy response buffer overflow

Replace fixed-size stack buffer:
```c
// Replace:
char proxy_response[INITIAL_BUFFER_SIZE];
recv(backend_fd, proxy_response, sizeof(proxy_response), 0);

// With dynamic read loop, same pattern as client recv loop
```

### 2.4 Fix signal handler

Replace all allocator calls with an atomic flag:

```c
// In header or main.c
#include <stdatomic.h>
static volatile atomic_int shutdown_requested = 0;

void handle_signal(int sig) {
    // Only async-signal-safe operations:
    const char *msg = "[!] Signal received, shutting down...\n";
    write(STDERR_FILENO, msg, strlen(msg));   // write() is AS-safe
    atomic_store(&shutdown_requested, 1);
}
```

Main event loop checks the flag:
```c
while (!atomic_load(&shutdown_requested)) {
    int n_ready = epoll_wait(...);
    ...
}
// cleanup after loop
```

---

## Phase 3 — Protocol Correctness

### 3.1 Add Content-Type and Content-Length response headers

`response_builder()` must call `httpresponse_add_header()`:

```c
HTTPResponse *response_builder(int status_code, const char *phrase,
                                const char *body, size_t body_length,
                                const char *content_type) {
    HTTPResponse *res = httpresponse_constructor();
    if (!res) return NULL;

    res->status_code   = status_code;
    res->version       = strdup("HTTP/1.1");
    res->reason_phrase = strdup(phrase);
    res->body          = malloc(body_length);
    memcpy(res->body, body, body_length);
    res->body_length   = body_length;

    char content_length_str[32];
    snprintf(content_length_str, sizeof(content_length_str), "%zu", body_length);
    httpresponse_add_header(res, "Content-Length", content_length_str);
    httpresponse_add_header(res, "Content-Type", content_type);
    httpresponse_add_header(res, "Server", "cserve/0.1");

    return res;
}
```

### 3.2 Restore body parsing

Uncomment and fix the body parsing section in `parsers.c`. The `Content-Length` header must be found during header parsing and used to read `body_len` bytes. Use `atol()` (or `strtol()` with validation) rather than `atoi()` for `Content-Length`, as large bodies overflow `int`.

### 3.3 Fix EAGAIN handling in send loop

```c
while (total_sent < response_len) {
    ssize_t bytes_sent = send(client_fd, response_str + total_sent,
                              response_len - total_sent, MSG_NOSIGNAL);
    if (bytes_sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // register EPOLLOUT and defer; for now just close
            break;
        }
        LOG("ERROR", "send() failed: %s", strerror(errno));
        break;
    }
    total_sent += (size_t)bytes_sent;
}
```

### 3.4 Fix proxy response body length

```c
// Before (wrong length)
response_builder(200, "OK", body, proxy_request_len, "text/html");

// After
size_t body_len = (size_t)(proxy_response + proxy_response_len - body);
response_builder(200, "OK", body, body_len, "text/html");
```

### 3.5 Case-insensitive keep-alive detection

Use `strncasecmp()` instead of `strncmp()` for both header name and value comparison.

---

## Phase 4 — Architectural Restructuring

### 4.1 Eliminate the god function `launch()`

Decompose into distinct functions:

```
launch()
  |-- accept_new_connection(server)    -> Connection *
  |-- handle_readable(server, conn)
  |     |-- recv_into_buffer(conn)     -> RecvResult
  |     |-- process_connection(conn)
  |           |-- parse_request(conn)  -> ParseResult
  |           |-- dispatch_request(conn, router)
  |           |-- send_response(conn, response)
  |-- close_connection(server, conn)
```

Each function fits on one screen, has a single responsibility, and can be unit-tested independently.

### 4.2 Introduce a Router abstraction

Remove hardcoded URI prefix matching from the server. Introduce:

```c
// src/http/router.h
typedef HTTPResponse *(*HandlerFunc)(HTTPRequest *req, void *ctx);

typedef struct Route {
    const char *prefix;
    HandlerFunc handler;
    void *ctx;
} Route;

typedef struct Router {
    Route  *routes;
    size_t  count;
} Router;

Router *router_create(void);
void    router_add(Router *r, const char *prefix, HandlerFunc fn, void *ctx);
HTTPResponse *router_dispatch(Router *r, HTTPRequest *req);
void    router_free(Router *r);
```

Built-in handlers registered at startup:
```c
router_add(router, "/static", static_file_handler, &cfg);
router_add(router, "/api",    proxy_handler,        &cfg);
```

This enables: configurable routes, middleware chains, and testable handlers.

### 4.3 Fix parser's dangling pointer problem

Two options:

**Option A (preferred for now):** Copy field values at parse time.
```c
req->request_line.method = strndup(ptr, method_len);
```
Strings are now stable regardless of buffer reallocation. Free them in `free_http_request()`.

**Option B (nginx approach, future):** Implement a simple pool allocator per request that the buffer never outlives. More complex but eliminates individual frees.

### 4.4 Decouple `common.h`

`common.h` currently includes 15+ system headers and is included everywhere. This creates implicit dependencies and breaks modularity.

**Rule:** Each `.h` file includes only the headers it directly uses. Remove `common.h` as a catch-all include.

- Move connection and HTTP constants to `http/server.h`
- Move error codes to a dedicated `error.h`
- Move HTTP method/state enums to `http/types.h`
- `logger.h` and platform types only go in files that use them

### 4.5 Add include guards everywhere

`parsers.h` and `logger.h` are missing `#ifndef` guards. Add them:

```c
#ifndef CSERVE_PARSERS_H
#define CSERVE_PARSERS_H
...
#endif /* CSERVE_PARSERS_H */
```

### 4.6 Remove personal hardcoded path

```c
// Delete from common.h:
#define DEFAULT_CONFIG_PATH "/home/voidp/Projects/samandar/1lang1server/cserver"
```

Replace with a compile-time default set by CMake or discovered at runtime.

### 4.7 Use `-1` sentinel for "no socket"

Replace the `socket == 0` convention (incorrect, fd 0 is valid) with `-1`:

```c
// Initialization
conn->socket = -1;

// Free slot search
if (self->connections[j].socket == -1) { ... }
```

### 4.8 Route error and info logs correctly

```c
// In logger.c:
if (strcmp(level, "ERROR") == 0)
    vfprintf(stderr, fmt, args);
else
    vfprintf(stdout, fmt, args);
```

---

## Phase 5 — Testing Infrastructure

### 5.1 Replace `test_sqrt.c`

`tests/test_sqrt.c` is a broken placeholder (`assert(2 == 3)`, no `main()`). Replace with real tests.

Initial test targets:
- `test_parsers.c` — unit tests for `parse_request_line()`, `parse_header()`, `parse_http_request()` against raw HTTP strings
- `test_config.c` — tests for `parse_config()` with valid/invalid/edge-case INI files
- `test_response.c` — tests for `response_builder()`, `httpresponse_serialize()`, header emission
- `test_router.c` — tests for route dispatch, 404 fallback

Use `libcheck` (already referenced in the Zig build) via CMake `pkg_check_modules(CHECK REQUIRED check)`.

### 5.2 Add integration test

A shell script or Python script that starts the server, sends HTTP requests with `curl`, and validates responses. Run via CTest `add_test(NAME integration COMMAND ${CMAKE_SOURCE_DIR}/tests/integration.sh)`.

---

## Phase 6 — Code Quality

### 6.1 Remove debug `printf` calls

All `printf(...)` in `http/server.c` and `http/parsers.c` that are not behind a log level macro must be removed or converted to `LOG("DEBUG", ...)`.

### 6.2 Remove dead code

- `init_config()` in `main.c` (empty, never called)
- `str(x)` / `xstr(x)` macros (defined, never used)
- `TransportType` enum in `sock/server.h` (never used)
- `get_in_addr()` in `http/server.c` (never called)

### 6.3 Fix `body_length` type

`HTTPResponse.body_length` is `int` but holds a size. Change to `size_t` to match `body` length semantics and eliminate sign-comparison warnings.

### 6.4 Use `strtol()` for integer config parsing

Replace `atoi()` in config parsing (which silently returns 0 on invalid input) with `strtol()` with error checking.

---

## Summary — Priority Order

| Priority | Phase | Key deliverable |
|----------|-------|-----------------|
| 1 | 0 — Build | CMake builds cleanly, `build.zig` removed |
| 2 | 1 — Critical bugs | No double-free, no shadow variable, no tautology |
| 3 | 2 — Security | No path traversal, no buffer overflow |
| 4 | 3 — Protocol | Valid HTTP responses, body parsing works |
| 5 | 4 — Architecture | Router, decomposed launch(), stable parser pointers |
| 6 | 5 — Testing | Passing unit tests committed with each fix |
| 7 | 6 — Quality | No debug prints, no dead code, clean compiler warnings |

Each phase should be committed independently so that progress is bisectable and reviewable.
