# Project Research Summary

**Project:** cserve — HTTP/1.1 Web Server and Reverse Proxy
**Domain:** C systems programming, Linux network server
**Researched:** 2026-03-17
**Confidence:** HIGH (RFC specifications are stable; architecture patterns from nginx/lighttpd are well-established; pitfalls drawn from CVE history)

## Executive Summary

cserve is a C-based HTTP/1.1 web server and reverse proxy targeting Linux with epoll-based event handling. The project already exists with working but fragile code: it serves files, forwards requests to backends, and handles basic HTTP — but has documented security vulnerabilities (path traversal, buffer overflows), memory safety issues (use-after-free from parser dangling pointers), and RFC compliance gaps that prevent it from being deployed responsibly. The research confirms this is a fixable foundation, not a rewrite-from-scratch situation, but the fixes must happen in a deliberate order because several bugs are load-bearing for later features.

The recommended approach follows the nginx/lighttpd architecture pattern: a protocol-agnostic event loop that delivers events to a connection state machine, which drives an incremental HTTP parser, which dispatches to pluggable handlers (static file or proxy). The critical architectural change from the current monolith is separating these concerns into testable components with narrow interfaces. This refactoring is not optional — the current monolithic event handler is the root cause of multiple bugs and prevents unit testing, which is itself a prerequisite for implementing the remaining RFC 7230-7235 compliance features safely.

The single most important risk mitigation is this sequencing constraint: security fixes must come before feature additions, and parser correctness (copy-based, not pointer-into-buffer) must come before enabling keep-alive connections. Adding features on top of the current unsafe parser will produce vulnerabilities, not progress. The "zero external dependencies" constraint must be relaxed for TLS — OpenSSL 3.x is the only responsible choice, as every production C web server (nginx, Apache, HAProxy) takes the same position.

## Key Findings

### Recommended Stack

The project is correctly using C11 with GNU extensions, CMake, and epoll — these are the right choices and should not change. The additions recommended by research are: OpenSSL 3.x as an optional build-time dependency for TLS (no alternative exists), CMocka for unit testing (replaces the current custom macros that cannot mock and cannot isolate test failures), AddressSanitizer + UBSan as a standard CI build type (would catch the existing use-after-free and integer type bugs immediately), and cppcheck + clang-tidy for static analysis. The current custom INI config parser and custom HTTP parser are correct decisions — both are core competencies for this project and adding external parsers would reduce control over critical components. See `STACK.md` for full rationale and alternatives considered.

**Core technologies:**
- C11 with GNU extensions — application language, already in use, correct choice
- CMake 3.20+ — build system, already in use, correct choice
- OpenSSL 3.x — TLS termination, optional build flag, the only responsible TLS choice
- CMocka 1.1.7+ — unit testing with mocking, replaces custom macros that exit on first failure
- ASan + UBSan — runtime memory/UB detection, catches existing bugs on first run
- cppcheck 2.13+ + clang-tidy 17+ — static analysis, low false-positive complementary tools
- CPack — package generation (.deb, .rpm, .tar.gz), bundled with CMake
- wrk — HTTP load testing and benchmarking

### Expected Features

The research identified substantial RFC compliance gaps in the current code. The MUST-level requirements from RFC 7230-7235 that are currently missing include: chunked transfer encoding (decode and encode), Host header validation (reject missing/duplicate), rejection of TE+CL conflicts (request smuggling prevention), HEAD method, Date header on all responses, 405/413/414/501/504/505 status codes, Via header injection (proxy role), hop-by-hop header stripping before forwarding, X-Forwarded-For/Proto/Host injection, conditional request support (ETag, If-Modified-Since, 304 responses), and range request support (206 Partial Content). The full compliance checklist is in `FEATURES.md`.

**Must have (table stakes — RFC compliance):**
- Chunked transfer encoding — required by HTTP/1.1 spec for unknown-length responses
- Host header validation with 400 rejection — mandatory per RFC 7230 §5.4
- TE+CL conflict rejection — request smuggling prevention, RFC 7230 §3.3.3
- HEAD method — MUST support per RFC 7231 §4.3.2
- Date header — origin server MUST send per RFC 7231 §7.1.1.2
- Complete 4xx/5xx error codes with correct headers (405+Allow, 413, 414, 501, 504)
- Via header injection for proxy role
- Hop-by-hop header stripping before forwarding
- X-Forwarded-For/Proto/Host headers
- Backend connection timeouts
- Conditional requests (ETag, If-Modified-Since, 304)
- Range requests (206 Partial Content)
- MIME type detection for static files
- Directory index (serve index.html)
- TLS termination via OpenSSL (HTTPS listener, certificate config, SNI)

