# CServe - Project Analysis Report

**Date:** 2026-03-06
**Scope:** Source code audit — structure, implementation correctness, invariant violations, and comparison to production server practices.

---

## 1. Project Overview

CServe is a single-threaded HTTP/1.1 server written in C targeting Linux, using epoll for I/O multiplexing. Its stated goals are learning nginx internals and applying software design best practices. It supports static file serving and basic reverse proxying.

**Source tree:**
```
src/
  main.c
  common.h
  sock/server.{c,h}       — TCP socket abstraction
  http/server.{c,h}       — epoll event loop, connection lifecycle, routing
  http/parsers.{c,h}      — HTTP request line and header parser
  http/request.{c,h}      — HTTPRequest struct management
  http/response.{c,h}     — HTTPResponse struct management
  utils/config.{c,h}      — INI config file parser
  utils/logger.{c,h}      — structured logging macro
tests/
  test_sqrt.c             — placeholder test (broken)
build.zig                 — Zig build script (broken)
```

---

## 2. Architecture

### 2.1 What works well

- **Separation of concerns in principle.** Networking is separated into `sock/`, HTTP semantics into `http/`, utilities into `utils/`. This is the right direction.
- **epoll-based event loop.** Correct choice for a single-process server; avoids a thread-per-connection model.
- **Constructor/destructor pattern.** Both `SocketServer` and `HTTPServer` use explicit constructor/destructor pairs, which is idiomatic for lifecycle management in C.
- **Zero-copy parser design (intended).** The parser sets pointer/length pairs into the request buffer rather than copying strings — a pattern used by nginx itself.
- **clang-format configured.** Enforces consistent style.
- **`SIGPIPE` ignored.** Correct — prevents process death on broken pipe writes.

### 2.2 Structural weaknesses

| Issue | Location | Severity |
|-------|----------|----------|
| God function `launch()` (~350 lines) handles accept, read, parse, route, send | `http/server.c:11` | High |
| Routing hard-coded by URI prefix inside server loop | `http/server.c:346` | High |
| `common.h` is a global mega-include pulling in 15+ system headers | `src/common.h` | Medium |
| Hardcoded personal path `/home/voidp/...` in a shared header | `src/common.h:44` | High |
| No abstraction layer for backends (hardcoded `"localhost:8002"`) | `http/server.c:452` | High |
| `test_sqrt.c` has no `main()` and `assert(2 == 3)` always fails | `tests/test_sqrt.c` | Medium |

---

## 3. Bugs and Invariant Violations

### 3.1 Critical — Memory Safety

#### Dangling pointer after buffer realloc (use-after-free)
**File:** `http/server.c:207-218`, `http/parsers.c:26-41`

The parser stores raw pointers into `conn->buffer`:
```c
req_t->request_line.method = (char *)ptr;  // points into conn->buffer
```
When `conn->buffer` is reallocated to grow (`realloc(conn->buffer, new_size)`), all previously stored pointers into the old allocation become dangling. Any subsequent access to `req->request_line.method`, `req->headers[i].name`, etc. is undefined behavior.

**Industry standard:** nginx uses a memory pool (`ngx_pool_t`) per request; buffer pointers are stable within a pool's lifetime. The fix is either to copy values at parse time, or to guarantee no realloc after parsing starts.

---

#### Double-close of client file descriptor
**File:** `http/server.c:530-548` (`free_connection`), `http/server.c:332-334`

`free_connection()` closes `conn->socket`:
```c
close(conn->socket);
conn->socket = 0;
```
The caller then immediately closes again:
```c
free_connection(conn, client_fd, self->epoll_fd);
epoll_ctl(self->epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
close(client_fd);   // <-- double close, fd may have been reused
```
Between the first and second `close()`, the OS may have assigned the same fd number to a newly accepted connection. The second `close()` would silently destroy that new connection.

---

#### `malloc` result used before NULL check
**File:** `http/server.c:381-384`

```c
char *buffer = malloc(filesize);
memset(buffer, 0, filesize);   // <-- dereferences before NULL check
if (!buffer) { ... }
```
The `memset` runs before the NULL guard. On OOM, this is a null pointer dereference.

---

#### Path traversal vulnerability (security)
**File:** `http/server.c:353-364`

```c
snprintf(filepath, sizeof(filepath), "%s%.*s", realpath(BASE_DIR, NULL),
         (int)request_ptr->request_line.uri_len, request_ptr->request_line.uri);
int fd = open(filepath, O_RDONLY);
```
A request for `/static/../../../etc/passwd` will resolve to an arbitrary path on the filesystem. There is no validation that the resolved path is within `static_dir`. nginx mitigates this with `ngx_conf_set_path_slot` and path normalization before dispatch.

---

#### `realpath()` memory leak
**File:** `http/server.c:356`, `utils/config.c:38`

`realpath(BASE_DIR, NULL)` allocates memory that must be freed. The returned pointer is used inline and immediately discarded — leaked on every request.

