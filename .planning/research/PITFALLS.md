# Domain Pitfalls

**Domain:** C-based HTTP/1.1 web server and reverse proxy
**Researched:** 2026-03-17
**Confidence:** HIGH (well-established domain with decades of CVE history)

## Critical Pitfalls

Mistakes that cause security vulnerabilities, data corruption, or require architectural rewrites.

---

### Pitfall 1: HTTP Request Smuggling via Ambiguous Parsing

**What goes wrong:** The server interprets request boundaries differently than upstream/downstream proxies. An attacker crafts a request with conflicting `Content-Length` and `Transfer-Encoding` headers (or malformed chunked encoding) so the server sees one request where a proxy sees two, or vice versa.

**Why it happens:** HTTP/1.1 allows both `Content-Length` and `Transfer-Encoding: chunked` in the same message. RFC 7230 Section 3.3.3 defines precedence rules, but many implementations get this wrong, especially when handling malformed chunk sizes, trailing whitespace in Content-Length, or duplicate Content-Length headers with different values.

**Consequences:** Cache poisoning, request hijacking, authentication bypass. An attacker can smuggle a second request that gets processed with another user's credentials.

**Prevention:**
1. If both `Transfer-Encoding` and `Content-Length` are present, ignore `Content-Length` (RFC 7230 S3.3.3 rule 3)
2. Reject requests with multiple differing `Content-Length` values (400 Bad Request)
3. Reject requests with `Transfer-Encoding` values you don't understand
4. Parse chunk sizes strictly: reject non-hex characters, enforce CRLF terminators, reject chunk extensions you don't understand
5. When acting as reverse proxy, normalize requests before forwarding -- strip ambiguous headers

**Detection:** Fuzz test with tools like `h2csmuggler`, `smuggler.py`. Test malformed chunked encoding, duplicate headers, `\n` vs `\r\n` line endings.

**Real-world CVEs:**
- **CVE-2019-20372 (nginx):** HTTP request smuggling via error pages. nginx mishandled certain error conditions allowing request smuggling between nginx and backend servers.
- **CVE-2023-44487 (multiple):** While HTTP/2 specific, illustrates how protocol-level parsing differences between proxy layers create exploitable gaps.
- **CVE-2022-22720 (Apache httpd):** Failed to close inbound connection when encountering errors during request body discarding, enabling request smuggling.
- **CVE-2015-3183 (Apache httpd):** Chunked transfer encoding parsing allowed request smuggling via malformed chunk-size values.

**Phase mapping:** Must be addressed during HTTP/1.1 compliance phase (chunked encoding implementation). Test extensively when adding reverse proxy forwarding.

---

### Pitfall 2: Buffer Overflow in Header/URI Parsing

**What goes wrong:** HTTP request lines and headers have no inherent size limit in the spec. Attacker sends a 1GB URI or header value, and the server either overflows a fixed buffer or exhausts memory trying to store it.

**Why it happens:** Developers allocate fixed-size buffers for convenience (`char uri[4096]`) or use unbounded dynamic allocation without limits. Both are wrong. Fixed buffers overflow; unbounded allocation enables DoS.

**Consequences:** Stack/heap corruption leading to remote code execution (fixed buffers). Memory exhaustion DoS (unbounded allocation). cserve already has this: proxy request/response use fixed stack buffers.

**Prevention:**
1. Define maximum sizes: URI (8KB), single header (8KB), total headers (32KB), request body (configurable, default 1MB)
2. Reject requests exceeding limits with 414 URI Too Long or 431 Request Header Fields Too Large
3. Never use stack buffers for request data -- always heap-allocate with size tracking
4. Track total bytes received per request; abort if limits exceeded before parsing completes
5. Use `size_t` for all length calculations, never `int` (cserve has this bug in response.h)

**Detection:** Send requests with progressively larger URIs, headers, and bodies. Monitor for crashes, memory growth, and proper 4xx responses.

**Real-world CVEs:**
- **CVE-2021-23017 (nginx):** Off-by-one in DNS resolver allowed 1-byte heap overwrite, leading to RCE. Demonstrates how even tiny buffer errors in C servers become critical.
- **CVE-2022-41741/41742 (nginx):** Memory corruption in mp4 module via crafted mp4 files. Buffer size miscalculation pattern.
- **CVE-2013-2028 (nginx):** Stack buffer overflow in chunked transfer encoding handling. Integer sign error caused negative size to be treated as large positive.