**Should have (differentiators):**
- Strict RFC compliance with every behavior citing the relevant section
- Request smuggling resistance as an explicit tested property
- Valgrind-clean guarantee maintained across the codebase
- Comprehensive HTTP compliance test suite built alongside features

**Defer to v2+:**
- Content-Encoding (gzip/deflate) — complex Accept-Encoding negotiation
- Load balancing with multiple backends and health checks
- Connection pooling to backends
- WebSocket proxying
- Access logging (structured, rotatable)
- Rate limiting
- Graceful reload via SIGHUP

### Architecture Approach

The target architecture separates concerns the same way nginx and lighttpd do: the event loop (epoll) is protocol-agnostic and delivers readability/writability events to a connection state machine; the HTTP parser is incremental and stateful, consuming whatever bytes are available and returning INCOMPLETE/COMPLETE/ERROR; and handlers (static file, proxy) are uniform function pointers registered via config at startup. The primary anti-patterns to eliminate from the current code are: the monolithic event handler that mixes accept/read/parse/dispatch/send, the zero-copy parser that stores raw pointers into the read buffer (root cause of use-after-free), and the single catch-all `common.h` header that prevents module isolation. The refactored connection state machine should have explicit states: ACCEPTED, READING, PARSING, HANDLING, WRITING, CLOSING, with each transition testable independently. See `ARCHITECTURE.md` for full component API definitions and data flow diagrams.

**Major components:**
1. `core/event.c` — epoll wrapper, protocol-agnostic event delivery
2. `core/connection.c` — connection state machine, buffer management, fd lifecycle
3. `http/parser.c` — incremental RFC 7230 request parser, copy-based (no buffer pointers)
4. `http/router.c` — URI pattern matching, handler dispatch from config
5. `handler/static.c` — path resolution, MIME detection, sendfile for static content
6. `handler/proxy.c` + `proxy/upstream.c` — request forwarding, non-blocking backend connections
7. `core/tls.c` (future) — OpenSSL wrapper that slots into the connection I/O layer without restructuring the above

### Critical Pitfalls

Five pitfalls are confirmed in the current codebase and must be addressed before expanding features:

1. **HTTP Request Smuggling** — The server does not reject requests with both TE and CL headers, does not enforce strict chunked parsing, and uses case-sensitive header comparison. Prevent by implementing RFC 7230 §3.3.3 strictly, rejecting ambiguous requests, and fuzzing the parser with h2csmuggler.

2. **Buffer Overflow in Parsing** — Proxy request/response handling uses fixed stack buffers. An attacker sending an oversized request causes stack corruption. Prevent by heap-allocating all request data with explicit size limits, rejecting with 414/431 when exceeded. Replace every `char buf[N]` that touches request data.

3. **Path Traversal in Static Serving** — The current path sanitization is incomplete. URL-encoded variants (`%2e%2e%2f`), null bytes, and double-encoding can escape the document root. Prevent by URL-decoding first, then calling `realpath()` on both the concatenated path and the document root, verifying the resolved path starts with the resolved root, and rejecting any path containing `..` as defense-in-depth.

4. **Use-After-Free in Connection Lifecycle** — The parser stores raw pointers into the read buffer. Buffer reallocation (triggered by large requests) invalidates these pointers. The sentinel fd value is 0, not -1, making "is this fd closed?" checks unreliable. Prevent by copying all parsed fields with `strndup()` in the parser, setting fd to -1 after `close()`, and removing the connection from epoll before freeing the structure.

5. **Signal Handler Corruption** — The current signal handler calls unsafe functions. Prevent by having the handler set only a `volatile sig_atomic_t` flag (or use `signalfd()` to integrate signals into the epoll loop directly). All cleanup runs in the main event loop after flag detection.