---

### 3.2 Critical — Logic Errors

#### Shadow variable — global `httpserver_ptr` is never set
**File:** `src/main.c:17,33`

```c
HTTPServer *httpserver_ptr = NULL;   // line 17: global

int main(void) {
    ...
    HTTPServer *httpserver_ptr =      // line 33: local variable, shadows global
        httpserver_constructor(...);
```
The local declaration shadows the global. The global remains `NULL` for the entire program lifetime. The signal handler `handle_signal()` checks the global and therefore never frees the server on SIGINT/SIGTERM.

---

#### Connection state condition is a tautology — always true
**File:** `http/server.c:222`

```c
if (conn->state != CONN_CLOSING || conn->state != CONN_ERROR)
```
For any value of `conn->state`, either it is not `CONN_CLOSING` or it is not `CONN_ERROR`. This condition is always `true`. The intended logic is `&&`:
```c
if (conn->state != CONN_CLOSING && conn->state != CONN_ERROR)
```
This means error and closing connections are always re-entered for processing — a request is always parsed and a response sent even on a broken/closing connection.

---

#### `reset_connection` does not reset `buffer_len` or `curr_request`
**File:** `http/server.c:551-557`

```c
int reset_connection(Connection *conn) {
    memset(conn->buffer, 0, conn->buffer_size);
    conn->state = CONN_ESTABLISHED;
    return OK;
}
```
After keep-alive reset, `conn->buffer_len` retains the old value and `conn->curr_request` is not freed or cleared. The next request will overwrite a stale length and the parser will be initialized on a non-NULL `curr_request`.

---

#### Wrong body length in proxy response
**File:** `http/server.c:488`

```c
HTTPResponse *response =
    response_builder(200, "OK", body, proxy_request_len, "text/html");
```
`proxy_request_len` is the length of the *outgoing* request to the backend, not the length of the received body. The response content length will be wrong.

---

#### `socket() == 0` misidentifies error
**File:** `src/sock/server.c:31`

```c
if (server_ptr->socket == 0)
{
    perror("Failed to connect socket...");
    exit(1);
}
```
`socket()` returns `-1` on failure, not `0`. File descriptor `0` is valid (stdin). This check will pass on error and fail spuriously if stdin happens to be closed.

---

#### `dead code` — `full_path` built but `filename` opened
**File:** `utils/config.c:37-39`

```c
char full_path[PATH_MAX];
snprintf(full_path, sizeof(full_path), "%s/%s", realpath(BASE_DIR, NULL), filename);
FILE *f = fopen(filename, "r");   // uses raw filename, not full_path
```
`full_path` is computed and then immediately discarded. The config is opened relative to the CWD, not `BASE_DIR`.

---

#### `strncat` buffer overflow in proxy request
**File:** `http/server.c:450`

```c
char proxy_request[INITIAL_BUFFER_SIZE];   // fixed 4096 bytes
int proxy_request_len = snprintf(proxy_request, ...);  // fills with headers
strncat(proxy_request, request_ptr->body, request_ptr->body_len);
```
`strncat`'s third argument is the max number of bytes to append, but it does not account for bytes already in the buffer. If headers consumed 3000 bytes and the body is 2000 bytes, the strncat silently overflows `proxy_request`.

---

### 3.3 High — Protocol Correctness

#### HTTP response missing Content-Type and Content-Length headers
**File:** `http/response.c:67-105`, `http/server.c:419-423`

`response_builder()` sets `res->content_type` and `res->body_length` as struct fields, but `httpresponse_serialize()` only writes from `res->headers[]`, which is never populated by `response_builder()`. Every response is sent without `Content-Type` or `Content-Length` headers, violating RFC 7230. Browsers and HTTP clients may fail to parse or display responses correctly.

---

#### Body parsing is entirely commented out
**File:** `http/parsers.c:112-137`

All body parsing code is commented out. `req->body` and `req->body_len` are never set. POST/PUT requests will always appear bodyless to the application.

---

#### `send()` is called in blocking mode on what may be a non-blocking fd
**File:** `http/server.c:277-289`

The client socket is set non-blocking after `init_connection`. The send loop does not handle `EAGAIN`/`EWOULDBLOCK` — if the kernel send buffer is full, `send()` returns -1 and the response is silently dropped.

---

#### Keep-alive detection uses non-null-terminated comparison
**File:** `http/server.c:302-305`

```c
strncmp(conn->curr_request->headers[j].value, "keep-alive",
        conn->curr_request->headers[j].value_len)
```
`strncmp` with the field's `value_len` is correct, but HTTP/1.1 specifies that `Connection: keep-alive` is the default. HTTP/1.0 requires it explicitly; the code does the opposite, closing all connections by default. Also, comparison should be case-insensitive (`Connection: Keep-Alive` is common).

---

### 3.4 Medium — Resource Management

#### SIGSEGV handler calls non-async-signal-safe functions
**File:** `src/main.c:55-81`

