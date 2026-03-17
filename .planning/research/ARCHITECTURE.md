# Architecture Patterns

**Domain:** C-based HTTP/1.1 web server and reverse proxy
**Researched:** 2026-03-17
**Confidence:** HIGH (based on well-documented nginx/lighttpd/haproxy architectures)

## How Professional C Web Servers Are Structured

### nginx Architecture (Primary Reference)

nginx separates concerns into distinct subsystems:

1. **Core** (`src/core/`) -- Memory pools, string utilities, linked lists, hash tables, configuration framework, cycle (process lifecycle). The core provides data structures and lifecycle management that everything else builds on.

2. **Event subsystem** (`src/event/`) -- Abstract event interface with platform-specific backends (epoll, kqueue, poll). The event loop is isolated from protocol logic. `ngx_event_t` is the fundamental unit -- a callback attached to a file descriptor. The event module never parses HTTP; it just delivers "readable" or "writable" notifications.

3. **HTTP subsystem** (`src/http/`) -- Request parsing, header processing, URI handling, upstream (proxy), static file serving. Each "phase" of request processing is a separate handler in a pipeline. The HTTP subsystem is a *consumer* of events, not intertwined with them.

4. **Module system** -- nginx's defining pattern. Every feature (gzip, SSL, proxy, static files, logging) is a module with a standard interface: `init_module`, `init_process`, handler callbacks. Modules register into processing phases (post-read, rewrite, access, content, log). Configuration directives map directly to module callbacks.

5. **Connection abstraction** -- `ngx_connection_t` owns the socket fd, read/write event pointers, send/recv function pointers (swappable for TLS), and per-connection memory pool. The connection does not know about HTTP -- it is protocol-agnostic.

6. **Upstream** (reverse proxy) -- Completely separate from the static file handler. Has its own connection pool to backends, its own buffer chain management, and its own state machine for forwarding.

**Key takeaway:** nginx achieves testability and extensibility by making the event loop protocol-agnostic and making every feature a pluggable module with a uniform interface.

### lighttpd Architecture (Secondary Reference)

lighttpd uses a simpler but equally clean separation:

1. **Server core** (`server.c`) -- Process lifecycle, main loop, worker management. The core *drives* the event loop but delegates all protocol work.

2. **Connections** (`connections.c`) -- Connection lifecycle independent of protocol specifics. State machine: accept, read, parse, handle, write, close.

3. **Request/Response** (`request.c`, `response.c`) -- HTTP parsing and response construction as pure functions operating on connection buffers.

4. **Handlers** (`mod_*.c`) -- Each handler is a loadable module: `mod_staticfile`, `mod_proxy`, `mod_accesslog`. Handlers register for URI patterns and get called during the content phase.

5. **Chunk/buffer system** (`chunk.c`, `buffer.c`) -- Generic I/O buffer abstraction used everywhere. `sendfile()` integration for zero-copy static file serving.

**Key takeaway:** lighttpd proves you do not need nginx's complexity. A clean connection state machine + handler dispatch + buffer abstraction is sufficient for a professional server.

### haproxy Architecture (Tertiary Reference, proxy-focused)

haproxy is relevant for the reverse proxy dimension:

1. **Stream abstraction** -- Every proxied request is a "stream" with a client-side connection and a server-side connection. The stream owns both sides and manages the relay.

2. **Connection pooling** -- Backend connections are pooled and reused across requests. Idle connections are kept alive with configurable timeouts.

3. **Health checking** -- Independent health check connections run on a timer, not triggered by client requests. Backend state (up/down/draining) is maintained separately.

4. **Buffer relay** -- Data is relayed between client and backend using shared buffer rings. No full-request buffering is needed for simple forwarding.

## Recommended Architecture for cserve

The following architecture takes the best patterns from nginx and lighttpd, scaled down for a single-threaded, zero-dependency C server.

### Component Diagram

