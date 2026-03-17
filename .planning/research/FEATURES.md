# Feature Landscape

**Domain:** HTTP/1.1 web server and reverse proxy (C, Linux, epoll)
**Researched:** 2026-03-17
**Confidence:** HIGH for RFC requirements (stable specs from 2014, well-known), MEDIUM for competitive feature landscape (based on training data, no web verification available)

## Table Stakes

Features users expect. Missing = product is not a compliant HTTP/1.1 server.

### RFC 7230: Message Syntax and Routing (MUST-level)

| Feature | Why Expected | Complexity | RFC Section | Current State |
|---------|--------------|------------|-------------|---------------|
| Request-line parsing (method SP request-target SP HTTP-version CRLF) | Core protocol | Low | 3.1.1 | Exists, buggy |
| Status-line generation (HTTP-version SP status-code SP reason-phrase CRLF) | Core protocol | Low | 3.1.2 | Exists |
| Header field parsing (field-name ":" OWS field-value OWS CRLF) | Core protocol | Med | 3.2 | Exists, needs OWS handling |
| Host header requirement: reject HTTP/1.1 requests missing Host with 400 | Mandatory per spec | Low | 5.4 | Missing |
| Host header: reject requests with multiple Host headers with 400 | Mandatory per spec | Low | 5.4 | Missing |
| Transfer-Encoding: chunked decoding (receiving chunked requests) | Required for HTTP/1.1 | High | 3.3.1, 4.1 | Missing |
| Transfer-Encoding: chunked encoding (sending chunked responses) | Required for streaming/unknown-length | High | 3.3.1, 4.1 | Missing |
| Content-Length handling: parse, validate, use for message framing | Core framing mechanism | Med | 3.3.2 | Partial |
| Reject requests with both Transfer-Encoding and Content-Length (smuggling) | Security requirement | Low | 3.3.3 | Missing |
| Persistent connections (default in HTTP/1.1) | Performance baseline | Med | 6.3 | Partial |
| Connection: close handling (honor client request to close) | Required | Low | 6.1 | Partial |
| Message body framing: determine message length correctly | Correctness | Med | 3.3.3 | Partial |
| URI parsing: absolute-form, origin-form, authority-form, asterisk-form | Proxy and server roles | Med | 5.3 | Partial (origin-form only) |
| Reject request-targets with whitespace in them | Security (request smuggling) | Low | 3.5 | Missing |
| Handle obs-fold (line folding in headers): reject or unfold | Spec compliance | Low | 3.2.4 | Missing |
| Limit request header sizes (SHOULD, but de facto required for DoS) | Security | Low | 3.2.5 | Missing |
| Reject HTTP/1.1 requests to proxy without absolute-URI form | Proxy correctness | Low | 5.3.2 | Missing |

### RFC 7230: Message Syntax and Routing (SHOULD-level, treat as required for strict compliance)

| Feature | Why Expected | Complexity | RFC Section |
|---------|--------------|------------|-------------|
| Trailer field support in chunked encoding | Completeness | Med | 4.1.2 |
| TE header processing | Completeness | Low | 4.3 |
| Via header addition (proxy MUST add Via) | Proxy compliance | Low | 5.7.1 |
| Max-Forwards for TRACE/OPTIONS | Prevent loops | Low | 5.7.2 |
| Connection header: remove hop-by-hop headers when forwarding | Proxy correctness | Med | 6.1 |
| Upgrade header handling (101 Switching Protocols or ignore) | Completeness | Med | 6.7 |

### RFC 7231: Semantics and Content