`httpserver_destructor()` calls `free()`, `close()`, and `free()` recursively. POSIX defines a small set of async-signal-safe functions; `free()` and `malloc()`-family are not among them. Calling them from a signal handler (especially SIGSEGV, which indicates heap/stack corruption) is undefined behavior and can deadlock.

**Standard approach:** Set an `atomic_flag` in the signal handler; the main loop checks it and exits cleanly.

---

#### `accept()` failure closes an invalid fd
**File:** `http/server.c:74-81`

```c
int client_fd = accept(...);
if (client_fd == -1) {
    LOG("ERROR", "Failed to accept a new connection.");
    close(client_fd);   // closes -1, which is a no-op but signals intent confusion
    continue;
}
```
`close(-1)` is harmless but reveals a logic error — `client_fd` is -1 and closing it is meaningless.

---

#### `epoll_ctl DEL` on already-closed fd
**File:** `http/server.c:332-334`

```c
free_connection(conn, client_fd, self->epoll_fd);  // closes conn->socket
epoll_ctl(self->epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);  // fd already closed
close(client_fd);  // second close
```
On Linux, closing an fd automatically removes it from epoll, so the explicit `EPOLL_CTL_DEL` operates on a closed fd. If the fd number has been reused by `accept()`, this silently removes the new connection from the epoll set.

---

### 3.5 Low — Code Quality

| Issue | Location |
|-------|----------|
| `printf` debug output left in production paths | `http/server.c:151-153`, `http/parsers.c:94`, `http/server.c:230` |
| `parsers.h` has no include guard (`#ifndef`) | `src/http/parsers.h` |
| `logger.h` has no include guard | `src/utils/logger.h` |
| `init_config()` in main.c is an empty function that is never called | `src/main.c:83` |
| `str(x)` / `xstr(x)` macros defined but never used | `src/common.h:34-35` |
| `TransportType` enum (TCP/UDP) defined but never used | `src/sock/server.h:21-24` |
| `get_in_addr()` defined but never called | `src/http/server.c:588` |
| `void *get_in_addr` uses IPv6 codepath but `client_addr` is `sockaddr_in` (IPv4 only) | `src/http/server.c:70` |
| Logging to stdout — errors should go to stderr | `src/utils/logger.c:11` |
| `create_http_request()` redundantly initializes fields already zero'd by `calloc` | `src/http/request.c:14-29` |
| `body_length` is `int` but used to index memory — should be `size_t` | `src/http/response.h:23` |
| `header_count` in `HTTPResponse` is `int`; negative count is possible | `src/http/response.h:22` |

---

## 4. Comparison to nginx Methodology

| Dimension | CServe | nginx approach |
|-----------|--------|----------------|
| Memory management | Ad-hoc malloc/free per request | Pool allocator per request, freed atomically at end |
| Parser | Pointer into connection buffer (dangling on realloc) | Pointer into stable pool allocation |
| Routing | Hardcoded prefix matching in server loop | Configurable `location` blocks, compiled trie |
| Configuration | Simple INI with hardcoded defaults | Rich config DSL, validated at startup |
| Error handling | Inconsistent (exit, return, continue, ignore) | Layered error codes propagated up the call stack |
| Logging | stdout only, no log levels at runtime | Configurable log level, error log separate from access log |
| Signal handling | Non-AS-safe operations in handler | Only sets a flag in handler; main loop drains and exits |
| Testing | Broken placeholder | Comprehensive unit/integration tests |
| Build | Broken Zig script | autoconf + make (historically), now CMake-friendly |

---

## 5. Build System — Zig vs CMake

### Zig `build.zig` (current state: broken)

**Problems identified:**
1. `addGlob()` iterates with `while (it.next())` — the Zig iterator returns an error union; this won't compile with current Zig (0.13+).
2. `.path = path` inside `addCSourceFile`/`addIncludePath` is deprecated API (was changed in Zig 0.12). Current API requires `b.path(path)`.
3. References `src/connection/*.c` — this directory does not exist.
4. `exe.run()` must be assigned to a variable before calling `.step` on it.
5. Zig's C build API has broken backward compatibility multiple times and is still considered unstable.

**Verdict:** The Zig build script cannot compile the project in its current state and requires non-trivial fixes that would need revisiting after every Zig release.

### CMake (deleted, but the right choice)

CMake is the industry standard for C projects and offers:
- Stable, mature API with long-term backward compatibility
- First-class IDE support (VSCode `cmake-tools`, CLion, Qt Creator)
- `CTest` for test integration
- `find_package` / `FetchContent` for dependency management
- Easy Doxygen integration (`Doxyfile.in` already present in repo)
- Straightforward CI/CD (GitHub Actions, GitLab CI have official CMake support)
- Install targets, package config export

**Conclusion:** CMake is the correct tool for this project. The Zig build was an interesting experiment but is not viable for a C project today.