```
main.c
  |
  v
+-----------------+
|   core/server   |  Process lifecycle, startup, shutdown, signal handling
+-----------------+
  |
  v
+-----------------+     +------------------+
|  core/event     |---->|  core/connection |  Event loop (epoll) delivers
|  (event loop)   |     |  (conn manager)  |  events to connections
+-----------------+     +------------------+
                              |
                              v
                        +------------------+
                        |  http/parser     |  HTTP/1.1 request parsing
                        +------------------+
                              |
                              v
                        +------------------+
                        |  http/router     |  URI matching, handler dispatch
                        +------------------+
                           /        \
                          v          v
                   +-----------+  +-----------+
                   | handler/  |  | handler/  |
                   | static    |  | proxy     |  Content handlers
                   +-----------+  +-----------+
                                       |
                                       v
                                  +-----------+
                                  | proxy/    |
                                  | upstream  |  Backend connection pool
                                  +-----------+

Cross-cutting:
  +------------------+  +------------------+  +------------------+
  |  core/buffer     |  |  util/config     |  |  util/log        |
  +------------------+  +------------------+  +------------------+

Future layer (slots in, does not restructure):
  +------------------+
  |  core/tls        |  Wraps connection send/recv with OpenSSL
  +------------------+
```

### Component Boundaries

| Component | Directory | Responsibility | Communicates With |
|-----------|-----------|---------------|-------------------|
| **Server lifecycle** | `src/core/server.c` | Startup, signal handling, shutdown, main loop orchestration | Event loop, config |
| **Event loop** | `src/core/event.c` | epoll_wait, fd registration, event dispatch | Connection manager |
| **Connection manager** | `src/core/connection.c` | Connection pool, state machine, fd-to-connection mapping | Event loop, HTTP parser, buffer |
| **Buffer** | `src/core/buffer.c` | Dynamic buffer allocation, growth, reset; used by connections and responses | Connection, parser, handlers |
| **HTTP parser** | `src/http/parser.c` | RFC 7230 request line + header parsing, produces HTTPRequest struct | Connection (input buffer), router |
| **HTTP types** | `src/http/types.h` | HTTPRequest, HTTPResponse, HTTPMethod, HTTPStatus, Header structs | All HTTP-layer code |
| **HTTP response** | `src/http/response.c` | Response construction, header serialization, status line formatting | Handlers, connection (output) |
| **Router** | `src/http/router.c` | URI pattern matching, handler dispatch based on config | Parser (input), handlers (dispatch) |
| **Static file handler** | `src/handler/static.c` | Path resolution, MIME detection, file reading, sendfile | Router, response builder |
| **Proxy handler** | `src/handler/proxy.c` | Request forwarding, response relay, header rewriting | Router, upstream pool |
| **Upstream pool** | `src/proxy/upstream.c` | Backend connection pool, health state, round-robin selection | Proxy handler, event loop |
| **Config** | `src/util/config.c` | INI parsing, virtual host config, backend list, TLS paths | Server lifecycle, router, upstream |
| **Logger** | `src/util/log.c` | Structured logging with levels, timestamps, file/line | Everything |
| **TLS (future)** | `src/core/tls.c` | SSL_CTX management, connection wrapping, handshake | Connection manager |

### Module Interfaces (Clean APIs Between Components)

Each component exposes a narrow, typed API. Internal state is opaque to callers.

**Event loop** (`core/event.h`):
```c
typedef void (*event_handler_fn)(int fd, uint32_t events, void *data);

typedef struct EventLoop EventLoop;

EventLoop *eventloop_create(int max_events);
void       eventloop_destroy(EventLoop *loop);
int        eventloop_add(EventLoop *loop, int fd, uint32_t events, void *data);
int        eventloop_modify(EventLoop *loop, int fd, uint32_t events);
int        eventloop_remove(EventLoop *loop, int fd);
int        eventloop_run(EventLoop *loop, event_handler_fn handler, int timeout_ms);
// Returns number of events dispatched, or -1 on error.
// handler is called once per ready fd.
```