**Phase mapping:** Fix in Phase 0/1 (security fixes). cserve's proxy buffer overflows are already identified in CONCERNS.md.

---

### Pitfall 3: Path Traversal in Static File Serving

**What goes wrong:** Attacker requests `GET /../../etc/shadow` or uses URL-encoded variants (`%2e%2e%2f`), null bytes (`%00`), or double-encoding to escape the document root.

**Why it happens:** Naive implementations concatenate the URI onto the base directory and serve whatever file the OS resolves. Even `realpath()` checks can be bypassed if the check happens before URL decoding, or if symlinks are followed outside the root.

**Consequences:** Arbitrary file read. In the worst case, reading `/etc/shadow`, private keys, or application secrets. cserve currently has this vulnerability.

**Prevention:**
1. URL-decode the path first, then normalize (collapse `//`, resolve `.` and `..`)
2. Call `realpath()` on the final concatenated path
3. Verify the resolved path starts with the resolved document root (using `realpath()` on root too)
4. Reject paths containing null bytes after decoding
5. Consider whether to follow symlinks at all (nginx has `disable_symlinks` directive for a reason)
6. Reject paths with `..` segments entirely as a defense-in-depth measure

**Detection:** Test with `curl --path-as-is`, URL-encoded traversal sequences, null bytes, overly long paths.

**Real-world CVEs:**
- **CVE-2021-41773 / CVE-2021-42013 (Apache httpd 2.4.49/2.4.50):** Path traversal via `%2e` encoding in path normalization. The first fix was incomplete, leading to a second CVE. Demonstrates how easy it is to get path normalization wrong.
- **CVE-2018-16845 (nginx):** While a different vector (mp4 module), shows how file-handling code in C servers is a persistent attack surface.

**Phase mapping:** Fix immediately in Phase 1 (security). Already identified in CONCERNS.md. Must be bulletproof before any deployment.

---

### Pitfall 4: Use-After-Free in Connection Lifecycle

**What goes wrong:** Connection structures are freed while still referenced -- by the event loop, by pending I/O operations, or by parser state that holds pointers into the connection's read buffer. cserve already has this with parser dangling pointers.

**Why it happens:** In event-driven servers, a connection can be referenced from multiple places: the epoll set, the connection pool, parser state, pending write buffers. Freeing from one path without cleaning up all references creates dangling pointers.

**Consequences:** Heap corruption, crashes, potential RCE. Especially dangerous because a new connection may reuse the freed memory, causing cross-connection data leakage.

