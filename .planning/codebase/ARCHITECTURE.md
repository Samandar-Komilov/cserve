# Architecture

**Analysis Date:** 2026-03-17

## Pattern Overview

**Overall:** Layered event-driven architecture with separation of concerns across socket, HTTP, and application logic layers.

**Key Characteristics:**
- Epoll-based event loop for scalable connection handling
- Static HTTP request/response objects with pointer-based parsing (zero-copy approach)
- Reverse proxy capability with sequential backend connection handling
- Connection pooling with state machine (CONN_ESTABLISHED → CONN_PROCESSING → CONN_SENDING_RESPONSE → CONN_CLOSING)

## Layers

**Socket Layer:**
- Purpose: Abstract TCP socket creation and configuration
- Location: `src/sock/server.c`, `src/sock/server.h`
- Contains: `SocketServer` struct initialization, domain/service/protocol setup
- Depends on: POSIX socket APIs
- Used by: HTTPServer (initiates underlying socket)

**HTTP Protocol Layer:**
- Purpose: Parse HTTP requests, build HTTP responses, serialize for transmission
- Location: `src/http/` (request.c/h, response.c/h, parsers.c/h)
- Contains: Request/response data structures, parsing state machines, serialization
- Depends on: common.h enums and constants
- Used by: HTTPServer request handler

**Server/Event Loop Layer:**
- Purpose: Accept connections, multiplex I/O with epoll, orchestrate request-response cycle
- Location: `src/http/server.c`, `src/http/server.h`
- Contains: Connection management, event loop, request handler, response serialization
- Depends on: Socket layer, HTTP layer, configuration
- Used by: main.c entry point

**Utilities Layer:**
- Purpose: Configuration parsing, logging
- Location: `src/utils/` (config.c/h, logger.c/h)
- Contains: INI config file parsing, structured logging with timestamps
- Depends on: POSIX file I/O
- Used by: main.c, HTTPServer

## Data Flow

**Incoming Request:**

1. Main event loop calls epoll_wait() on listening socket and client sockets
2. Accept new TCP connection → allocate Connection struct from pool
3. Epoll registers new client socket as readable event
4. recv() reads bytes into Connection.buffer (dynamic, grows on demand)
5. parse_http_request() scans buffer for complete request (request line + headers + blank line)
6. Request parsed → request_handler() executed with populated HTTPRequest struct

**Request Handling:**

1. request_handler() examines HTTPRequest.uri for routing
2. Routes: `/static/*` → file serving from disk, `/api/*` → reverse proxy, otherwise 404
3. File serving: open(filepath), read() into buffer, serialize HTTPResponse, send()
4. Reverse proxy: connect_to_backend(host, port), forward request, recv() response, relay to client
5. HTTPResponse serialized with status line + headers + body separator + body

**Outgoing Response:**

1. httpresponse_serialize() builds complete HTTP response as contiguous buffer
2. send() writes response bytes to client socket
3. Check for Connection: keep-alive header
4. Keep-alive: reset_connection() clears buffer/request, awaits next request on same fd
5. No keep-alive: epoll_ctl(DEL), close() socket, free_connection() clears pool slot

**State Management:**

Connection lifetime tracked via `Connection.state` enum:
- CONN_ESTABLISHED: Socket accepted, buffer allocated, awaiting data
- CONN_PROCESSING: Data received, parsing/handling in progress
- CONN_SENDING_RESPONSE: Response being transmitted
- CONN_CLOSING: Socket marked for closure after cleanup
- CONN_ERROR: Protocol/IO error, immediate cleanup required

## Key Abstractions

**Connection:**
- Purpose: Represents a single client socket and its associated request/response lifecycle
- Examples: `src/http/server.h` lines 17-28
- Pattern: Object-oriented C with embedded data; init_connection(), reset_connection(), free_connection() utility functions

**HTTPRequest:**
- Purpose: Immutable (pointer-based) representation of parsed HTTP request
- Examples: `src/http/request.h` lines 14-40
- Pattern: Parsing functions store pointers into original buffer, no string copies; state tracked via HTTPRequestState enum

**HTTPResponse:**
- Purpose: Mutable builder for constructing responses before serialization
- Examples: `src/http/response.h` lines 14-25
- Pattern: Constructor allocates, properties set directly or via httpresponse_add_header(), serialization on demand

**SocketServer:**
- Purpose: Low-level TCP socket with address binding configuration
- Examples: `src/sock/server.h` lines 26-39
- Pattern: Minimal wrapper; constructor creates socket and applies SO_REUSEADDR, destructor closes fd

## Entry Points

**main():**
- Location: `src/main.c` lines 19-53
- Triggers: Process startup
- Responsibilities: Signal handlers, config parsing, HTTPServer construction, launch, cleanup

**launch():**
- Location: `src/http/server.c` lines 11-343
- Triggers: httpserver->launch() called from main after construction
- Responsibilities: Bind socket, listen, epoll_create1(), event loop, accept connections, read/parse/handle requests, send responses

**request_handler():**
- Location: `src/http/server.c` lines 345-516
- Triggers: Called when HTTPRequest fully parsed
- Responsibilities: Route based on URI path, execute handler (file serve or proxy), return HTTPResponse

**parse_http_request():**
- Location: `src/http/parsers.c` lines 82-142
- Triggers: Called when data available in connection buffer
- Responsibilities: Parse request line, headers, extract method/uri/protocol, populate HTTPRequest struct

## Error Handling

**Strategy:** Return negative error codes on failure; log errors with LOG(ERROR, ...) macro; attempt graceful connection closure.

**Patterns:**
- Sentinel return values: Functions return -1 or -ErrorCode on failure (e.g., INVALID_REQUEST_LINE = -711)
- Logging: LOG(level, fmt, ...) wraps file/line/timestamp, used throughout for debugging
- Socket errors: EAGAIN/EWOULDBLOCK handled as non-fatal; other errors trigger CONN_ERROR state
- Memory errors: Allocation failures logged and signal connection/request error; pool exhaustion results in connection rejection
- Proxy errors: Failed backend connection returns 502 Bad Gateway; response relay errors logged

## Cross-Cutting Concerns

**Logging:** LOG(level, file, line, fmt, ...) macro defined in `src/utils/logger.h` lines 5; implementation in `src/utils/logger.c` provides formatted output with timestamp, file, line number, and level.

**Validation:**
- Request line parsing validates presence of method/uri/protocol and CRLF terminator
- Headers validated: colon required, whitespace handling, CRLF required
- Content-Length extracted from headers for request body length
- Path validation: realpath() used in file serving to prevent directory traversal

**Authentication:** Not implemented; no auth layer present. Reverse proxy forwards requests as-is without credential verification.

---

*Architecture analysis: 2026-03-17*