Additional pitfalls to design against: Slowloris DoS (requires per-connection timeouts via timerfd), incorrect keep-alive state reset between requests (requires copy-based parser to fix first), integer overflow in Content-Length parsing (atoi to strtoull, body_length int to size_t), and TLS I/O semantics (SSL_ERROR_WANT_READ after SSL_write requires changing epoll flags, not retrying the same operation).

## Implications for Roadmap

The research defines a clear dependency graph: security and safety fixes are blockers for everything else. Architecture restructuring enables testability, which enables safe feature implementation. Features build on each other in RFC compliance order. TLS is last because it slots into the I/O layer cleanly only after the I/O layer is properly abstracted.

### Phase 1: Security and Safety Fixes

**Rationale:** Four confirmed vulnerabilities exist in the current code. Deploying cserve in any context before fixing these is irresponsible. These fixes are also small and targeted — they can be made to the current code structure without requiring the full architecture refactor. Fixing them first reduces the risk surface that the refactor must carry forward.
**Delivers:** A server that is not exploitable via known vulnerability classes in the current code.
**Addresses:** Path traversal, buffer overflows in proxy code, use-after-free from parser pointers, signal handler safety, fd sentinel value, double-close of client fd, atoi integer overflow.
**Avoids:** Pitfalls 2 (Buffer Overflow), 3 (Path Traversal), 4 (Use-After-Free), 5 (Signal Handler), 11 (Integer Overflow).
**Research flag:** Standard patterns — no additional research needed. Issues are documented in CONCERNS.md and map directly to CVE-established mitigations.

### Phase 2: Architecture Restructuring and Testing Infrastructure

**Rationale:** The current monolithic server.c cannot be unit tested, which means every subsequent feature is built without a safety net. The architecture research (ARCHITECTURE.md) defines a concrete target structure with narrow component APIs. Restructuring before adding features means features can be built test-first instead of test-never. This phase does not add user-visible functionality — it is about replacing working-but-unsafe code with working-and-testable code.
**Delivers:** Separated components (event loop, connection manager, parser, router, handlers) each with unit tests. CMocka integrated. ASan+UBSan CI build type enabled. All existing functionality preserved.
**Uses:** CMocka, CMake sanitizer build configuration, clang-tidy, cppcheck.
**Implements:** core/event.c, core/connection.c (with state machine), http/parser.c (copy-based, incremental), http/router.c — as defined in ARCHITECTURE.md.
**Avoids:** Pitfall 1 (Smuggling — incremental parser makes strict validation feasible), Pitfall 8 (epoll edge cases — isolated event loop is testable), Pitfall 10 (Keep-Alive — copy-based parser is prerequisite).
**Research flag:** Well-documented patterns. nginx and lighttpd architectures are the reference implementations. The ARCHITECTURE.md component APIs are ready to implement.

### Phase 3: HTTP/1.1 Compliance — Parsing and Connection Management

**Rationale:** RFC 7230 compliance is the core value proposition. Chunked encoding and persistent connections are both MUST-level requirements that are currently missing. Chunked encoding must be in place before the proxy phase because backends send chunked responses. Persistent connections must be implemented correctly (requires the copy-based parser from Phase 2) before adding higher-level features that assume a working connection lifecycle.
**Delivers:** RFC 7230-compliant request parsing and connection management. Chunked request/response handling. Default keep-alive with Connection: close support. Host header enforcement. TE+CL conflict rejection. Request size limits. Expect: 100-continue. Date header. Correct 4xx/5xx error codes.
**Addresses:** RFC 7230 §3.1-3.3 (message framing), §4.1 (chunked encoding), §5.4 (Host header), §6.1-6.3 (connection management), RFC 7231 error status codes.
**Avoids:** Pitfall 1 (Smuggling — TE+CL rejection, strict chunked parsing), Pitfall 9 (Slowloris — connection timeouts via timerfd), Pitfall 10 (Keep-Alive — proper state reset), Pitfall 11 (Integer Overflow — strtoull for Content-Length).
**Research flag:** Well-documented. RFC 7230-7235 are the specification; no external research needed. Fuzz testing the parser with AFL or libFuzzer is recommended but optional for v1.

### Phase 4: Static File Serving — Complete

