# Functional Programming Patterns in CServe

## Overview

Introduce FP patterns alongside existing OOP-in-C patterns. Goal: reduce complexity in data transformation and logic paths while keeping OOP (constructor/destructor, state machines) for resource management.

---

## Pattern 1: Result Type for Error Handling

**Where**: Replace negative error code returns in parsers and builders.

**Files to modify**:
- `src/common.h` — define `Result` type
- `src/http/request.c` — return `Result` from `parse_http_request`
- `src/http/response.c` — return `Result` from `response_builder`, `httpresponse_serialize`

**Implementation**:
```c
// src/common.h
typedef struct {
    enum { RESULT_OK, RESULT_ERR } tag;
    union {
        void *value;
        ErrorCode error;
    };
} Result;

static inline Result Ok(void *value) {
    return (Result){ .tag = RESULT_OK, .value = value };
}

static inline Result Err(ErrorCode code) {
    return (Result){ .tag = RESULT_ERR, .error = code };
}

// Convenience macros
#define IS_OK(r)  ((r).tag == RESULT_OK)
#define IS_ERR(r) ((r).tag == RESULT_ERR)
#define UNWRAP(r, T) ((T *)(r).value)
#define TRY(expr) do { Result _r = (expr); if (IS_ERR(_r)) return _r; } while(0)
```

**Before → After**:
```c
// Before: caller must know -711 means invalid request line
int consumed = parse_http_request(buf, len, req);
if (consumed < 0) { /* error */ }

// After: explicit, self-documenting
Result res = parse_http_request(buf, len);
if (IS_ERR(res)) {
    LOG("ERROR", "Parse failed: %d", res.error);
    return res;
}
HTTPRequest *req = UNWRAP(res, HTTPRequest);
```

---

## Pattern 2: Route Table with Handler Functions

**Where**: Replace if/else dispatch in request handling.

**Files to modify**:
- `src/http/router.h` (new) — route table types and lookup
- `src/http/router.c` (new) — `find_handler` implementation
- `src/http/server.c` — use router in `handle_request`

**Implementation**:
```c
// src/http/router.h
typedef HTTPResponse *(*RequestHandler)(const HTTPRequest *req, void *ctx);

typedef struct {
    const char *method;
    const char *path;
    RequestHandler handler;
} Route;

RequestHandler find_handler(const Route *routes, size_t count,
                            const HTTPRequest *req);

// src/http/router.c
RequestHandler find_handler(const Route *routes, size_t count,
                            const HTTPRequest *req) {
    for (size_t i = 0; i < count; i++) {
        if (strcmp(routes[i].method, req->request_line.method) == 0 &&
            strcmp(routes[i].path, req->request_line.path) == 0)
            return routes[i].handler;
    }
    return NULL;
}
```

**Usage**:
```c
// Handlers are pure functions
HTTPResponse *handle_static(const HTTPRequest *req, void *ctx) {
    const char *static_dir = (const char *)ctx;
    // serve file...
}

// Routes declared as data
Route routes[] = {
    {"GET",  "/",       handle_index},
    {"GET",  "/health", handle_health},
};

RequestHandler h = find_handler(routes, ARRAY_LEN(routes), req);
HTTPResponse *resp = h ? h(req, ctx) : response_builder(404, ...);
```

---

## Pattern 3: Connection Iterators (Map/Filter/Fold)

**Where**: Replace manual loops over connection arrays.

**Files to modify**:
- `src/http/server.h` — add iterator types
- `src/http/server.c` — refactor connection loops

**Implementation**:
```c
// Callback type: operates on one connection
typedef void (*ConnAction)(Connection *conn, void *ctx);
typedef bool (*ConnPredicate)(const Connection *conn, void *ctx);

// Apply action to all connections matching predicate
void conn_for_each(Connection *conns, size_t count,
                   ConnPredicate pred, ConnAction action, void *ctx);

// Count connections matching predicate
size_t conn_count_if(Connection *conns, size_t count,
                     ConnPredicate pred, void *ctx);
```

**Before → After**:
```c
// Before: manual loop with inlined logic
for (size_t j = 0; j < MAX_CONNECTIONS; j++) {
    if (self->connections[j].socket != 0 &&
        time(NULL) - self->connections[j].last_active > 30) {
        close_connection(&self->connections[j]);
    }
}

// After: declarative
bool is_idle(const Connection *c, void *ctx) {
    time_t *timeout = ctx;
    return c->socket != 0 && (time(NULL) - c->last_active > *timeout);
}

void do_close(Connection *c, void *ctx) { close_connection(c); }

time_t timeout = 30;
conn_for_each(self->connections, MAX_CONNECTIONS, is_idle, do_close, &timeout);
```

---

## Pattern 4: Immutable Configuration

**Where**: Server setup and initialization.

**Files to modify**:
- `src/http/server.h` — add `ServerConfig` struct
- `src/http/server.c` — take `const ServerConfig *` in constructor

**Implementation**:
```c
typedef struct {
    int port;
    const char *static_dir;
    const char **proxy_backends;
    int backend_count;
    int max_connections;
    time_t idle_timeout;
} ServerConfig;

// Config is const — cannot be modified after creation
HTTPServer *httpserver_create(const ServerConfig *cfg);
```

**Usage**:
```c
const ServerConfig cfg = {
    .port = 8080,
    .static_dir = "./static",
    .max_connections = 1024,
    .idle_timeout = 30,
};
HTTPServer *srv = httpserver_create(&cfg);
```

---

## Pattern 5: Const-Correct Function Signatures

**Where**: All functions that don't need to mutate their inputs.

**Files to modify**: All headers — audit every function parameter.

**Rule**: If a function doesn't modify a pointer parameter, mark it `const`.

```c
// Before
char *httpresponse_serialize(HTTPResponse *res, size_t *out_len);
int parse_http_request(char *buffer, size_t len, HTTPRequest *req);

// After
char *httpresponse_serialize(const HTTPResponse *res, size_t *out_len);
Result parse_http_request(const char *buffer, size_t len);
```

---

## Pattern 6: Processing Pipeline

**Where**: Request handling flow (read → parse → route → respond).

**Files to modify**:
- `src/http/pipeline.h` (new) — pipeline types
- `src/http/server.c` — refactor event loop to use pipeline

**Implementation**:
```c
typedef Connection *(*PipelineStage)(Connection *conn, void *ctx);

Connection *run_pipeline(Connection *conn, void *ctx,
                         PipelineStage *stages, size_t count) {
    for (size_t i = 0; i < count; i++) {
        conn = stages[i](conn, ctx);
        if (!conn) return NULL;  // short-circuit on failure
    }
    return conn;
}

// Declare pipeline as data
PipelineStage request_pipeline[] = {
    stage_read,
    stage_parse,
    stage_route,
    stage_respond,
};
```

---

## Implementation Order

| Priority | Pattern | Effort | Impact |
|----------|---------|--------|--------|
| 1 | Const-correct signatures | Low | Foundation for everything else |
| 2 | Result type | Low | Safer error handling across codebase |
| 3 | Route table | Medium | Cleaner dispatch, extensible |
| 4 | Immutable config | Low | Cleaner initialization |
| 5 | Connection iterators | Medium | Reduces loop boilerplate |
| 6 | Processing pipeline | High | Major refactor of event loop |

## What to Keep as OOP

- **Constructor/destructor pairs** — C has no RAII; manual cleanup stays
- **State machines** — `ConnectionState`, `HTTPRequestState` are already clean
- **Event loop core** — mutation is appropriate for performance-critical paths
- **Resource ownership** — malloc/free with clear ownership semantics