**Connection manager** (`core/connection.h`):
```c
typedef struct Connection Connection;

Connection *connection_pool_init(size_t max_connections);
Connection *connection_accept(Connection *pool, int listen_fd, EventLoop *loop);
void        connection_close(Connection *conn, EventLoop *loop);
void        connection_reset(Connection *conn);  // for keep-alive reuse

// State queries
ConnState   connection_state(const Connection *conn);
Buffer     *connection_read_buf(Connection *conn);
Buffer     *connection_write_buf(Connection *conn);
int         connection_fd(const Connection *conn);
```

**Buffer** (`core/buffer.h`):
```c
typedef struct Buffer Buffer;

Buffer *buffer_create(size_t initial_capacity);
void    buffer_destroy(Buffer *buf);
int     buffer_append(Buffer *buf, const void *data, size_t len);
int     buffer_consume(Buffer *buf, size_t len);  // discard from front
void    buffer_reset(Buffer *buf);
char   *buffer_data(const Buffer *buf);
size_t  buffer_len(const Buffer *buf);
size_t  buffer_remaining(const Buffer *buf);  // capacity - len
```

**HTTP parser** (`http/parser.h`):
```c
typedef enum {
    PARSE_INCOMPLETE,   // need more data
    PARSE_COMPLETE,     // full request parsed
    PARSE_ERROR         // malformed request
} ParseResult;

// Incremental: call repeatedly as data arrives.
// Returns how many bytes were consumed from buf.
ParseResult http_parse_request(Buffer *buf, HTTPRequest *req);

void http_request_reset(HTTPRequest *req);
void http_request_free(HTTPRequest *req);
```

**Router** (`http/router.h`):
```c
typedef HTTPResponse *(*request_handler_fn)(const HTTPRequest *req, void *ctx);

typedef struct Router Router;

Router *router_create(const Config *cfg);
void    router_destroy(Router *router);
// Dispatch returns the response. Caller owns the response.
HTTPResponse *router_dispatch(Router *router, const HTTPRequest *req);
```

**Handler interface** (uniform for static and proxy):
```c
// Both static and proxy handlers conform to request_handler_fn.
// Registration happens in router_create() based on config.

// handler/static.h
HTTPResponse *static_handle(const HTTPRequest *req, void *ctx);

// handler/proxy.h
HTTPResponse *proxy_handle(const HTTPRequest *req, void *ctx);
```

**TLS (future)** (`core/tls.h`):
```c
typedef struct TLSContext TLSContext;

TLSContext *tls_create(const char *cert_path, const char *key_path);
void        tls_destroy(TLSContext *ctx);

// Wrap a connection's I/O. After this, connection_read/write use SSL.
int tls_wrap_connection(TLSContext *ctx, Connection *conn);
// Called from event loop when handshake needs more I/O.
int tls_handshake(Connection *conn);
```

### Data Flow

**Incoming request (complete lifecycle):**

```
1. epoll_wait() returns EPOLLIN on client fd
         |
2. eventloop_run() calls handler with (fd, events, connection_ptr)
         |
3. recv() into connection's read buffer (buffer_append)
         |
4. http_parse_request(read_buf, &conn->request)
   - Returns PARSE_INCOMPLETE? -> re-register EPOLLIN, return
   - Returns PARSE_ERROR? -> send 400, close
   - Returns PARSE_COMPLETE? -> continue
         |
5. router_dispatch(router, &conn->request)
   - Matches URI pattern from config
   - Calls static_handle() or proxy_handle()
         |
6. Handler returns HTTPResponse*
         |
7. http_response_serialize(response, conn->write_buf)
         |
8. send() from write buffer
   - EAGAIN? -> register EPOLLOUT, return (resume on writable)
   - Complete? -> check keep-alive
         |
9. Keep-alive: connection_reset(), register EPOLLIN
   No keep-alive: connection_close()
```

