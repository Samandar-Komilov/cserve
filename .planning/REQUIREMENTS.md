# Requirements: cserve

**Defined:** 2026-03-17
**Core Value:** Every HTTP behavior must be correct per RFC 7230-7235, memory-safe (valgrind clean), and written to professional C standards

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### Security & Safety

- [ ] **SEC-01**: Fix path traversal vulnerability — URL-decode first, realpath() both paths, verify resolved path starts with resolved root
- [ ] **SEC-02**: Fix buffer overflows in proxy code — heap-allocate all request data with explicit size limits, no stack buffers for request data
- [x] **SEC-03**: Fix use-after-free in parser — copy all parsed fields with strndup(), no raw pointers into read buffer
- [ ] **SEC-04**: Fix signal handler safety — handler sets only volatile sig_atomic_t flag (or use signalfd()), all cleanup in main loop
- [ ] **SEC-05**: Fix fd sentinel value — use -1 (not 0) after close(), remove from epoll before freeing
- [ ] **SEC-06**: Fix integer overflow in Content-Length parsing — replace atoi with strtoull, body_length int to size_t

### Architecture

- [ ] **ARCH-01**: Decompose monolithic server.c into separate modules: event loop, connection manager, parser, router, handlers
- [ ] **ARCH-02**: Integrate CMocka test framework with CMake/CTest
- [ ] **ARCH-03**: Enable ASan+UBSan as CMake build type for development
- [ ] **ARCH-04**: Integrate cppcheck and clang-tidy into build/CI
- [ ] **ARCH-05**: Implement copy-based incremental HTTP parser (no buffer pointer storage)
- [ ] **ARCH-06**: Implement connection state machine with explicit states: ACCEPTED, READING, PARSING, HANDLING, WRITING, CLOSING

### HTTP Parsing (RFC 7230)

- [ ] **PARSE-01**: Validate Host header — reject HTTP/1.1 requests missing Host with 400 (RFC 7230 §5.4)
- [ ] **PARSE-02**: Reject requests with multiple Host headers with 400 (RFC 7230 §5.4)
- [ ] **PARSE-03**: Decode chunked transfer encoding on incoming requests (RFC 7230 §4.1)
- [ ] **PARSE-04**: Encode chunked transfer encoding on outgoing responses (RFC 7230 §4.1)
- [ ] **PARSE-05**: Reject requests with both Transfer-Encoding and Content-Length (RFC 7230 §3.3.3)
- [ ] **PARSE-06**: Determine message body length per RFC 7230 §3.3.3 priority rules
- [ ] **PARSE-07**: Handle obs-fold in header values — reject or replace with SP (RFC 7230 §3.2.4)
- [ ] **PARSE-08**: Reject request-targets containing whitespace (RFC 7230 §3.5)
- [ ] **PARSE-09**: Enforce request header size limits — reject with 431 (RFC 7230 §3.2.5)
- [ ] **PARSE-10**: Enforce request-line length limit — reject with 414 (RFC 7230 §3.1.1)
- [ ] **PARSE-11**: Reject whitespace between header field-name and colon (RFC 7230 §3.2.4)

### Connection Management (RFC 7230)

- [ ] **CONN-01**: Default to persistent connections in HTTP/1.1 (RFC 7230 §6.3)
- [ ] **CONN-02**: Honor Connection: close — close after response (RFC 7230 §6.1)
- [ ] **CONN-03**: Implement idle connection timeout via timerfd (RFC 7230 §6.3)
- [ ] **CONN-04**: Handle Expect: 100-continue — respond with 100 or final status before reading body (RFC 7231 §5.1.1)

### HTTP Semantics (RFC 7231)

- [ ] **SEM-01**: Support HEAD method — identical to GET but no response body (RFC 7231 §4.3.2)
- [ ] **SEM-02**: Send Date header on all responses except 1xx (RFC 7231 §7.1.1.2)
- [ ] **SEM-03**: Return 405 Method Not Allowed with Allow header for unsupported methods (RFC 7231 §6.5.5)
- [ ] **SEM-04**: Return 413 Payload Too Large when body exceeds limit (RFC 7231 §6.5.11)
- [ ] **SEM-05**: Return 414 URI Too Long (RFC 7231 §6.5.12)
- [ ] **SEM-06**: Return 501 Not Implemented for unrecognized methods (RFC 7231 §6.6.2)
- [ ] **SEM-07**: Return 503 Service Unavailable when backend unreachable (RFC 7231 §6.6.4)
- [ ] **SEM-08**: Return 504 Gateway Timeout when backend times out (RFC 7231 §6.6.5)
- [ ] **SEM-09**: Return 505 HTTP Version Not Supported for non-HTTP/1.x (RFC 7231 §6.6.6)
- [ ] **SEM-10**: Send Server header on responses (RFC 7231 §7.4.2)

