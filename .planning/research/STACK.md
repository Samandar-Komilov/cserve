# Technology Stack

**Project:** cserve - HTTP/1.1 Web Server and Reverse Proxy
**Researched:** 2026-03-17
**Overall Confidence:** MEDIUM (web search unavailable; recommendations based on training data for a very stable domain -- C systems tooling changes slowly, but exact version numbers need verification)

## Recommended Stack

### Core Language and Build

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| C11 (GNU extensions) | C11 | Application language | Already in use. C11 is the right standard -- modern enough for `_Static_assert`, `_Noreturn`, anonymous structs/unions, but not C17/C23 which add little value and reduce compiler compatibility. GNU extensions needed for `memmem()`, `asprintf()`, `epoll`. | HIGH |
| CMake | 3.20+ | Build system | Already in use. 3.20+ provides `cmake_path()`, improved presets support, and `target_sources(FILE_SET)`. Well-established for C projects. | HIGH |
| GCC | 13+ | Primary compiler | GCC remains the default Linux C compiler. Version 13+ for better C11 conformance, improved static analysis (`-fanalyzer`), and better sanitizer support. | HIGH |
| Clang | 17+ | Secondary compiler, analysis | Build with both GCC and Clang. Clang provides superior diagnostics, `clang-tidy` integration, and different optimization characteristics that catch different bugs. | HIGH |

### TLS Library

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| OpenSSL | 3.x (3.2+) | TLS termination | **This is the key stack decision.** The project says "zero external dependencies" but also requires "Built-in TLS termination." Writing TLS from scratch is not feasible -- it would take years and produce an insecure result. OpenSSL 3.x is the correct choice: ubiquitous on Linux, well-audited, provider-based architecture for algorithm flexibility, and the library nginx/Apache/curl all use. Make it a build-time optional dependency via `cmake -DWITH_TLS=ON`. | HIGH |

**Critical note on "zero dependencies":** The project constraint of "zero external libraries" must be relaxed for TLS. There is no responsible way to implement TLS from scratch. The realistic interpretation is: zero dependencies for core HTTP functionality, OpenSSL as the single optional dependency for HTTPS. This is exactly how every professional C web server (nginx, lighttpd, HAProxy) handles it.

**Why not alternatives:**
- **LibreSSL:** Fork of OpenSSL with cleaner internals, but smaller ecosystem, less documentation, and some API incompatibilities. Only choose if targeting OpenBSD.
- **wolfSSL:** Smaller footprint, dual-licensed (GPLv2 or commercial). Good for embedded, but overkill licensing complexity for a server.
- **GnuTLS:** LGPL-licensed, different API paradigm. Less industry adoption than OpenSSL for web servers.
- **BearSSL:** Minimal and clean, but unmaintained (last release ~2020). Not suitable for production.
- **mbedTLS:** ARM-focused, good for embedded. Unnecessary for a Linux server targeting x86_64/ARM Linux.
- **Roll your own:** Irresponsible. TLS is the single most security-critical component. Use a well-audited library.

### Testing Framework

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| CMocka | 1.1.7+ | Unit testing with mocking | Use CMocka because the project needs mock support for testing network code. CMocka provides: `cmocka_unit_test()` test runners, `expect_*()` / `will_return()` mocking, `assert_*()` macros that don't `exit()` (uses `longjmp` for test isolation), memory leak detection, CTest/TAP output formats. Critically, it supports mocking via linker wrapping (`--wrap`) which is essential for testing socket/epoll code without real network I/O. | HIGH |
| CTest | (bundled) | Test orchestration | Already in use via CMake. CTest runs test binaries, supports parallel execution (`ctest -j`), labels, timeouts, and output-on-failure. Keep using it as the test runner. | HIGH |