**Reverse proxy data flow:**

```
1. proxy_handle() receives parsed HTTPRequest
         |
2. upstream_acquire(pool) -> get or create backend connection
         |
3. Forward request to backend (rewrite Host, add X-Forwarded-For)
         |
4. recv() backend response into proxy buffer
         |
5. Parse backend response status + headers
         |
6. Construct client HTTPResponse from backend response
         |
7. upstream_release(pool, backend_conn) -> return to pool
         |
8. Return HTTPResponse to caller
```

**State machine (per connection):**

```
ACCEPTED --> READING --> PARSING --> HANDLING --> WRITING --> [READING | CLOSING]
    |           |           |           |           |
    +---------- +---------- +---------- +---------- +--> ERROR --> CLOSING
```

- ACCEPTED: fd accepted, registered with epoll, buffers allocated
- READING: recv() in progress, may need multiple reads
- PARSING: parser running on buffer, may return INCOMPLETE
- HANDLING: router dispatched to handler, handler building response
- WRITING: send() in progress, may need multiple writes (EAGAIN/EPOLLOUT)
- CLOSING: epoll remove, close fd, free buffers, return slot to pool

## Patterns to Follow

### Pattern 1: Opaque Types with Function-Pointer "Methods"

**What:** Expose only `typedef struct Foo Foo;` in headers. All access through functions. Internal struct definition lives in .c file only.

**When:** Every major component (EventLoop, Connection, Buffer, Router).

**Why:** Prevents coupling. Callers cannot reach into struct internals. Changing internal layout does not require recompiling callers.

**Example:**
```c
// core/buffer.h (public)
typedef struct Buffer Buffer;
Buffer *buffer_create(size_t cap);
size_t  buffer_len(const Buffer *buf);

// core/buffer.c (private)
struct Buffer {
    char  *data;
    size_t len;
    size_t cap;
};
```

### Pattern 2: Incremental Parsing (nginx/lighttpd style)

**What:** Parser consumes whatever bytes are available and returns INCOMPLETE/COMPLETE/ERROR. Never blocks waiting for more data. Parser state persists across calls.

**When:** HTTP request parsing. Critical for non-blocking I/O.

**Why:** The event loop delivers data in arbitrary chunks. A `recv()` might give you half a header line. The parser must handle partial input without losing state.

**Example:**
```c
// Parser state lives in HTTPRequest
typedef struct {
    HTTPRequestState state;  // which part we're parsing
    size_t           parsed_offset;  // how far we got last time
    // ... parsed fields ...
} HTTPRequest;

ParseResult http_parse_request(Buffer *buf, HTTPRequest *req) {
    switch (req->state) {
    case REQ_PARSE_LINE:
        // scan for \r\n from parsed_offset
        // if not found, return PARSE_INCOMPLETE
        // if found, parse method/uri/version, advance state
        // fall through
    case REQ_PARSE_HEADER:
        // scan for \r\n\r\n (end of headers)
        // parse each header line
        // if incomplete, return PARSE_INCOMPLETE
        // fall through
    case REQ_PARSE_BODY:
        // use Content-Length to know how much body to expect
        // return INCOMPLETE until we have it all
        req->state = REQ_PARSE_DONE;
        return PARSE_COMPLETE;
    }
}
```

### Pattern 3: Handler Registration via Config (lighttpd style)

**What:** Handlers are plain functions registered with the router at startup, based on configuration. No runtime plugin loading needed for a focused server.

**When:** Mapping URI patterns to static/proxy handlers.

**Why:** Simpler than nginx's full module system. cserve has two handlers (static, proxy). A function pointer table is sufficient.

**Example:**
```c
typedef struct {
    const char        *pattern;    // e.g., "/static/" or "/api/"
    request_handler_fn handler;
    void              *ctx;        // handler-specific config (static_dir, upstream_pool)
} Route;

Router *router_create(const Config *cfg) {
    Router *r = calloc(1, sizeof(Router));
    // From config: add static routes, proxy routes
    router_add_route(r, cfg->static_prefix, static_handle, static_ctx);
    router_add_route(r, cfg->proxy_prefix, proxy_handle, proxy_ctx);
    return r;
}
```