**Rationale:** Static file serving is partially implemented but missing MIME detection, directory index, robust path traversal prevention, and conditional request support. These features share a common dependency (stat() metadata) and form a natural group. Range requests depend on ETag/Last-Modified being in place first.
**Delivers:** Complete static file serving with MIME detection, directory indexes, hardened path traversal prevention, conditional request handling (ETag, If-Modified-Since, 304 Not Modified), range requests (206 Partial Content), and sendfile() for performance.
**Addresses:** RFC 7232 (conditional requests), RFC 7233 (range requests), static file table stakes from FEATURES.md.
**Avoids:** Pitfall 3 (Path Traversal — full realpath() + root prefix check after URL decode), Pitfall 14 (MIME types — extension mapping, X-Content-Type-Options: nosniff).
**Research flag:** Standard patterns. The conditional request evaluation order (RFC 7232 §6) and 304 response requirements are fully specified. sendfile() usage is well-documented.

### Phase 5: Reverse Proxy — Complete

**Rationale:** The reverse proxy functionality exists but is missing hop-by-hop header stripping, X-Forwarded-* headers, Via injection, backend timeouts, and non-blocking backend connections. The current synchronous proxy connect/send/recv blocks the event loop — every slow backend stalls all clients. Non-blocking proxy connections require the event loop abstraction from Phase 2 and the connection state machine.
**Delivers:** RFC-compliant reverse proxy with non-blocking backend connections, hop-by-hop header stripping, Via injection, X-Forwarded-For/Proto/Host headers, configurable backend timeouts, correct 502/503/504 responses, and auth header forwarding.
**Addresses:** RFC 7230 §5.7 (Via), §6.1 (hop-by-hop removal), reverse proxy table stakes from FEATURES.md.
**Avoids:** Pitfall 4 (Use-After-Free — non-blocking proxy requires careful connection lifecycle), Pitfall 7 (Response Desynchronization — implement all RFC 7230 §3.3.3 message length rules), Pitfall 12 (Proxy Header Injection — strip and re-add X-Forwarded-* headers, never pass client-supplied values), Pitfall 17 (DNS Blocking — resolve backends at config load time, require IP addresses or hostname-at-startup).
**Research flag:** Moderate complexity. The haproxy stream model (ARCHITECTURE.md) is the reference for non-blocking proxy implementation. The specific behavior of SSL_write/SSL_read interaction with proxy buffering needs attention if TLS is enabled for backend connections.

### Phase 6: TLS Termination

**Rationale:** TLS is last because it requires a clean I/O abstraction layer to slot into. The ARCHITECTURE.md TLS component (`core/tls.c`) wraps connection send/recv without restructuring any other component. Attempting TLS integration before Phase 2's architecture restructure would mean bolting it onto the monolith, which PITFALLS.md §Pitfall 6 identifies as a source of connection hangs, data corruption, and handshake failures under load.
**Delivers:** HTTPS listener with TLS 1.2/1.3, certificate + key file configuration, SNI support for virtual hosting, HTTP-to-HTTPS redirect.
**Uses:** OpenSSL 3.x (the single justified external dependency).
**Avoids:** Pitfall 6 (Incomplete TLS Integration — abstract I/O layer handles SSL_ERROR_WANT_READ/WRITE by changing epoll flags; non-blocking handshake driven by epoll events; disable renegotiation; proper SSL_shutdown() two-phase process).
**Research flag:** Needs careful implementation. The OpenSSL non-blocking I/O semantics (SSL_ERROR_WANT_READ after SSL_write) are a known complexity point. The PITFALLS.md prevention steps are necessary but insufficient — verify against OpenSSL documentation and test with `openssl s_client`, slow clients, and renegotiation attempts.

### Phase 7: Production Hardening and Packaging

**Rationale:** The server needs production-grade observability, resource management, and distribution packaging before it can be recommended for production use.
**Delivers:** systemd unit file with LimitNOFILE, fd exhaustion handling (EMFILE), per-connection timeout enforcement, CPack-generated .deb and .rpm packages, Doxygen documentation, clang-format enforcement in CI.
**Uses:** CPack (bundled with CMake), systemd service file, timerfd for timeout integration.
**Avoids:** Pitfall 13 (File Descriptor Exhaustion), Pitfall 16 (Memory fragmentation — pool allocators for per-request data).
**Research flag:** Standard patterns. CPack, systemd service files, and Linux ulimit configuration are well-documented.

### Phase Ordering Rationale