| Feature | Why Expected | Complexity | RFC Section | Current State |
|---------|--------------|------------|-------------|---------------|
| GET method: return representation of target resource | Core functionality | Low | 4.3.1 | Exists |
| HEAD method: identical to GET but no body | Required (MUST support) | Low | 4.3.2 | Missing |
| POST method: process enclosed representation | Core functionality | Low | 4.3.3 | Exists (proxy only) |
| OPTIONS method: describe communication options | Required for compliance | Low | 4.3.7 | Missing |
| Date header: origin server MUST send in all responses (except 1xx, 5xx) | Mandatory | Low | 7.1.1.2 | Missing |
| Content-Type header on responses with body | Expected | Low | 3.1.1.5 | Exists |
| Content-Length header on responses (when known) | Expected | Low | 3.3.2 (7230) | Exists |
| Server header (SHOULD) | Standard practice | Low | 7.4.2 | Missing |
| 400 Bad Request for malformed requests | Error handling | Low | 6.5.1 | Partial |
| 404 Not Found | Error handling | Low | 6.5.4 | Exists |
| 405 Method Not Allowed (with Allow header) | Required response | Low | 6.5.5 | Missing |
| 413 Payload Too Large | Size limits | Low | 6.5.11 | Missing |
| 414 URI Too Long | Size limits | Low | 6.5.12 | Missing |
| 500 Internal Server Error | Error handling | Low | 6.6.1 | Partial |
| 501 Not Implemented (for unrecognized methods) | Required | Low | 6.6.2 | Missing |
| 502 Bad Gateway (proxy: bad upstream response) | Proxy error | Low | 6.6.3 | Exists |
| 503 Service Unavailable | Proxy error | Low | 6.6.4 | Missing |
| 504 Gateway Timeout | Proxy error | Low | 6.6.5 | Missing |
| 505 HTTP Version Not Supported | Version handling | Low | 6.6.6 | Missing |
| Allow header in 405 responses | Mandatory | Low | 7.4.1 | Missing |
| Location header in 201 and 3xx responses | Required for redirects | Low | 7.1.2 | Missing |
| Content negotiation (Accept, Accept-Charset, Accept-Encoding, Accept-Language) | SHOULD support | High | 5.3 | Missing |
| Expect: 100-continue handling (respond with 100 Continue or final status) | MUST handle | Med | 5.1.1 | Missing |

### RFC 7232: Conditional Requests

| Feature | Why Expected | Complexity | RFC Section |
|---------|--------------|------------|-------------|
| Last-Modified header on static file responses | Standard for caching | Low | 2.2 |
| ETag header on static file responses (SHOULD) | Standard for caching | Med | 2.3 |
| If-Modified-Since support (conditional GET, return 304) | Essential for caching | Med | 3.3 |
| If-None-Match support (conditional GET with ETags, return 304) | Essential for caching | Med | 3.2 |
| If-Match support (precondition for PUT/DELETE) | Completeness | Med | 3.1 |
| If-Unmodified-Since support | Completeness | Med | 3.4 |
| 304 Not Modified response (no body, specific headers only) | Caching protocol | Low | 4.1 |
| 412 Precondition Failed response | Conditional semantics | Low | 4.2 |
| Precondition evaluation order | Correctness | Low | 6 |

### RFC 7233: Range Requests

| Feature | Why Expected | Complexity | RFC Section |
|---------|--------------|------------|-------------|
| Accept-Ranges header (bytes or none) | Indicate capability | Low | 2.3 |
| Range request handling (single byte range) | Required for media, downloads | Med | 2.1 |
| 206 Partial Content response | Range serving | Med | 4.1 |
| 416 Range Not Satisfiable response | Error case | Low | 4.4 |
| If-Range header support (combine conditional + range) | Correctness | Med | 3.2 |
| Multipart byte ranges (multipart/byteranges) | Full compliance | High | 4.1 |

### RFC 7234: Caching

| Feature | Why Expected | Complexity | RFC Section |
|---------|--------------|------------|-------------|
| Cache-Control header support in responses | Servers set policy | Low | 5.2.2 |
| Expires header support | Legacy but expected | Low | 5.3 |
| Age header (proxy MUST add when serving from cache) | Proxy caching | Med | 5.1 |

Note: cserve is a server/proxy, not a caching proxy for v1. Cache-Control/Expires are about *setting* headers on responses, not implementing a cache store.

### RFC 7235: Authentication

| Feature | Why Expected | Complexity | RFC Section |
|---------|--------------|------------|-------------|
| 401 Unauthorized with WWW-Authenticate header | Pass-through | Low | 3.1 |
| 407 Proxy Authentication Required | Proxy role | Low | 3.2 |
| Forward Authorization headers to backends | Proxy role | Low | 4.2 |
| Proxy-Authorization header handling | Proxy role | Low | 4.4 |

Note: cserve does not need to *implement* authentication schemes -- it needs to correctly *forward* auth headers and generate proper 401/407 responses when appropriate.

### Static File Serving (table stakes beyond RFC)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Serve files from configured document root | Core use case | Low | Exists |
| MIME type detection (Content-Type from file extension) | Usability | Med | Needs mime type map |
| Directory index (serve index.html for directory requests) | Universal expectation | Low | Missing |
| Path traversal prevention (sanitize ../ etc.) | Security critical | Med | Exists (buggy) |
| Symlink handling policy (follow or reject) | Security | Low | Missing |