### Pattern 4: Connection-Owned Buffers with Separate Read/Write

**What:** Each connection owns two buffers: one for incoming data (read), one for outgoing data (write). Buffers are reused across keep-alive requests.

**When:** Every connection.

**Why:** Separating read and write buffers allows the server to be reading the next request while still writing the current response (pipelining-ready). The current codebase uses a single buffer, which forces serial read-then-write.

### Pattern 5: sendfile() for Static Content (nginx/lighttpd style)

**What:** Use `sendfile()` syscall to transfer file data directly from disk fd to socket fd, bypassing userspace copying.

**When:** Static file responses where the file is a regular file and the client connection is not TLS.

**Why:** Eliminates one copy (kernel buffer -> userspace -> kernel send buffer). For large static files this is a significant performance win. Falls back to read()+send() for TLS connections (where data must pass through SSL_write).

## Anti-Patterns to Avoid

### Anti-Pattern 1: Monolithic Event Handler (Current State)

**What:** A single function that handles accept, read, parse, dispatch, and send based on complex conditional logic and nested loops.

**Why bad:** Untestable (must run full server to test any component), unmaintainable (635+ lines), fragile (state management bugs hide in deep nesting).

**Instead:** Each event triggers exactly one action based on connection state. The event handler is a dispatcher: "connection in READING state received EPOLLIN -> call `connection_read()`". Each action function is independently testable.

### Anti-Pattern 2: Zero-Copy Parsing with Mutable Buffers

**What:** Storing pointers into the receive buffer instead of copying parsed fields.

**Why bad:** Buffer reallocation invalidates all pointers. This is the root cause of the existing use-after-free bug. The "performance win" of avoiding string copies is negligible compared to the correctness cost. nginx copies parsed fields; lighttpd copies parsed fields.

**Instead:** `strndup()` parsed fields into the HTTPRequest struct. Free them in `http_request_free()`. The overhead of copying a few short strings (method: 3-7 bytes, URI: ~100 bytes, headers: ~1KB) is unmeasurable compared to network I/O latency.

### Anti-Pattern 3: Catch-All Header File

**What:** A single `common.h` that includes every system header and is included by every source file.

**Why bad:** Destroys module isolation. Every file depends on every header. Changes rebuild everything. Implicit dependencies hide actual coupling.

**Instead:** Each header includes only what it needs. Create focused type headers:
- `http/types.h` -- HTTPMethod, HTTPStatus, Header, HTTPRequest, HTTPResponse structs
- `core/error.h` -- ErrorCode enum
- `core/connection.h` -- ConnectionState, Connection API

### Anti-Pattern 4: Synchronous Proxy Forwarding

**What:** Blocking connect/send/recv to the backend within the request handler, freezing the event loop.

**Why bad:** While waiting for a slow backend, ALL other clients are stalled. A single slow backend response can freeze 999 other connections.

**Instead:** Non-blocking backend connections registered with the same epoll instance. The proxy handler initiates the connection and returns. When the backend responds, epoll delivers the event and the relay continues. This is how nginx upstream works.

## Scalability Considerations

| Concern | At 100 connections | At 10K connections | At 100K connections |
|---------|-------------------|-------------------|---------------------|
| Connection pool | Fixed array, fine | Dynamic array or arena | Slab allocator, per-connection memory pools |
| Buffer memory | 4KB * 100 = 400KB | 4KB * 10K = 40MB | Need bounded buffers, reject oversized requests |
| epoll performance | O(1) per event, trivial | O(1) per event, fine | O(1) per event, but need edge-triggered mode |
| File descriptors | Default ulimit fine | Raise ulimit to 65535 | Per-process fd limit, may need SO_REUSEPORT |
| Static files | read() into buffer | sendfile() essential | sendfile() + mmap cache for hot files |
| Proxy backends | Direct connect | Connection pool mandatory | Pool + health checks + circuit breaker |