**Why not alternatives:**
- **Check:** Good framework but heavier (uses `fork()` for test isolation). The fork-based isolation is overkill for this project and adds complexity on some CI systems.
- **Unity:** Popular in embedded, but lacks built-in mocking. Would need to pair with CMock (code-generated mocks from headers), which adds Ruby as a build dependency.
- **Custom framework (current):** The existing `ASSERT()`/`RUN()` macros exit on first failure, can't mock, and don't report which tests were skipped. Replace with CMocka.
- **Criterion:** Feature-rich but less widely adopted. Adds a heavier dependency for minimal benefit over CMocka.
- **minunit:** Too minimal -- same problems as the current custom framework.

### Static Analysis and Sanitizers

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| AddressSanitizer (ASan) | (compiler-bundled) | Memory error detection at runtime | Catches buffer overflows, use-after-free, double-free, stack overflow at runtime. Enable with `-fsanitize=address`. Run all tests with ASan enabled in CI. This is the single most valuable tool for this project given its existing memory safety issues. | HIGH |
| UndefinedBehaviorSanitizer (UBSan) | (compiler-bundled) | Undefined behavior detection | Catches signed integer overflow, null pointer dereference, misaligned access, type punning violations. Enable with `-fsanitize=undefined`. Combine with ASan: `-fsanitize=address,undefined`. | HIGH |
| Valgrind (memcheck) | 3.22+ | Heap analysis, leak detection | Catches memory leaks that sanitizers miss (ASan leak detection is less thorough). Run separately from ASan (they conflict). Use for pre-release validation: `valgrind --leak-check=full --error-exitcode=1`. | HIGH |
| cppcheck | 2.13+ | Static analysis | Catches null pointer dereference, uninitialized variables, buffer overflows, resource leaks at compile time without running code. Low false-positive rate. Use `--enable=all --error-exitcode=1 --suppress=missingIncludeSystem`. | HIGH |
| clang-tidy | 17+ | Lint and modernization | More opinionated than cppcheck. Catches style issues, CERT-C violations, and suggests fixes. Use checks: `bugprone-*,cert-*,clang-analyzer-*,performance-*,readability-*`. Integrates with `compile_commands.json` already exported by CMake. | HIGH |
| GCC `-fanalyzer` | GCC 13+ | Built-in static analysis | GCC's built-in interprocedural analyzer. Catches double-free, use-after-free, null dereference, file descriptor leaks across function boundaries. Free -- no extra tool needed. Add to CI builds. | MEDIUM |

**Recommended CMake integration:**

```cmake
# Sanitizer build type
option(ENABLE_SANITIZERS "Enable ASan + UBSan" OFF)
if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Static analysis target
find_program(CPPCHECK cppcheck)
if(CPPCHECK)
    set(CMAKE_C_CPPCHECK ${CPPCHECK}
        --enable=all
        --suppress=missingIncludeSystem
        --error-exitcode=1
        --inline-suppr
    )
endif()
```

### Packaging

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| CPack | (CMake-bundled) | Package generation orchestration | CPack is bundled with CMake and generates .deb, .rpm, and .tar.gz from a single configuration. Use `install()` directives in CMakeLists.txt to define what goes where, then CPack generates packages. This is the standard approach for CMake-based C projects. | HIGH |
| rpmbuild | (system) | RPM generation (Fedora/RHEL) | CPack's RPM generator calls rpmbuild under the hood. Provide a `.spec` file template for fine-grained control, or let CPack generate one. Target Fedora 39+/RHEL 9+. | HIGH |
| dpkg-deb | (system) | DEB generation (Debian/Ubuntu) | CPack's DEB generator handles this. Target Ubuntu 22.04+/Debian 12+. | HIGH |
| systemd | (system) | Service management | Ship a `.service` unit file. Install to `/lib/systemd/system/cserve.service`. Use `Type=notify` (requires `sd_notify()` from libsystemd, another justified optional dependency) or `Type=simple` to avoid the dependency. | HIGH |

**Recommended CPack configuration:**