### Reverse Proxy (table stakes beyond RFC)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Forward requests to configured backend | Core use case | Med | Exists |
| X-Forwarded-For header injection | Client IP transparency | Low | Missing |
| X-Forwarded-Proto header injection | Protocol transparency | Low | Missing |
| X-Forwarded-Host header injection | Host transparency | Low | Missing |
| Connection: close/keep-alive stripping on hop | RFC 7230 6.1 | Low | Missing |
| Hop-by-hop header removal when forwarding | RFC 7230 6.1 | Med | Missing |
| Backend connection timeout | Reliability | Med | Missing |
| Response relay (stream backend response to client) | Core proxy | Med | Exists |

### TLS (table stakes for v1 per project requirements)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| TLS 1.2/1.3 termination (HTTPS listener) | Modern web requirement | High | Missing, zero-dep constraint means raw OpenSSL or system TLS |
| Certificate + key file configuration | Deployment | Low | Missing |
| SNI support (multiple certs per IP) | Virtual hosting | Med | Missing |
| HTTP-to-HTTPS redirect | Common pattern | Low | Missing |

## Differentiators

Features that set cserve apart. Not expected in every server, but valued.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Strict RFC 7230-7235 compliance (every behavior cites section) | Correctness as brand; most servers cut corners | High | Project's core differentiator |
| Zero external dependencies | Minimal attack surface, simple auditing, easy packaging | N/A | Already a constraint |
| Valgrind-clean guarantee | Memory safety provable, not just claimed | Med | Ongoing discipline |
| Request smuggling resistance (strict parsing, reject ambiguity) | Security differentiator vs lenient servers | Med | Reject conflicting CL/TE, reject bad whitespace |
| Detailed error responses citing RFC section violated | Developer-friendly, unique | Low | Log/response includes RFC ref |
| Small binary size / minimal resource footprint | IoT, edge, embedded use cases | Low | Natural outcome of C + zero deps |
| Single-file configuration, no Lua/scripting layer | Simplicity vs nginx | Low | Already INI-based |
| Comprehensive HTTP compliance test suite | Proves claims, catches regressions | High | Build alongside features |

## Anti-Features

Features to explicitly NOT build for v1.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| HTTP/2 or HTTP/3 | Massive complexity, different framing model, v1 focus is HTTP/1.1 correctness | Reverse proxy behind HTTP/2-capable frontend if needed |
| CGI / FastCGI / SCGI | Dynamic content generation adds huge attack surface and complexity | Reverse proxy to app servers (the modern pattern) |
| Built-in caching proxy (cache store) | Significant complexity (invalidation, storage, freshness) | Set Cache-Control headers; use Varnish if caching needed |
| URL rewriting / regex routing | Scope creep, complex to get right, config complexity | Simple path-prefix routing for proxy; file paths for static |
| Lua / scripting extensions | Huge complexity, sandbox security issues | Defer to v2+ if ever |
| WebSocket proxying | Different protocol semantics, connection upgrade complexity | Defer to v2 |
| Access logging (structured, rotatable) | Needed for production but adds complexity to v1 | Defer to v2 (production-ready milestone) |
| Rate limiting / throttling | Application-layer concern, complex to implement correctly | Defer to v2 |
| Graceful reload (SIGHUP config reload) | Complex signal handling, connection draining | Defer to v2 |
| gzip / deflate content encoding | Performance optimization, not correctness | Defer to v2 (Accept-Encoding negotiation is complex) |
| HTTP authentication implementation | Auth schemes (Basic, Digest, Bearer) belong in app layer | Forward auth headers, return 401/407 status codes |
| Windows / macOS portability | Epoll is Linux-only; kqueue/IOCP abstractions add complexity | Linux-only is a valid market position |
| Load balancing algorithms (round-robin, least-conn, etc.) | Complex state management, health checking | Single backend per route for v1; load balancing in v2 |
| Connection pooling to backends | Performance optimization | New connection per proxy request in v1; pool in v2 |

## Feature Dependencies

```
                    Request Parsing (RFC 7230 3.1-3.2)
                           |
              +------------+------------+
              |                         |
     Host Header Validation      Content-Length / Transfer-Encoding
     (RFC 7230 5.4)              (RFC 7230 3.3)
              |                         |
              v                         v
     Virtual Host Routing        Chunked Encoding (RFC 7230 4.1)
              |                         |
              v                         v
     Static File Serving         Request Body Handling
              |                         |
              +-----+-----+            |
              |     |     |            |
              v     v     v            v
          MIME   Dir    Range      Reverse Proxy Forwarding
          Types  Index  Requests        |
              |                    +----+----+
              v                    |         |
     Conditional Requests     X-Forwarded  Via Header
     (ETag, Last-Modified)    Headers      (RFC 7230 5.7.1)
     (RFC 7232)                    |
              |                    v
              v              Backend Timeout
     304 Not Modified             |
     206 Partial Content          v
                            Error Responses (502, 503, 504)

     Persistent Connections (RFC 7230 6.3)
              |
              v
     Connection: close handling (RFC 7230 6.1)
              |
              v
     Keep-alive timeout management

     TLS Termination
              |
              +--------+
              |        |
              v        v
         SNI Support   HTTP->HTTPS Redirect
              |
              v
         Virtual Hosting (multiple certs)
```

