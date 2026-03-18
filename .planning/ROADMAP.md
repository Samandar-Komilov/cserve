# Roadmap: cserve

## Overview

cserve goes from a working-but-vulnerable HTTP server to a production-ready, RFC 7230-7235 compliant web server and reverse proxy. The journey is fix-first: eliminate security vulnerabilities, restructure for testability, then layer protocol compliance features in dependency order. Every phase delivers a coherent, verifiable capability that the next phase builds on. TLS and packaging come last because they slot cleanly onto a correct foundation but create chaos on a broken one.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Security and Safety** - Eliminate all known vulnerabilities before building on the codebase
- [ ] **Phase 2: Architecture and Testing** - Decompose monolith into testable modules with CI safety nets
- [ ] **Phase 3: HTTP Parsing and Connection Management** - RFC 7230 compliant message framing, chunked encoding, and persistent connections
- [ ] **Phase 4: HTTP Semantics** - RFC 7231 method handling, status codes, and required response headers
- [ ] **Phase 5: Static File Serving** - Complete static file handler with MIME types, conditional requests, and cache validation
- [ ] **Phase 6: Range Requests** - RFC 7233 byte-range support for efficient partial content delivery
- [ ] **Phase 7: Reverse Proxy** - Non-blocking RFC-compliant proxy with header rewriting and backend error handling
- [ ] **Phase 8: TLS Termination** - HTTPS via OpenSSL with SNI and certificate configuration
- [ ] **Phase 9: Packaging and Production Hardening** - systemd integration, .deb/.rpm packages, and resource limit handling

## Phase Details

### Phase 1: Security and Safety
**Goal**: The server is not exploitable via any known vulnerability class in the current code
**Depends on**: Nothing (first phase)
**Requirements**: SEC-01, SEC-02, SEC-03, SEC-04, SEC-05, SEC-06
**Success Criteria** (what must be TRUE):
  1. Requests containing path traversal sequences (encoded or not) cannot access files outside the document root
  2. Oversized HTTP requests to the proxy do not cause stack or heap corruption
  3. The HTTP parser does not hold raw pointers into the read buffer -- all parsed fields are independent copies
  4. Sending SIGTERM or SIGINT during active request handling shuts down cleanly without calling unsafe functions in the signal handler
  5. Closing a connection sets the fd to -1 and removes it from epoll before freeing -- no double-close or use-after-free possible
**Plans**: 3 plans

Plans:
- [ ] 01-01-PLAN.md -- Foundation fixes: fd sentinel (SEC-05), Content-Length parsing (SEC-06), size limit constants, adjacent Critical/High bugs
- [ ] 01-02-PLAN.md -- Parser use-after-free fix (SEC-03): strndup all parsed fields, update free_http_request
- [ ] 01-03-PLAN.md -- Path traversal (SEC-01), proxy buffer overflows (SEC-02), signal handler safety (SEC-04), smoke tests

### Phase 2: Architecture and Testing
**Goal**: The codebase is decomposed into isolated, testable modules with automated safety nets that catch regressions
**Depends on**: Phase 1
**Requirements**: ARCH-01, ARCH-02, ARCH-03, ARCH-04, ARCH-05, ARCH-06
**Success Criteria** (what must be TRUE):
  1. Each major component (event loop, connection manager, parser, router, handlers) lives in its own source file with a narrow public API
  2. Running `ctest` executes unit tests for the parser, connection state machine, and router -- all passing
  3. Building with the ASan+UBSan build type and running the test suite produces zero sanitizer errors
  4. cppcheck and clang-tidy run as part of the build and report zero warnings on project code
  5. All existing functionality (static serving, proxy forwarding, config parsing) still works after restructuring
**Plans**: TBD

Plans:
- [ ] 02-01: TBD
- [ ] 02-02: TBD

### Phase 3: HTTP Parsing and Connection Management
**Goal**: The server handles HTTP/1.1 message framing, persistent connections, and transfer encoding correctly per RFC 7230
**Depends on**: Phase 2
**Requirements**: PARSE-01, PARSE-02, PARSE-03, PARSE-04, PARSE-05, PARSE-06, PARSE-07, PARSE-08, PARSE-09, PARSE-10, PARSE-11, CONN-01, CONN-02, CONN-03, CONN-04
**Success Criteria** (what must be TRUE):
  1. HTTP/1.1 requests without a Host header receive a 400 response; requests with duplicate Host headers receive a 400 response
  2. The server correctly decodes chunked request bodies and encodes chunked response bodies
  3. Requests containing both Transfer-Encoding and Content-Length are rejected with 400 (smuggling prevention)
  4. HTTP/1.1 connections remain open by default for multiple request-response cycles; Connection: close causes the server to close after the response
  5. Idle connections are closed after a configurable timeout period via timerfd
  6. Requests exceeding header size or URI length limits receive 431 or 414 respectively
  7. A client sending Expect: 100-continue receives a 100 Continue before the server reads the body
**Plans**: TBD

Plans:
- [ ] 03-01: TBD
- [ ] 03-02: TBD
- [ ] 03-03: TBD

### Phase 4: HTTP Semantics
**Goal**: The server implements correct HTTP method handling, error status codes, and required response headers per RFC 7231
**Depends on**: Phase 3
**Requirements**: SEM-01, SEM-02, SEM-03, SEM-04, SEM-05, SEM-06, SEM-07, SEM-08, SEM-09, SEM-10
**Success Criteria** (what must be TRUE):
  1. HEAD requests return the same headers as GET but with no response body
  2. Every response includes a Date header (except 1xx) and a Server header
  3. Requesting with an unsupported method returns 405 with an Allow header listing supported methods; unrecognized methods return 501
  4. Requests with bodies exceeding the configured limit receive 413; URIs exceeding the limit receive 414
  5. Non-HTTP/1.x requests receive 505 HTTP Version Not Supported