```cmake
# Install directives
install(TARGETS cserve RUNTIME DESTINATION bin)
install(FILES config/cserve.conf DESTINATION /etc/cserve/)
install(FILES systemd/cserve.service DESTINATION lib/systemd/system/)
install(DIRECTORY DESTINATION /var/log/cserve)

# CPack
set(CPACK_PACKAGE_NAME "cserve")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HTTP/1.1 web server and reverse proxy")
set(CPACK_PACKAGE_CONTACT "maintainer@example.com")
set(CPACK_GENERATOR "DEB;RPM;TGZ")

# DEB-specific
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.35)")
set(CPACK_DEBIAN_PACKAGE_SECTION "httpd")

# RPM-specific
set(CPACK_RPM_PACKAGE_GROUP "System Environment/Daemons")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")

include(CPack)
```

### Performance and Debugging Tools

| Technology | Version | Purpose | When to Use | Confidence |
|------------|---------|---------|-------------|------------|
| perf | (system) | CPU profiling | Profile hot paths in event loop, request parsing, file I/O. Use `perf record` + `perf report`. Essential for optimizing the epoll loop. | HIGH |
| strace | (system) | System call tracing | Debug socket/epoll/file descriptor issues. `strace -e trace=network,epoll` to see exactly what syscalls the server makes. | HIGH |
| gdb | (system) | Debugger | Core dump analysis, breakpoint debugging. Ensure debug builds include `-g -O0`. | HIGH |
| wrk | (system) | HTTP benchmarking | Load testing. `wrk -t4 -c100 -d30s http://localhost:8080/` for baseline performance measurement. Better than ab (Apache Bench) -- supports Lua scripting and more accurate latency measurement. | HIGH |
| h2load | (system) | HTTP benchmarking | Alternative to wrk with better reporting. Part of nghttp2 package. | MEDIUM |

### Code Formatting

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| clang-format | 17+ | Code formatting | Enforces consistent style across the codebase. Use a `.clang-format` file with a defined style (recommend `BasedOnStyle: LLVM` with modifications for C conventions). Run in CI to reject unformatted code. | HIGH |

### Documentation

| Technology | Version | Purpose | Why | Confidence |
|------------|---------|---------|-----|------------|
| Doxygen | 1.9+ | API documentation | Already configured. Keep using it. Ensure all public headers have `@brief`, `@param`, `@return` annotations. | HIGH |

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| TLS | OpenSSL 3.x | LibreSSL | Smaller ecosystem, API drift from OpenSSL, primarily OpenBSD-focused |
| TLS | OpenSSL 3.x | wolfSSL | GPL licensing complexity, less community adoption for servers |
| TLS | OpenSSL 3.x | No TLS | Users expect HTTPS; "use a separate TLS terminator" adds deployment friction |
| Testing | CMocka | Check | Fork-based isolation is heavier; CMocka's linker-wrap mocking is critical for network code |
| Testing | CMocka | Unity + CMock | Requires Ruby for code generation; unnecessary build dependency |
| Testing | CMocka | Custom macros | No mocking, no test isolation, exits on first failure |
| Analysis | cppcheck + clang-tidy | Coverity | Commercial; overkill for an open-source project (free tier exists but requires cloud submission) |
| Analysis | cppcheck + clang-tidy | PVS-Studio | Commercial; free for OSS but requires license renewal |
| Packaging | CPack | FPM | External Ruby tool; CPack is already bundled with CMake |
| Benchmarking | wrk | ab (Apache Bench) | Less accurate latency measurement, no scripting, older tool |
| Event loop | epoll (raw) | libevent/libev | Project is Linux-only; raw epoll is correct. libevent adds abstraction overhead and a dependency for cross-platform support that is not needed. |
| HTTP parser | Custom | llhttp (Node.js parser) | Viable alternative, but the project's custom parser is already working and is a core learning/control point. Rewriting to use llhttp would mean giving up control of a critical component. |
| HTTP parser | Custom | http-parser (Joyent) | Deprecated in favor of llhttp. Do not use. |
| Config parser | Custom INI | libconfuse / libconfig | Adds external dependency for something the custom parser handles. The INI format is sufficient. |