- Phases 1-2 are non-negotiable blockers: existing vulnerabilities must be fixed, and the architecture must be testable before new features are layered on.
- Phases 3-5 follow the feature dependency graph from FEATURES.md: message framing must be correct before connection management, connection management must be correct before static/proxy features that depend on it, and static/proxy features can proceed in parallel after Phase 3 is complete.
- Phase 6 (TLS) is last because it requires the I/O abstraction from Phase 2 and benefits from having all other features tested before adding TLS complexity.
- Phase 7 (hardening) is last because it addresses production concerns orthogonal to correctness.

### Research Flags

Phases likely needing deeper research or careful validation during planning:
- **Phase 6 (TLS):** OpenSSL non-blocking I/O with epoll is a known complexity trap. Verify SSL_ERROR_WANT_READ/WRITE handling against current OpenSSL 3.x documentation before implementation.
- **Phase 5 (Proxy):** Non-blocking connect() to backends, with timeout via timerfd, requires careful design of the connection state machine extension for the backend side.

Phases with standard well-documented patterns (skip research-phase):
- **Phase 1 (Security fixes):** Fixes are documented in CONCERNS.md, mitigations are standard.
- **Phase 2 (Architecture):** Component APIs are defined in ARCHITECTURE.md; this is an execution phase, not a research phase.
- **Phase 3 (HTTP/1.1 compliance):** RFC 7230 specifies exact behavior; no ambiguity.
- **Phase 4 (Static files):** RFC 7232/7233 + sendfile() are well-documented.
- **Phase 7 (Packaging):** Standard CMake/CPack/systemd patterns.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | MEDIUM | Core choices (C11, CMake, epoll) are HIGH confidence. OpenSSL version (3.2+) and CMocka version (1.1.7+) should be verified at implementation time — based on training data, not live verification. |
| Features | HIGH | RFC 7230-7235 are published, stable standards. Compliance checklist in FEATURES.md is directly derived from the RFCs. Current-state assessment is based on codebase reading. |
| Architecture | HIGH | nginx, lighttpd, haproxy architectures are well-documented public references. Patterns (opaque types, incremental parsing, handler dispatch) are standard C systems programming. |
| Pitfalls | HIGH | All pitfalls are backed by CVE history and RFC specification. The cserve-specific pitfall map is derived from CONCERNS.md, which is the authoritative list of known issues. |

**Overall confidence:** HIGH for the approach, direction, and sequencing. MEDIUM for specific version numbers that should be verified before pinning in build files.

### Gaps to Address

- **DNS resolution policy for proxy backends:** Research recommends requiring IP addresses (or hostname-at-startup resolution) to avoid blocking the event loop with getaddrinfo(). This is a configuration design decision that needs a call: accept the constraint as a documented limitation, or add async DNS (which would require c-ares, breaking the zero-dependencies goal for core functionality). Resolve this during Phase 5 planning.

- **Connection pool sizing:** ARCHITECTURE.md notes Phase 2 should support up to 10K connections. The specific data structure (dynamic array vs. arena vs. linked free list for the connection pool) needs a decision during Phase 2 planning. This affects memory usage and connection acceptance performance under load.

- **OpenSSL version pinning:** STACK.md recommends OpenSSL 3.2+ but cannot verify the exact current version from training data. Verify against https://www.openssl.org/source/ before writing CMake find_package() requirements.

## Sources

### Primary (HIGH confidence)
- RFC 7230-7235 (June 2014, IETF) — HTTP/1.1 compliance checklist, must-have features, smuggling prevention rules
- cserve CONCERNS.md (project file) — confirmed vulnerability and bug list, maps to pitfalls
- nginx source architecture (nginx.org development guide) — event loop, connection, module patterns
- lighttpd source architecture (lighttpd wiki) — connection state machine, handler dispatch
- haproxy architecture documentation — stream model, proxy buffering patterns
- POSIX/Linux kernel documentation — epoll, sendfile(), signalfd(), timerfd()

### Secondary (MEDIUM confidence)
- CVE database entries for nginx, Apache httpd, curl — pitfall CVE cross-references (from training data up to early 2025)
- Training data knowledge of C systems programming ecosystem — version numbers for OpenSSL, CMocka, cppcheck, Valgrind

### Tertiary (requires verification)
- Specific version numbers in STACK.md — verify at project setup time before pinning

---
*Research completed: 2026-03-17*
*Ready for roadmap: yes*