### Conditional Requests (RFC 7232)

- [ ] **COND-01**: Send Last-Modified header on static file responses (RFC 7232 §2.2)
- [ ] **COND-02**: Generate and send ETag header on static file responses (RFC 7232 §2.3)
- [ ] **COND-03**: Support If-Modified-Since — return 304 Not Modified when appropriate (RFC 7232 §3.3)
- [ ] **COND-04**: Support If-None-Match — return 304 when ETag matches (RFC 7232 §3.2)
- [ ] **COND-05**: Follow precondition evaluation order per RFC 7232 §6

### Range Requests (RFC 7233)

- [ ] **RANGE-01**: Send Accept-Ranges: bytes header on static file responses (RFC 7233 §2.3)
- [ ] **RANGE-02**: Handle single byte-range requests — return 206 Partial Content (RFC 7233 §4.1)
- [ ] **RANGE-03**: Return 416 Range Not Satisfiable for invalid ranges (RFC 7233 §4.4)

### Static File Serving

- [ ] **STATIC-01**: MIME type detection from file extension (Content-Type header)
- [ ] **STATIC-02**: Directory index — serve index.html for directory requests
- [ ] **STATIC-03**: Hardened path traversal prevention — URL decode → realpath() → root prefix check
- [ ] **STATIC-04**: Use sendfile() for efficient file transfer

### Reverse Proxy

- [ ] **PROXY-01**: Non-blocking backend connections registered with epoll
- [ ] **PROXY-02**: Strip hop-by-hop headers before forwarding (RFC 7230 §6.1)
- [ ] **PROXY-03**: Add Via header to forwarded messages (RFC 7230 §5.7.1)
- [ ] **PROXY-04**: Inject X-Forwarded-For header with client IP
- [ ] **PROXY-05**: Inject X-Forwarded-Proto header
- [ ] **PROXY-06**: Inject X-Forwarded-Host header
- [ ] **PROXY-07**: Configurable backend connect and read timeouts
- [ ] **PROXY-08**: Return 502 Bad Gateway for invalid backend responses (RFC 7231 §6.6.3)
- [ ] **PROXY-09**: Return 503 Service Unavailable when backend refuses connection
- [ ] **PROXY-10**: Return 504 Gateway Timeout when backend times out
- [ ] **PROXY-11**: Forward Authorization headers transparently (RFC 7235 §4.2)

### TLS

- [ ] **TLS-01**: TLS 1.2/1.3 termination via OpenSSL (HTTPS listener)
- [ ] **TLS-02**: Certificate + key file configuration in config file
- [ ] **TLS-03**: SNI support for multiple certificates per IP
- [ ] **TLS-04**: HTTP-to-HTTPS redirect (optional per listener)

### Packaging

- [ ] **PKG-01**: systemd unit file with LimitNOFILE
- [ ] **PKG-02**: CPack-generated .deb package
- [ ] **PKG-03**: CPack-generated .rpm package
- [ ] **PKG-04**: Config installed to /etc/cserve/
- [ ] **PKG-05**: Handle fd exhaustion (EMFILE) gracefully

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Performance & Scaling

- **PERF-01**: Content-Encoding support (gzip/deflate)
- **PERF-02**: Load balancing across multiple backends (round-robin, least-conn)
- **PERF-03**: Connection pooling to backend servers (upstream keepalive)
- **PERF-04**: Multi-threaded or multi-process architecture (master-worker)

### Operations

- **OPS-01**: Access logging (structured, rotatable)
- **OPS-02**: Graceful reload via SIGHUP (config reload, connection draining)
- **OPS-03**: Rate limiting per client/route

### Protocol Extensions

- **PROTO-01**: WebSocket proxying (Upgrade handling)
- **PROTO-02**: Multipart byte ranges (multipart/byteranges)
- **PROTO-03**: Full content negotiation (Accept-Language, Accept-Charset, Vary)

## Out of Scope