**Critical path:** Request parsing -> Host validation -> Content-Length/TE handling -> Persistent connections -> Static file serving -> Reverse proxy -> Conditional requests -> Range requests -> TLS

## RFC 7230-7235 Compliance Checklist

### MUST-level Requirements (non-negotiable for compliance claims)

**RFC 7230 - Message Syntax and Routing:**
- [ ] Parse request-line: method SP request-target SP HTTP-version CRLF (3.1.1)
- [ ] Generate status-line: HTTP-version SP status-code SP reason-phrase CRLF (3.1.2)
- [ ] Parse header fields: field-name ":" OWS field-value OWS CRLF (3.2)
- [ ] Header field names are case-insensitive (3.2)
- [ ] Reject requests with whitespace between header field-name and colon (3.2.4)
- [ ] Handle obs-fold in header values (reject or replace with SP) (3.2.4)
- [ ] Determine message body length per priority rules (3.3.3)
- [ ] Treat Transfer-Encoding as higher priority than Content-Length (3.3.3)
- [ ] Reject requests with both TE and CL (request smuggling prevention) (3.3.3)
- [ ] Decode chunked transfer coding when receiving (4.1)
- [ ] Parse chunk-size (hex), chunk-data, and final zero-length chunk (4.1)
- [ ] Reject chunk-ext that exceeds limits (4.1.1)
- [ ] Process trailers after final chunk (or discard) (4.1.2)
- [ ] As proxy: apply TE decoding and re-encode or use CL (4.1.3)
- [ ] Reject request-target containing whitespace (3.5, security)
- [ ] Require Host header in HTTP/1.1 requests; reject with 400 if missing (5.4)
- [ ] Reject requests with multiple Host headers with 400 (5.4)
- [ ] As proxy: add Via header to forwarded messages (5.7.1)
- [ ] As proxy: remove connection-specific headers before forwarding (6.1)
- [ ] Default to persistent connections in HTTP/1.1 (6.3)
- [ ] Close connection after sending/receiving "Connection: close" (6.1)
- [ ] Respond to unrecognized methods with 501 (if not forwarding as proxy)

**RFC 7231 - Semantics and Content:**
- [ ] Support GET method (4.3.1)
- [ ] Support HEAD method: same as GET but no response body (4.3.2)
- [ ] Respond with Date header on all responses (except 1xx, 5xx when clock unavailable) (7.1.1.2)
- [ ] Respond with Allow header in 405 responses (7.4.1)
- [ ] Handle Expect: 100-continue (respond with 100 or final status before reading body) (5.1.1)
- [ ] Return 405 Method Not Allowed when method recognized but not allowed for resource (6.5.5)
- [ ] Return 501 Not Implemented for unrecognized methods (6.6.2)

**RFC 7232 - Conditional Requests:**
- [ ] Follow precondition evaluation order: If-Match -> If-Unmodified-Since -> If-None-Match -> If-Modified-Since -> If-Range (6)
- [ ] Return 304 when conditional GET/HEAD succeeds (no body, limited headers) (4.1)
- [ ] Return 412 when precondition fails for unsafe methods (4.2)

**RFC 7233 - Range Requests:**
- [ ] Send Accept-Ranges header (indicate byte-range support or "none") (2.3)
- [ ] Handle Range header on GET requests (return 206 or ignore range) (3.1)
- [ ] Return 416 Range Not Satisfiable when range invalid (4.4)

**RFC 7234 - Caching:**
- [ ] As proxy: add Age header when forwarding a stored response (5.1)
- [ ] As proxy: add Warning header for stale responses (5.5) -- deprecated in later RFCs but part of 7234

**RFC 7235 - Authentication:**
- [ ] Include WWW-Authenticate header in 401 responses (3.1)
- [ ] Include Proxy-Authenticate header in 407 responses (3.2)
- [ ] Forward Authorization headers transparently as proxy (4.2)

### SHOULD-level Requirements (implement for strict compliance)