## What NOT to Use

| Technology | Why Not |
|------------|---------|
| libevent / libev / libuv | Project is Linux-only. Raw epoll is the correct abstraction level. These libraries exist to abstract over kqueue/epoll/IOCP; that abstraction is wasted here and adds a dependency. |
| http-parser (Joyent) | Deprecated. Superseded by llhttp. |
| Any `select()` or `poll()` based approach | epoll is strictly superior on Linux for the connection counts this server targets (1000+). |
| autotools (autoconf/automake) | CMake is already configured and working. Autotools is legacy for new projects. |
| Meson | Good build system, but switching from CMake provides no benefit and would require rewriting all build configuration. |
| Threading (pthreads) for request handling | The single-threaded event-driven model is correct for an HTTP proxy. Threading adds complexity (locking, race conditions) without proportional benefit. Consider threading only for CPU-bound tasks (TLS handshakes at scale), and only after profiling proves it necessary. |
| `sendfile()` replacement libraries | Use raw `sendfile()` syscall directly for static file serving. No library needed. |

## Build Configurations

### Development

```bash
# Configure with sanitizers and static analysis
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_SANITIZERS=ON \
    -DCMAKE_C_COMPILER=gcc

# Build
cmake --build build

# Test with sanitizers active
ctest --test-dir build --output-on-failure

# Static analysis (separate pass)
cppcheck --enable=all --suppress=missingIncludeSystem \
    --error-exitcode=1 -I src/ src/
clang-tidy -p build src/**/*.c
```

### Release

```bash
# Configure optimized build
cmake -B build-release -DCMAKE_BUILD_TYPE=Release \
    -DWITH_TLS=ON

# Build
cmake --build build-release

# Validate with Valgrind (no sanitizers)
valgrind --leak-check=full --error-exitcode=1 \
    ./build-release/test_runner

# Generate packages
cd build-release && cpack
```

### CI Pipeline

```bash
# Matrix: GCC + Clang, Debug + Release, Sanitizers ON/OFF
# Minimum CI steps:
1. Build (GCC, Debug, Sanitizers ON)
2. Test with ASan+UBSan
3. Build (Clang, Debug)
4. clang-tidy pass
5. cppcheck pass
6. Build (GCC, Release)
7. Valgrind memcheck
8. Package generation (DEB + RPM)
```

## Installation

### Build Dependencies (Development)

```bash
# Fedora/RHEL
sudo dnf install gcc cmake make openssl-devel valgrind cppcheck \
    clang clang-tools-extra libcmocka-devel

# Debian/Ubuntu
sudo apt install gcc cmake make libssl-dev valgrind cppcheck \
    clang clang-tidy libcmocka-dev

# Benchmarking tools (optional)
sudo dnf install wrk  # or build from source
```

### Runtime Dependencies

```bash
# Fedora/RHEL
openssl-libs  # Only if built with TLS

# Debian/Ubuntu
libssl3  # Only if built with TLS
```

## Version Verification Notes

The following versions are based on training data (May 2025 cutoff). Verify before pinning:

| Technology | Stated Version | Verify At |
|------------|---------------|-----------|
| OpenSSL | 3.2+ | https://www.openssl.org/source/ |
| CMocka | 1.1.7+ | https://cmocka.org/ |
| cppcheck | 2.13+ | https://cppcheck.sourceforge.io/ |
| Valgrind | 3.22+ | https://valgrind.org/ |
| CMake | 3.20+ | https://cmake.org/download/ |

## Sources

- Training data knowledge of C systems programming ecosystem (MEDIUM confidence -- this domain is extremely stable and slow-moving, so training data is reliable, but exact version numbers should be verified)
- Existing codebase analysis (HIGH confidence -- read directly from source)
- Industry standard practices from nginx, HAProxy, lighttpd architectures (HIGH confidence -- well-established patterns)
