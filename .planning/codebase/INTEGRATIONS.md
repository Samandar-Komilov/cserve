# External Integrations

**Analysis Date:** 2026-03-17

## Overview

CServe is a self-contained HTTP server with reverse proxy capabilities. It has **minimal external dependencies** — no third-party libraries, no databases, no cloud services, and no authentication providers. All integrations are internal to the application or are standard POSIX/Linux system calls.

## Static File Serving

**Local Filesystem:**
- Serves static files from configurable directory
- Configuration: `static_dir` in `cserver.ini`
- Implementation: Direct filesystem read operations
  - Files: `src/http/server.c` - File serving logic
  - MIME type detection: `src/http/parsers.c` - `get_mime_type()` function

## Reverse Proxy & Backend Integration

**Backend Server Connectivity:**
- Supports proxying requests to upstream backend servers
- Configuration: Multiple `backend=` entries in `cserver.ini`
  - Max backends: 16 (`MAX_BACKENDS` in `src/common.h`)
- Protocol: Raw HTTP over TCP/IP

**Connection Method:**
```c
// Function: src/http/server.c - connect_to_backend()
int connect_to_backend(const char *host, const char *port)
```

**Details:**
- Uses standard `getaddrinfo()` for DNS resolution
- Creates raw socket connection (SOCK_STREAM)
- Sends HTTP request directly to backend
- Receives HTTP response from backend
- File: `src/http/server.c` (lines ~461-480 for proxy request handling)

**Proxy Request Forwarding:**
```c
// Example from src/http/server.c:
int backend_fd = connect_to_backend("localhost", "8002");
snprintf(proxy_request, sizeof(proxy_request),
    "%.*s %s HTTP/1.1\r\nHost: localhost\r\n\r\n",
    request_ptr->request_line.method_len, request_ptr->request_line.method,
    api_path);
send(backend_fd, proxy_request, proxy_request_len, 0);
```

**Routing:**
- Requests to `/api/*` paths routed to backend servers
- Implementation: `src/http/server.c` - `request_handler()` function
- Currently hardcoded backend: `localhost:8002`
- TODO: Dynamic backend selection (not yet implemented)

## Network & Socket Layer

**Protocol Support:**
- HTTP/1.1 (partial implementation)
- TCP/IP (IPv4 and IPv6 socket structures prepared)

**Socket Operations:**
- Standard POSIX sockets (`sys/socket.h`)
- Linux epoll for event multiplexing (`sys/epoll.h`)
  - Enables handling 1000+ concurrent connections
  - File: `src/http/server.c` - epoll event loop
  - Max events per poll: 1024 (`MAX_EPOLL_EVENTS`)

**Port Configuration:**
- Configurable via `port=` in `cserver.ini`
- Default: 8000 (example, not hardcoded)
- Single listening port per instance

## Configuration Management

**Config File Format:**
- INI-style configuration
- Location: `cserver.ini` (current directory by default)
- Parser: `src/utils/config.c`

**Configuration Options:**
```ini
port=8000
root=/path/to/root
static_dir=./static
backend=localhost:8002
backend=localhost:8003
```

**Supported Keys:**
- `port` - Server listening port (integer)
- `root` - Root directory (string)
- `static_dir` - Static files directory (string)
- `backend` - Backend upstream (repeatable, max 16)

**Implementation:**
- Parser function: `src/utils/config.c` - `parse_config()`
- Struct: `src/utils/config.h` - `Config` struct
- Comments: Lines starting with `#`
- Format: `key=value` pairs

## Logging & Observability

**Logging System:**
- Custom in-process logger (no external service)
- Output: stderr (via fprintf)
- File: `src/utils/logger.h` and `src/utils/logger.c`

**Log Levels:**
- INFO
- DEBUG
- ERROR

**Log Macro:**
```c
#define LOG(level, fmt, ...) \
    log_message(level, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
```

**Features:**
- File and line number information
- Variable argument support
- Timestamp (via `time.h`)

**Implementation:**
- `src/utils/logger.c` - `log_message()` function
- Writes to stderr with file/line info
- No external logging service (Sentry, DataDog, etc.)

## Signal Handling

**OS-Level Signal Integration:**
- Graceful shutdown via SIGINT, SIGTERM
- Segfault detection via SIGSEGV
- Pipe handling via SIGPIPE (ignored)

**Implementation:**
- File: `src/main.c` - `handle_signal()` function
- Cleans up resources on termination
- Destroys HTTPServer and Config structs

## HTTP Request/Response Handling

**Parsing:**
- Custom HTTP parser (no external HTTP library)
- Files: `src/http/parsers.c`, `src/http/request.c`, `src/http/response.c`

**Request Parsing:**
- Parses request line: method, URI, protocol version
- Parses headers into dynamic array (max 50)
- Parses body (if Content-Length specified)
- Error detection: malformed requests rejected

**Response Building:**
- Dynamic response serialization
- Status codes and reason phrases
- Custom headers support
- Content-Type detection (MIME types)
- File: `src/http/response.c` - `response_builder()` function

## Connection Management

**Stateful Connection Tracking:**
- Per-connection state machine (ESTABLISHED, PROCESSING, SENDING_RESPONSE, CLOSING, ERROR)
- Keep-alive support (configurable per request)
- Request pipelining support (multiple requests per connection)
- Connection pooling: Pre-allocated slots (max 1000)

**Implementation:**
- Struct: `src/common.h` - `Connection` typedef
- Lifecycle: `src/http/server.c` - `init_connection()`, `free_connection()`, `reset_connection()`
- Timeout handling: 60-second epoll wait

## No External Services/Integrations

**NOT USED:**
- No database (SQL, NoSQL, in-process)
- No message queues (Redis, RabbitMQ)
- No cloud APIs (AWS, GCP, Azure)
- No authentication services (OAuth, LDAP)
- No CDN or static asset delivery networks
- No rate limiting services
- No APM/monitoring platforms
- No distributed tracing (Jaeger, Zipkin)
- No third-party cryptography libraries (uses only standard C)

---

*Integration audit: 2026-03-17*