**Prevention:**
1. Single ownership model: one component owns the connection struct's lifetime
2. After closing a connection, immediately `epoll_ctl(EPOLL_CTL_DEL)` before `close(fd)` and before freeing the struct
3. Set the fd to -1 after closing (sentinel value, not 0 -- cserve gets this wrong)
4. Parser must copy data out of the read buffer, not store pointers into it (cserve's critical bug)
5. Use a "deferred free" pattern: mark connections for cleanup, free them at a safe point in the event loop (not inside event handlers)
6. Never realloc the read buffer while parser state holds pointers into it

**Detection:** Run under AddressSanitizer (`-fsanitize=address`). Stress test with rapid connect/disconnect cycles. Test keep-alive with multiple requests per connection.

**Real-world CVEs:**
- **CVE-2022-41742 (nginx):** Use-after-free in mp4 module allowing memory disclosure. Classic C lifecycle management failure.
- **CVE-2014-0133 (nginx):** Use-after-free in SPDY. Connection freed while handler still processing it.
- **CVE-2024-24989/24990 (nginx):** Null pointer dereference and use-after-free in HTTP/3 QUIC handling.

**Phase mapping:** Fix parser dangling pointers in Phase 1 (security). Implement proper connection lifecycle in Phase 2 (architecture restructuring). Fix fd sentinel value immediately.

---

### Pitfall 5: Signal Handler Corruption

**What goes wrong:** Signal handler calls `malloc()`, `free()`, `printf()`, `exit()`, or any function that takes a lock. If the signal interrupts a thread holding the same lock, instant deadlock. If it interrupts `malloc`, heap corruption.

**Why it happens:** Developers write signal handlers like regular functions. The C standard and POSIX restrict what's safe to call from signal context, but this is non-obvious and easy to violate. cserve does this.

**Consequences:** Deadlock on shutdown (server hangs instead of stopping). Heap corruption if signal interrupts `malloc/free`. Double-free if signal handler frees resources the main code is also freeing.

**Prevention:**
1. Signal handler should ONLY set a `volatile sig_atomic_t` or `atomic_int` flag, and optionally `write()` a single byte to a self-pipe
2. All cleanup happens in the main event loop after detecting the flag
3. Self-pipe trick: `write(pipe_fd, "x", 1)` in handler, `read()` in epoll -- integrates signal handling into the event loop cleanly
4. Use `signalfd()` on Linux to receive signals as file descriptors (can add to epoll directly)
5. Never call `exit()` from a signal handler; let the main loop terminate cleanly

**Detection:** Run under ThreadSanitizer. Test `kill -TERM` during high load. Test rapid repeated signals.

**Real-world incidents:**
- This is a class of bug that rarely gets CVEs because it manifests as "server occasionally hangs on shutdown" rather than a security exploit. But it causes operational incidents: servers that won't stop, requiring `kill -9`, leaving resources (sockets, files) in unclean state.
- nginx uses the `ngx_signal_handler()` approach: sets a global flag, the event loop checks it. No allocations, no locks.

**Phase mapping:** Fix in Phase 1 (security/safety). Already identified in CONCERNS.md. Use `signalfd()` for clean integration with epoll.

---

### Pitfall 6: Incomplete TLS Integration

**What goes wrong:** TLS is bolted on as a thin wrapper around `read()`/`write()`, but TLS has fundamentally different I/O semantics. `SSL_read()` may need to write (renegotiation), `SSL_write()` may need to read. A single `SSL_read()` call may return `SSL_ERROR_WANT_WRITE`, meaning the fd needs to be watched for writability, not readability.

**Why it happens:** Developers assume TLS is a transparent layer. It isn't. The TLS state machine has its own buffering, its own error conditions, and its own retry semantics that interact non-trivially with non-blocking I/O and epoll.

**Consequences:** Connections hang, data corruption, TLS handshake failures under load, security vulnerabilities from incomplete certificate validation.

**Prevention:**
1. Abstract I/O behind a layer that handles `SSL_ERROR_WANT_READ` and `SSL_ERROR_WANT_WRITE` by modifying epoll interest appropriately
2. Never assume `SSL_read()` returns data on the first call -- it may need multiple round trips for handshake
3. Handle `SSL_ERROR_WANT_READ` after `SSL_write()` (and vice versa) by changing epoll flags
4. Disable renegotiation (TLS 1.3 removes it, but TLS 1.2 clients may attempt it)
5. Use `SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1)` -- only allow TLS 1.2+
6. Call `SSL_shutdown()` properly (it's a two-phase process)
7. Set SNI callback for virtual host support

**Detection:** Test with `openssl s_client`, test renegotiation attempts, test with slow clients (byte-at-a-time), test certificate chain validation.

**Real-world CVEs:**
- **CVE-2014-0160 (Heartbleed, OpenSSL):** Not a server bug per se, but shows why staying current with OpenSSL patches is critical. Using OpenSSL means inheriting its vulnerability surface.
- **CVE-2014-0224 (OpenSSL):** CCS injection allowing MITM. Servers had to update even though the bug was in the library.
- **CVE-2021-3449 (OpenSSL):** NULL pointer dereference via crafted renegotiation ClientHello. Server crash from a single malicious TLS handshake.

**Phase mapping:** TLS is a later phase (after core HTTP correctness). Design the I/O abstraction layer during architecture restructuring so TLS can be added cleanly. Do NOT try to add TLS to the current monolithic code.

---

### Pitfall 7: Reverse Proxy Request/Response Desynchronization

**What goes wrong:** The proxy reads a response from the backend but misidentifies where it ends. It sends partial data to the client, or combines two backend responses into one client response, or holds the connection waiting for data that already arrived.

**Why it happens:** HTTP response framing is complex. The proxy must correctly handle: `Content-Length` responses, chunked responses, responses with no body (204, 304, HEAD responses), responses terminated by connection close, and `Transfer-Encoding` it doesn't understand.

**Consequences:** Data leakage between requests (response from user A's request sent to user B). Hung connections. Protocol desynchronization causing cascading failures.

**Prevention:**
1. Implement response body reading according to RFC 7230 S3.3.3 message length rules in order:
   - HEAD responses and 1xx/204/304 have no body
   - Transfer-Encoding: chunked overrides Content-Length
   - Content-Length defines exact body length
   - If neither, read until connection close (only valid for responses, not requests)
2. Validate Content-Length against actual bytes received
3. Forward chunked responses as chunked (don't buffer entire response to calculate Content-Length)
4. Implement timeouts on backend reads (don't hang forever waiting for a response)
5. Handle backend connection reset mid-response gracefully (502 Bad Gateway)

**Detection:** Test with backends that send chunked responses, large responses, empty bodies, slow responses (drip-feed). Test connection close mid-response.

**Real-world CVEs:**
- **CVE-2021-22901 (curl):** Use-after-free in TLS session handling when connection is reused. Proxy connection reuse is a persistent source of bugs.
- **CVE-2023-25690 (Apache httpd):** HTTP request smuggling via mod_proxy when `RewriteRule` and `ProxyPassMatch` interact. Shows how proxy header rewriting creates smuggling vectors.
- **CVE-2021-22959/22960 (Node.js/llhttp):** HTTP request smuggling in the HTTP parser used by Node.js. Demonstrates that even modern, well-tested parsers get framing wrong.

**Phase mapping:** Implement during reverse proxy enhancement phase. Requires correct HTTP parsing to be in place first. Must be tested with real backends exhibiting various response patterns.

---

## Moderate Pitfalls

---

### Pitfall 8: epoll Edge-Triggered Starvation and Missed Events

**What goes wrong:** With edge-triggered epoll (`EPOLLET`), you only get notified when the state changes. If you don't drain the socket completely on each notification, remaining data sits unread and no new notification comes. The connection appears hung.

**Prevention:**
1. With `EPOLLET`, always read in a loop until `EAGAIN` -- never read just once
2. Consider using level-triggered mode (default) unless you have measured a performance need for edge-triggered. Level-triggered is simpler and more forgiving.
3. If using edge-triggered, handle the case where `accept()` has multiple pending connections: call `accept()` in a loop until `EAGAIN`
4. Always check return values of `epoll_ctl()` -- silent failures mean the fd is not monitored
5. After `epoll_ctl(EPOLL_CTL_DEL)` and `close(fd)`, never use the fd again. If another thread opens a new fd with the same number, you may be modifying the wrong connection's epoll state.

**Detection:** Load test with many simultaneous connections. Some connections will appear to hang if edge-triggered draining is incomplete.

**Real-world context:** nginx uses edge-triggered epoll and gets this right, but it took years of refinement. For a new project, level-triggered is safer unless you have proven performance needs.

**Phase mapping:** Review during architecture restructuring. The current cserve uses `EPOLLIN | EPOLLOUT` (appears level-triggered) but doesn't handle `EAGAIN` on send correctly (CONCERNS.md: EAGAIN handling incomplete).

---

### Pitfall 9: Slowloris and Slow-Read DoS

**What goes wrong:** Attacker opens connections but sends data extremely slowly (one byte per second for headers, or reads responses one byte per second). Each connection ties up a slot in the connection pool. With 1000 max connections, 1000 slow clients exhaust the server.

**Prevention:**
1. Implement per-connection timeouts: header read timeout (e.g., 60s), body read timeout, response send timeout
2. Track when each connection last made progress; close idle connections
3. Use `timerfd` or a timer wheel integrated with epoll to enforce timeouts efficiently
4. Limit maximum time for receiving complete headers (not just idle time -- total elapsed time)
5. Consider minimum data rate enforcement for request bodies
6. The connection limit (MAX_CONNECTIONS) should be configurable, and reaching it should log a warning, not silently drop connections

**Detection:** Use `slowhttptest` or `slowloris.py`. Monitor connection count and response times under attack.

**Real-world context:** This is the attack that brought attention to connection-limit DoS. Apache was historically vulnerable because of its process-per-connection model. Event-driven servers (nginx, cserve) are more resistant but still vulnerable without timeouts.

**Phase mapping:** Implement timeouts during the connection management phase. Requires timer integration with the event loop, which should be designed during architecture restructuring.

---

### Pitfall 10: Incorrect Keep-Alive Handling

**What goes wrong:** The server mishandles persistent connections: not resetting state between requests, misinterpreting the `Connection` header, allowing unlimited requests on a single connection (resource exhaustion), or failing to handle pipelined requests.

**Prevention:**
1. After sending a response, fully reset parser state before reading the next request
2. For HTTP/1.1, connections are keep-alive by default; only close if `Connection: close` is sent
3. For HTTP/1.0, connections close by default; only keep alive if `Connection: keep-alive` is sent
4. Set a maximum requests-per-connection limit (nginx defaults to 1000)
5. Handle pipelined requests: if the read buffer contains data beyond the current request boundary, parse the next request immediately rather than waiting for new data
6. Send `Connection: close` in the final response if the server wants to close the connection
7. Case-insensitive header comparison is mandatory (cserve bug: uses `strncmp`)

**Detection:** Test sending multiple requests on a single TCP connection. Test mixing HTTP/1.0 and HTTP/1.1. Test sending `Connection: close` and `Connection: keep-alive`.

**Real-world context:** cserve's `reset_connection()` is incomplete (CONCERNS.md) and the parser stores pointers into the read buffer. This means keep-alive is currently broken and dangerous -- the second request on a connection may read corrupted state.

**Phase mapping:** Fix parser dangling pointers first (Phase 1), then implement proper connection reset, then enable keep-alive. Do NOT enable keep-alive until the parser copies data.

---

### Pitfall 11: Integer Overflow in Content-Length and Size Calculations

**What goes wrong:** `Content-Length` is parsed with `atoi()` (returns `int`, max ~2GB) or stored in `int` variables. Attacker sends `Content-Length: 4294967297` which wraps to 1 in a 32-bit integer, causing the server to read far less data than the client sends, desynchronizing the connection.

**Prevention:**
1. Parse `Content-Length` with `strtoull()` and validate the range
2. Store all sizes as `size_t` or `uint64_t`, never `int` or `unsigned int`
3. Check for overflow before arithmetic: `if (a + b < a)` indicates overflow
4. cserve already has `body_length` as `int` in `response.h` -- change to `size_t`
5. Reject Content-Length values that exceed a configured maximum (prevents memory exhaustion too)
6. Reject negative Content-Length values (they parse as large positive values with unsigned types, or cause sign confusion)

**Detection:** Send requests with `Content-Length: 0`, `Content-Length: -1`, `Content-Length: 999999999999`, duplicate Content-Length headers with different values.

**Real-world CVEs:**
- **CVE-2013-2028 (nginx):** Integer sign error in chunked transfer encoding. `ssize_t` value was treated as unsigned, causing a stack buffer overflow. This is the exact type confusion pitfall.
- **CVE-2015-3183 (Apache):** Chunk size parsing allowed values that bypassed size limits through integer truncation.

**Phase mapping:** Fix `atoi()` usage in Phase 1 (security). Fix type mismatches throughout. Enforce size limits when implementing body parsing.

---

### Pitfall 12: Proxy Header Injection

**What goes wrong:** When forwarding requests to backends, the proxy passes through headers verbatim. An attacker injects headers like `X-Forwarded-For` to spoof their IP, or `Host` to confuse the backend's virtual host routing.

**Prevention:**
1. Strip and re-add `X-Forwarded-For`, `X-Real-IP` -- never trust client-supplied values for these headers
2. Set `X-Forwarded-Proto` based on the actual connection (TLS or not), not from the client
3. Sanitize the `Host` header before forwarding (or replace it with the backend's expected host)
4. Strip hop-by-hop headers before forwarding: `Connection`, `Keep-Alive`, `Transfer-Encoding`, `TE`, `Trailer`, `Upgrade`, `Proxy-Authorization`, `Proxy-Authenticate`
5. Consider stripping `X-Forwarded-Host` and `X-Forwarded-Port` if they come from the client
6. Validate that header values don't contain `\r\n` (CRLF injection into response headers)

**Detection:** Send requests with spoofed `X-Forwarded-For` and verify the backend receives the correct client IP.

**Phase mapping:** Implement during reverse proxy phase. Design the header rewriting policy as part of proxy configuration.

---

## Minor Pitfalls

---

### Pitfall 13: File Descriptor Exhaustion

**What goes wrong:** Each connection uses an fd. Proxy connections to backends use additional fds. Log files use fds. Under load, the process hits the `ulimit -n` limit (often 1024 by default) and `accept()`, `socket()`, or `open()` fail with `EMFILE`.

**Prevention:**
1. Set `RLIMIT_NOFILE` at startup or document the required ulimit in the systemd unit file
2. Monitor fd count and log warnings when approaching the limit
3. When `accept()` returns `EMFILE`, have a strategy: close idle connections, or keep a "reserve" fd that can be closed temporarily to accept and immediately reject new connections
4. Close proxy backend connections promptly after use (or implement connection pooling with limits)

**Phase mapping:** Address during production hardening. systemd unit file should set `LimitNOFILE`.

---

### Pitfall 14: Improper MIME Type Detection

**What goes wrong:** Server sends wrong `Content-Type` for static files, causing browsers to misinterpret content. Worse, serving user-uploaded files with `text/html` enables stored XSS.

**Prevention:**
1. Use file extension mapping, not content sniffing (content sniffing is a security risk)
2. Send `X-Content-Type-Options: nosniff` header
3. Default to `application/octet-stream` for unknown extensions
4. For user-controlled content paths, be restrictive with MIME types

**Phase mapping:** Implement during static file serving improvement phase.

---

### Pitfall 15: Missing or Incorrect Date Header

**What goes wrong:** RFC 7231 S7.1.1.2 requires origin servers to send a `Date` header with every response (with minor exceptions). Missing `Date` breaks caching proxies and some HTTP clients.

**Prevention:**
1. Generate `Date` header in IMF-fixdate format: `Date: Sun, 06 Nov 1994 08:49:37 GMT`
2. Cache the formatted date string and update it once per second (not per request)
3. Use `gmtime_r()` (not `gmtime()`) for thread safety even in single-threaded code (future-proofing)

**Phase mapping:** Implement during HTTP/1.1 compliance phase. Low effort, high compliance value.

---

### Pitfall 16: Memory Allocator Fragmentation Under Long-Running Load

**What goes wrong:** After days/weeks of running, the server's RSS grows even without leaks. Frequent small allocations and frees for request/response buffers fragment the heap. `malloc()` returns memory from the OS but `free()` doesn't return it.

**Prevention:**
1. Use arena/pool allocators for per-request data -- allocate a block, bump-allocate within it, free the entire block when the request completes
2. Pre-allocate connection structures in a pool rather than malloc/free per connection
3. Use fixed-size buffers where sizes are predictable (header buffer, etc.) and only dynamically allocate for oversized requests
4. Monitor RSS in production; compare with valgrind massif profiles

**Phase mapping:** Consider during architecture restructuring. Not critical for correctness, but important for production stability.

---

### Pitfall 17: DNS Resolution Blocking the Event Loop

**What goes wrong:** When the reverse proxy needs to connect to a backend by hostname, `getaddrinfo()` blocks. In a single-threaded event loop, this blocks ALL connections, potentially for seconds (DNS timeout is typically 5s with 2 retries = 15s worst case).

**Prevention:**
1. Resolve backend hostnames at config load time, not per-request
2. Cache resolved addresses with TTL-based refresh
3. If dynamic resolution is needed, use a non-blocking DNS resolver (c-ares) or resolve in a thread pool
4. Since cserve has zero external dependencies, consider resolving at startup only and requiring IP addresses in config (simplest approach)

**Phase mapping:** Address during reverse proxy implementation. Design decision: accept the limitation of IP-only backend config, or add async DNS.

---

## Phase-Specific Warnings

| Phase Topic | Likely Pitfall | Mitigation |
|-------------|---------------|------------|
| Security fixes (Phase 1) | Path traversal fix is incomplete (encoding bypass) | Test with URL-encoded, double-encoded, and null-byte variants. Use `realpath()` after full decode. Consider rejecting `..` entirely. |
| Security fixes (Phase 1) | Buffer overflow fix introduces new bugs | Replace stack buffers with heap allocation. Use AddressSanitizer in CI. |
| Parser rewrite (Phase 1-2) | New parser introduces request smuggling | Fuzz test the parser with AFL/libFuzzer. Test against HTTP compliance suites. |
| Architecture restructure (Phase 2) | Refactoring breaks working functionality | Write integration tests BEFORE refactoring. Run them after each change. |
| Keep-alive (Phase 2-3) | Connection state leaks between requests | Full connection reset. Copy-based parser. Test multi-request sequences. |
| Chunked encoding (Phase 3) | Integer overflow in chunk size parsing | Parse with `strtoul()`, validate range, reject non-hex characters. |
| TLS integration (Phase 4) | `SSL_ERROR_WANT_READ/WRITE` not handled | Abstract I/O layer that translates SSL errors to epoll flag changes. |
| TLS integration (Phase 4) | Blocking on `SSL_do_handshake()` | Handshake must be non-blocking, driven by epoll events. |
| Reverse proxy (Phase 4-5) | Backend timeout causes event loop stall | Non-blocking connect to backends. Timeout via timerfd. |
| Reverse proxy (Phase 4-5) | Response desynchronization | Implement all RFC 7230 S3.3.3 message length rules. |
| Connection pooling (Phase 5) | Pool returns connection with stale data in buffer | Drain and validate pooled connections before reuse. |
| Load balancing (Phase 5) | Health check blocks event loop | Non-blocking health checks integrated with epoll. |
| Production hardening (Phase 6) | File descriptor exhaustion under load | Set ulimit, monitor fd count, handle EMFILE gracefully. |
| Packaging (Phase 6) | Config file path assumptions break across distros | Use compile-time defaults with runtime override. |

## cserve-Specific Pitfall Map

These pitfalls from CONCERNS.md map directly to the domain pitfalls above:

| CONCERNS.md Issue | Domain Pitfall | Priority |
|---|---|---|
| Path traversal vulnerability | Pitfall 3 (Path Traversal) | Fix first |
| Proxy request/response buffer overflow | Pitfall 2 (Buffer Overflow) | Fix first |
| Parser dangling pointers (use-after-free) | Pitfall 4 (Use-After-Free) | Fix first |
| Signal handler unsafe calls | Pitfall 5 (Signal Handler Corruption) | Fix first |
| EAGAIN handling incomplete | Pitfall 8 (epoll Event Handling) | Fix in restructure |
| Case-sensitive header comparison | Pitfall 10 (Keep-Alive) / Pitfall 1 (Smuggling) | Fix in compliance |
| body_length as int | Pitfall 11 (Integer Overflow) | Fix in compliance |
| atoi() for port parsing | Pitfall 11 (Integer Overflow) | Fix in Phase 1 |
| reset_connection() incomplete | Pitfall 10 (Keep-Alive Handling) | Fix before enabling keep-alive |
| Double close of client fd | Pitfall 4 (Use-After-Free / Lifecycle) | Fix in Phase 1 |
| Socket sentinel value 0 vs -1 | Pitfall 4 (Use-After-Free / Lifecycle) | Fix in Phase 1 |
| No unit tests | ALL pitfalls (no safety net) | Start in Phase 1, expand continuously |

## Sources

- RFC 7230 (HTTP/1.1 Message Syntax and Routing) -- especially Section 3.3.3 (Message Body Length)
- RFC 7231 (HTTP/1.1 Semantics and Content) -- Date header requirements
- CVE database entries for nginx, Apache httpd, lighttpd, curl (referenced inline)
- nginx source code architecture (signal handling, event loop, connection lifecycle patterns)
- POSIX signal safety documentation (async-signal-safe function list)
- OpenSSL documentation on non-blocking I/O with `SSL_ERROR_WANT_READ/WRITE`
- cserve CONCERNS.md (30+ identified issues mapped to domain pitfalls)

**Confidence note:** All pitfalls described are well-documented in the C web server domain. The CVE references are from training data (up to early 2025) and may have been supplemented by later advisories. The core patterns (buffer overflows, use-after-free, request smuggling, path traversal) are timeless in C network programming and have HIGH confidence regardless of specific CVE numbers.

---

*Pitfalls audit: 2026-03-17*