| Requirement | RFC Section | Priority |
|-------------|-------------|----------|
| Send Server header | 7231 7.4.2 | Low |
| Send Content-Length when known | 7230 3.3.2 | High (already done) |
| Support Content-Encoding (at minimum: identity) | 7231 3.1.2.2 | Medium |
| Close idle connections after timeout | 7230 6.3 | High |
| Limit request header total size | 7230 3.2.5 | High (DoS prevention) |
| Limit request-line length | 7230 3.1.1 | High (DoS prevention) |
| Return Retry-After with 503 responses | 7231 7.1.3 | Low |
| Strong ETag comparison for If-Match / If-None-Match | 7232 2.3.2 | Medium |
| Weak ETag comparison for If-Modified-Since fallback | 7232 2.3.2 | Low |
| Content-Location header on responses when applicable | 7231 3.1.4.2 | Low |

## MVP Recommendation

The project already declares v1 scope in PROJECT.md. Here is the recommended feature priority ordering within that scope:

### Phase 1: Correct HTTP/1.1 Parsing (foundation)

1. **Request-line parsing hardened** - reject malformed, handle all URI forms
2. **Header parsing hardened** - case-insensitive, OWS handling, obs-fold, limits
3. **Host header validation** - reject missing/duplicate, 400 response
4. **Content-Length message framing** - correct body length determination
5. **Reject TE+CL conflicts** - request smuggling prevention
6. **Required error status codes** - 400, 405, 413, 414, 501, 505 with proper headers

### Phase 2: Message Body and Connection Management

1. **Transfer-Encoding: chunked** - decode incoming, encode outgoing
2. **Persistent connections** - default keep-alive, Connection: close, idle timeout
3. **Expect: 100-continue** - respond before reading large bodies
4. **Date header** - on all responses

### Phase 3: Static File Serving (complete)

1. **MIME type detection** - file extension to Content-Type mapping
2. **Directory index** - index.html default
3. **Path traversal hardened** - realpath + chroot-like validation
4. **Last-Modified / ETag generation** - from stat() and inode/mtime/size
5. **Conditional requests** - If-Modified-Since, If-None-Match, 304 responses
6. **Range requests** - single byte ranges, 206 Partial Content

### Phase 4: Reverse Proxy (complete)

1. **Hop-by-hop header stripping** - Connection, TE, Transfer-Encoding, etc.
2. **Via header injection** - proxy identification per RFC 7230 5.7.1
3. **X-Forwarded-For/Proto/Host** - client transparency headers
4. **Backend timeouts** - connect and read timeouts
5. **502/503/504 proper responses** - with appropriate headers
6. **Auth header forwarding** - transparent Authorization pass-through

### Phase 5: TLS

1. **TLS termination** - OpenSSL integration (or system libssl)
2. **Certificate configuration** - cert + key file paths in config
3. **SNI support** - per-vhost certificates
4. **HTTP->HTTPS redirect** - optional per-listener

### Defer to v2:

- **Content-Encoding (gzip/deflate)** - performance optimization, complex
- **Load balancing** - multiple backends, health checks, algorithms
- **Connection pooling to backends** - keep-alive to upstreams
- **WebSocket proxy** - Upgrade handling
- **Access logging** - structured, rotatable
- **Rate limiting** - per-client/per-route
- **Graceful reload** - SIGHUP, connection draining
- **Multipart byte ranges** - rarely used, complex formatting
- **Full content negotiation** - Accept-Language, Accept-Charset, Vary

## Sources

- RFC 7230: Hypertext Transfer Protocol (HTTP/1.1): Message Syntax and Routing (June 2014) - https://www.rfc-editor.org/rfc/rfc7230
- RFC 7231: Hypertext Transfer Protocol (HTTP/1.1): Semantics and Content (June 2014) - https://www.rfc-editor.org/rfc/rfc7231
- RFC 7232: Hypertext Transfer Protocol (HTTP/1.1): Conditional Requests (June 2014) - https://www.rfc-editor.org/rfc/rfc7232
- RFC 7233: Hypertext Transfer Protocol (HTTP/1.1): Range Requests (June 2014) - https://www.rfc-editor.org/rfc/rfc7233
- RFC 7234: Hypertext Transfer Protocol (HTTP/1.1): Caching (June 2014) - https://www.rfc-editor.org/rfc/rfc7234
- RFC 7235: Hypertext Transfer Protocol (HTTP/1.1): Authentication (June 2014) - https://www.rfc-editor.org/rfc/rfc7235
- Confidence: HIGH - these are stable, well-known standards that have not changed since publication. Note RFC 9110-9114 (June 2022) consolidated and obsoleted 7230-7235 but the requirements are substantively the same; the project targets 7230-7235 explicitly.
