# cserve

## What This Is

A high-performance, epoll-based HTTP/1.1 web server and reverse proxy written in C, intended as an open-source alternative to nginx for the core use cases of static file serving and reverse proxying. Built for Linux systems with zero external dependencies.

## Core Value

Every HTTP behavior must be correct per RFC 7230-7235, memory-safe (valgrind clean), and written to professional C standards — correctness is the foundation everything else builds on.

## Requirements

### Validated

<!-- Inferred from existing codebase -->

- ✓ Epoll-based event loop with non-blocking I/O — existing
- ✓ TCP socket abstraction with SO_REUSEADDR — existing
- ✓ HTTP request parsing (method, URI, headers) — existing (needs cleanup)
- ✓ HTTP response construction and serialization — existing (needs cleanup)
- ✓ Static file serving from disk — existing (needs cleanup)
- ✓ Basic reverse proxy forwarding to backends — existing (needs cleanup)
- ✓ Connection state machine (established → processing → sending → closing) — existing
- ✓ INI-style configuration file parsing — existing
- ✓ Structured logging with timestamps — existing
- ✓ CMake build system with debug/release modes — existing

### Active

- [ ] Fix all security vulnerabilities (path traversal, buffer overflows, use-after-free)
- [ ] Fix all memory safety issues (valgrind clean)
- [ ] Fix signal handler safety violations
- [ ] Restructure monolithic code into clean, testable modules
- [ ] Full HTTP/1.1 compliance per RFC 7230-7235 (chunked encoding, persistent connections, all required headers, proper status codes)
- [ ] Built-in TLS termination (HTTPS)
- [ ] Full reverse proxy: load balancing, health checks, connection pooling, header rewriting, upstream keepalive
- [ ] Declarative configuration file (virtual hosts, proxy backends, TLS certs)
- [ ] apt/dnf system packages with systemd unit, config in /etc, logs in /var/log
- [ ] Comprehensive test suite

### Out of Scope

- HTTP/2 or HTTP/3 support — v1 focuses on getting HTTP/1.1 right
- Windows/macOS support — Linux-only (epoll dependency)
- Dynamic content generation (CGI/FastCGI) — reverse proxy to application servers instead
- Web application firewall features — out of scope for a web server
- GUI or web-based admin interface — config file driven
- Graceful reload, access logs, rate limiting — deferred to v2.0 (production-ready milestone)

## Context

- Existing codebase has ~30 identified concerns including critical security issues (path traversal, buffer overflows, signal handler safety, use-after-free in parser)
- Monolithic `server.c` (500+ lines) blocks testability and clean separation
- HTTP parsing uses zero-copy pointer-based approach (good concept, buggy implementation)
- Connection pool supports up to 1000 concurrent connections
- Single-threaded event-driven model — correct approach, needs proper implementation
- No external dependencies — self-contained C project, only system/POSIX APIs
- Codebase map available at `.planning/codebase/` with detailed analysis

## Constraints

- **Language**: C11 with GNU extensions (`_GNU_SOURCE` for memmem, asprintf, epoll)
- **Platform**: Linux only (epoll is Linux-specific)
- **Dependencies**: Zero external libraries — system/POSIX APIs only
- **Build**: CMake 3.20+ with GNU Make convenience wrapper
- **Invariants**:
  1. Memory safety — no leaks, no use-after-free, no buffer overflows, valgrind clean
  2. Spec compliance — every HTTP behavior must cite the RFC section it implements
  3. Professional C standards — clean architecture, proper error handling, industry best practices

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Fix-first strategy | Stabilize codebase before adding features — security and correctness debt compounds | — Pending |
| Strict RFC 7230-7235 compliance | Not a "practical subset" — full spec compliance is the differentiator | — Pending |
| Zero external dependencies | Self-contained, minimal attack surface, easier packaging | — Pending |
| INI-style config file | Existing config parser works; evolve format as needed for virtual hosts/proxy | — Pending |
| Linux-only (epoll) | Performance focus; cross-platform abstractions add complexity without value for target use case | — Pending |
| Built-in TLS for v1 | Users expect HTTPS; offloading adds deployment complexity | — Pending |

---
*Last updated: 2026-03-17 after initialization*