**Plans**: TBD

Plans:
- [ ] 04-01: TBD
- [ ] 04-02: TBD

### Phase 5: Static File Serving
**Goal**: The static file handler delivers files with correct MIME types, supports conditional requests for cache validation, and prevents path traversal
**Depends on**: Phase 4
**Requirements**: STATIC-01, STATIC-02, STATIC-03, STATIC-04, COND-01, COND-02, COND-03, COND-04, COND-05
**Success Criteria** (what must be TRUE):
  1. Served files have correct Content-Type headers based on file extension (html, css, js, images, etc.)
  2. Requesting a directory serves index.html if present; returns 404 otherwise
  3. Path traversal attempts (url-encoded, double-encoded, null bytes) are blocked and never resolve outside the document root
  4. Static file responses include Last-Modified and ETag headers derived from file metadata
  5. Requests with If-Modified-Since or If-None-Match that match the current file receive 304 Not Modified with no body
**Plans**: TBD

Plans:
- [ ] 05-01: TBD
- [ ] 05-02: TBD

### Phase 6: Range Requests
**Goal**: Clients can request partial file content for resumable downloads and media streaming
**Depends on**: Phase 5
**Requirements**: RANGE-01, RANGE-02, RANGE-03
**Success Criteria** (what must be TRUE):
  1. Static file responses include Accept-Ranges: bytes header
  2. A request with a valid Range header receives 206 Partial Content with the correct Content-Range header and partial body
  3. A request with an unsatisfiable range receives 416 Range Not Satisfiable with a Content-Range header indicating the full file size
**Plans**: TBD

Plans:
- [ ] 06-01: TBD

### Phase 7: Reverse Proxy
**Goal**: The server forwards requests to backend servers with correct header rewriting, non-blocking I/O, and proper error handling
**Depends on**: Phase 4
**Requirements**: PROXY-01, PROXY-02, PROXY-03, PROXY-04, PROXY-05, PROXY-06, PROXY-07, PROXY-08, PROXY-09, PROXY-10, PROXY-11
**Success Criteria** (what must be TRUE):
  1. Backend connections are non-blocking and registered with epoll -- a slow backend does not stall other clients
  2. Hop-by-hop headers (Connection, Keep-Alive, TE, Transfer-Encoding, Upgrade) are stripped before forwarding to the backend
  3. Forwarded requests include Via, X-Forwarded-For, X-Forwarded-Proto, and X-Forwarded-Host headers with correct values
  4. Backend connect and read timeouts are configurable; timeout produces 504, connection refused produces 503, malformed response produces 502
  5. Authorization headers from the client are forwarded transparently to the backend
**Plans**: TBD

Plans:
- [ ] 07-01: TBD
- [ ] 07-02: TBD
- [ ] 07-03: TBD

### Phase 8: TLS Termination
**Goal**: The server accepts HTTPS connections with modern TLS and supports virtual hosting via SNI
**Depends on**: Phase 7
**Requirements**: TLS-01, TLS-02, TLS-03, TLS-04
**Success Criteria** (what must be TRUE):
  1. The server accepts TLS 1.2 and TLS 1.3 connections and serves content over HTTPS
  2. Certificate and key file paths are configured in the config file; invalid paths produce clear startup errors
  3. Multiple virtual hosts with different certificates work on the same IP via SNI
  4. HTTP requests to the HTTPS port (or vice versa) can be redirected with a 301 when redirect is enabled in config
**Plans**: TBD

Plans:
- [ ] 08-01: TBD
- [ ] 08-02: TBD

### Phase 9: Packaging and Production Hardening
**Goal**: The server can be installed from a system package and runs as a managed systemd service with proper resource limits
**Depends on**: Phase 8
**Requirements**: PKG-01, PKG-02, PKG-03, PKG-04, PKG-05
**Success Criteria** (what must be TRUE):
  1. `dpkg -i cserve.deb` installs the binary, config to /etc/cserve/, and systemd unit file; `systemctl start cserve` starts the server
  2. `rpm -i cserve.rpm` achieves the same on RPM-based systems
  3. The systemd unit sets LimitNOFILE appropriately and the server starts under systemd supervision
  4. When the process hits EMFILE (fd exhaustion), it stops accepting new connections temporarily instead of crashing, and resumes when fds become available
**Plans**: TBD

Plans:
- [ ] 09-01: TBD
- [ ] 09-02: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7 -> 8 -> 9

Note: Phase 7 (Reverse Proxy) depends on Phase 4 (not Phase 6), so Phases 5-6 and Phase 7 could theoretically execute in parallel. However, for a solo developer workflow, sequential execution is recommended.

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Security and Safety | 0/3 | Planning complete | - |
| 2. Architecture and Testing | 0/? | Not started | - |
| 3. HTTP Parsing and Connection Management | 0/? | Not started | - |
| 4. HTTP Semantics | 0/? | Not started | - |
| 5. Static File Serving | 0/? | Not started | - |
| 6. Range Requests | 0/? | Not started | - |
| 7. Reverse Proxy | 0/? | Not started | - |
| 8. TLS Termination | 0/? | Not started | - |
| 9. Packaging and Production Hardening | 0/? | Not started | - |