**For cserve v1 target:** Optimize for 10K connections. This means:
- Dynamic connection pool (not fixed 1000)
- sendfile() for static files
- Non-blocking proxy with connection pooling
- Configurable buffer limits to prevent memory exhaustion

## Suggested Build Order (Dependencies Between Components)

The components have a natural dependency order that dictates build sequence:

```
Phase 1: Foundation (no HTTP knowledge)
  core/buffer.c    -- no dependencies, pure data structure
  util/log.c       -- no dependencies, standalone
  core/error.h     -- just enums, no code

Phase 2: Event Infrastructure
  core/event.c     -- depends on: buffer (for testing)
  core/connection.c -- depends on: buffer, event

Phase 3: HTTP Protocol
  http/types.h     -- just structs and enums
  http/parser.c    -- depends on: buffer, types
  http/response.c  -- depends on: buffer, types

Phase 4: Request Handling
  http/router.c    -- depends on: types, parser
  handler/static.c -- depends on: router, response, buffer
  handler/proxy.c  -- depends on: router, response, upstream

Phase 5: Proxy Infrastructure
  proxy/upstream.c -- depends on: event, connection, buffer

Phase 6: Server Assembly
  core/server.c    -- depends on: event, connection, router, config
  main.c           -- depends on: server, config

Phase 7: TLS
  core/tls.c       -- depends on: connection (wraps I/O)
```

**Critical insight:** Phases 1-3 can be built and tested WITHOUT a running server. Parser tests feed buffers directly. Response tests check serialization output. This is the primary architectural benefit of clean separation -- testability from day one.

## Mapping Current Code to Target Architecture

| Current File | Target Component(s) | Migration Notes |
|-------------|---------------------|-----------------|
| `src/common.h` | Split into `http/types.h`, `core/error.h`, constants into respective headers | Biggest refactor -- touch everything |
| `src/http/server.c` | Split into `core/event.c`, `core/connection.c`, `core/server.c`, `http/router.c` | The monolith decomposition |
| `src/http/server.h` | Split into `core/connection.h`, `core/server.h` | Connection and HTTPServer separated |
| `src/http/parsers.c` | `src/http/parser.c` | Fix dangling pointers, add incremental state |
| `src/http/request.c` | `src/http/types.h` + parser | Request struct definition moves to types |
| `src/http/response.c` | `src/http/response.c` | Add Content-Length/Content-Type, clean API |
| `src/sock/server.c` | Merged into `core/server.c` | Socket creation is a server lifecycle concern |
| `src/utils/config.c` | `src/util/config.c` | Fix atoi, add backend/virtualhost parsing |
| `src/utils/logger.c` | `src/util/log.c` | Fix include guards, add levels |
| `src/main.c` | `src/main.c` | Fix signal handling, use atomic shutdown flag |

## Sources

- nginx source code architecture: well-documented in nginx.org development guide and source tree layout (`src/core/`, `src/event/`, `src/http/`, `src/os/`). Confidence: HIGH (direct source code analysis from training data, cross-referenced with official documentation structure).
- lighttpd source architecture: documented in lighttpd wiki and source tree. `mod_*` handler pattern, `connections.c` state machine, `chunk.c` buffer abstraction. Confidence: HIGH.
- haproxy architecture: stream/session model, connection pooling, health check infrastructure. Confidence: HIGH (well-documented in haproxy.org architecture docs).
- POSIX APIs (epoll, sendfile, signal handling): Confidence: HIGH (stable kernel interfaces, unlikely to have changed).
- The recommended architecture patterns (opaque types, incremental parsing, handler dispatch) are standard C systems programming practices used across all three reference servers. Confidence: HIGH.

---

*Architecture research: 2026-03-17*