| Feature | Reason |
|---------|--------|
| HTTP/2 or HTTP/3 | Different framing model, massive complexity — v1 is HTTP/1.1 only |
| CGI / FastCGI / SCGI | Dynamic content via reverse proxy instead — modern pattern |
| Built-in caching proxy | Invalidation/storage complexity — use Varnish if needed |
| URL rewriting / regex routing | Scope creep — simple path-prefix routing only |
| Lua / scripting extensions | Sandbox security issues, huge complexity |
| Windows / macOS support | Linux-only (epoll) — valid market position |
| HTTP authentication implementation | Auth belongs in app layer — cserve forwards headers only |
| libuv / cross-platform event abstraction | Raw epoll for learning value and embedded systems skill transfer |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| SEC-01 | Phase 1 | Pending |
| SEC-02 | Phase 1 | Pending |
| SEC-03 | Phase 1 | Complete |
| SEC-04 | Phase 1 | Pending |
| SEC-05 | Phase 1 | Pending |
| SEC-06 | Phase 1 | Pending |
| ARCH-01 | Phase 2 | Pending |
| ARCH-02 | Phase 2 | Pending |
| ARCH-03 | Phase 2 | Pending |
| ARCH-04 | Phase 2 | Pending |
| ARCH-05 | Phase 2 | Pending |
| ARCH-06 | Phase 2 | Pending |
| PARSE-01 | Phase 3 | Pending |
| PARSE-02 | Phase 3 | Pending |
| PARSE-03 | Phase 3 | Pending |
| PARSE-04 | Phase 3 | Pending |
| PARSE-05 | Phase 3 | Pending |
| PARSE-06 | Phase 3 | Pending |
| PARSE-07 | Phase 3 | Pending |
| PARSE-08 | Phase 3 | Pending |
| PARSE-09 | Phase 3 | Pending |
| PARSE-10 | Phase 3 | Pending |
| PARSE-11 | Phase 3 | Pending |
| CONN-01 | Phase 3 | Pending |
| CONN-02 | Phase 3 | Pending |
| CONN-03 | Phase 3 | Pending |
| CONN-04 | Phase 3 | Pending |
| SEM-01 | Phase 4 | Pending |
| SEM-02 | Phase 4 | Pending |
| SEM-03 | Phase 4 | Pending |
| SEM-04 | Phase 4 | Pending |
| SEM-05 | Phase 4 | Pending |
| SEM-06 | Phase 4 | Pending |
| SEM-07 | Phase 4 | Pending |
| SEM-08 | Phase 4 | Pending |
| SEM-09 | Phase 4 | Pending |
| SEM-10 | Phase 4 | Pending |
| COND-01 | Phase 5 | Pending |
| COND-02 | Phase 5 | Pending |
| COND-03 | Phase 5 | Pending |
| COND-04 | Phase 5 | Pending |
| COND-05 | Phase 5 | Pending |
| STATIC-01 | Phase 5 | Pending |
| STATIC-02 | Phase 5 | Pending |
| STATIC-03 | Phase 5 | Pending |
| STATIC-04 | Phase 5 | Pending |
| RANGE-01 | Phase 6 | Pending |
| RANGE-02 | Phase 6 | Pending |
| RANGE-03 | Phase 6 | Pending |
| PROXY-01 | Phase 7 | Pending |
| PROXY-02 | Phase 7 | Pending |
| PROXY-03 | Phase 7 | Pending |
| PROXY-04 | Phase 7 | Pending |
| PROXY-05 | Phase 7 | Pending |
| PROXY-06 | Phase 7 | Pending |
| PROXY-07 | Phase 7 | Pending |
| PROXY-08 | Phase 7 | Pending |
| PROXY-09 | Phase 7 | Pending |
| PROXY-10 | Phase 7 | Pending |
| PROXY-11 | Phase 7 | Pending |
| TLS-01 | Phase 8 | Pending |
| TLS-02 | Phase 8 | Pending |
| TLS-03 | Phase 8 | Pending |
| TLS-04 | Phase 8 | Pending |
| PKG-01 | Phase 9 | Pending |
| PKG-02 | Phase 9 | Pending |
| PKG-03 | Phase 9 | Pending |
| PKG-04 | Phase 9 | Pending |
| PKG-05 | Phase 9 | Pending |

**Coverage:**
- v1 requirements: 62 total
- Mapped to phases: 62
- Unmapped: 0 ✓

---
*Requirements defined: 2026-03-17*
*Last updated: 2026-03-17 after initial definition*
