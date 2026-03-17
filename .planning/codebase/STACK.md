# Technology Stack

**Analysis Date:** 2026-03-17

## Languages

**Primary:**
- C (C11) - All application code, server, HTTP handling, socket operations

**Build System:**
- CMake 3.20+ - Project configuration and build orchestration

## Runtime

**Environment:**
- Linux (required for epoll support)
- GNU C library (glibc) for extensions like `memmem()`, `asprintf()`, `epoll`

**Platform Requirements:**
- x86_64 or ARM-based Linux systems
- POSIX-compliant socket APIs
- epoll support (Linux-specific)

## Build System & Tools

**Primary:**
- CMake 3.20+ - Configuration and build system
- GNU Make - Build orchestration via `Makefile`
- GCC/Clang - C compiler (via CMake toolchain)

**Build Modes:**
- Debug: `-DCMAKE_BUILD_TYPE=Debug` (default, includes debugging symbols)
- Release: `-DCMAKE_BUILD_TYPE=Release` (optimized, stripped symbols)

**Compilation Flags:**
- Standard: C11 (`-std=c11`)
- GNU extensions: `-D_GNU_SOURCE` (required for memmem(), asprintf(), epoll)
- Warning level: `-Wall -Wextra`

## Documentation

**Optional:**
- Doxygen - API documentation generation
  - Config: `Doxyfile.in` (template, generated to `build/Doxyfile`)
  - Output: `build/doc/html/` (auto-generated if Doxygen installed)
  - Trigger: `make docs` or `cmake --build build --target docs`

## Testing

**Framework:**
- CMake CTest - Test runner
- Custom C test framework with minimal assertions (no external test library)
- Location: `tests/test_parsers.c`
- Run: `make test` or `ctest --test-dir build --output-on-failure`

**Test Infrastructure:**
- Single executable: `test_runner` (built from `tests/test_parsers.c`)
- Linked against: `cserve_core` library
- Assertions: Macro-based `ASSERT()` with test counters

## Core Libraries (System/Standard)

**Standard C:**
- `stdio.h`, `stdlib.h`, `string.h` - Standard I/O and utilities
- `unistd.h` - POSIX system calls (fork, exec, close, read, write)
- `fcntl.h` - File control operations (O_NONBLOCK flags)
- `errno.h` - Error handling
- `limits.h` - System limits
- `ctype.h` - Character classification
- `stdarg.h` - Variable argument lists (for logging)
- `time.h` - Time functions and timestamps

**Networking:**
- `sys/socket.h` - Socket creation and operations
- `sys/types.h` - System data types
- `netinet/in.h` - Internet addressing (IPv4)
- `arpa/inet.h` - IP address conversion
- `netdb.h` - Network database (getaddrinfo, freeaddrinfo)

**Linux-Specific:**
- `sys/epoll.h` - Event polling mechanism (epoll_create1, epoll_wait, epoll_ctl)
- `linux/limits.h` - Linux-specific path limits
- `sys/stat.h` - File status (open, stat operations)

**No External Dependencies:**
- No third-party C libraries
- No package manager (npm, cargo, pip)
- Self-contained compilation

## Configuration Files

**Build Configuration:**
- `CMakeLists.txt` - CMake project definition
  - Static library: `cserve_core` (shared by server and tests)
  - Executable: `cserve` (main binary)
  - Export: `compile_commands.json` for IDE integration
- `Makefile` - Convenience wrapper around cmake commands
  - Targets: configure, build, release, run, test, docs, clean, rm

**Compiler Output:**
- `build/compile_commands.json` - Generated, used by IDEs and language servers

**Documentation:**
- `Doxyfile.in` - Doxygen template (processed by CMake)

## Project Structure

**Build Directory:**
- `build/` - Output directory for compiled binaries and artifacts
  - `cserve` - Main server executable
  - `test_runner` - Test executable
  - `Doxyfile` - Generated from template
  - `doc/html/` - Generated HTML documentation
  - `compile_commands.json` - Compiler command database

**Source Organization:**
- `src/` - Main application code
  - `main.c` - Entry point
  - `common.h` - Shared definitions and constants
  - `sock/` - Low-level socket server (TCP)
  - `http/` - HTTP protocol handling
  - `utils/` - Configuration parsing and logging

**Tests:**
- `tests/` - Test code
  - `test_parsers.c` - HTTP parser unit tests

## Build Commands

**Development:**
```bash
make configure          # Initialize debug build
make build              # Compile (debug mode)
make run                # Compile and run server
make test               # Compile and run tests
make docs               # Generate Doxygen documentation
```

**Production:**
```bash
make release            # Compile optimized release build
```

**Cleanup:**
```bash
make clean              # Remove compiled objects, keep CMakeCache
make rm                 # Delete entire build directory
```

## Performance Considerations

**Architecture:**
- Single-threaded event-driven (epoll-based)
- Non-blocking socket I/O
- Connection pooling (up to 1000 concurrent connections)
- Configurable buffer sizes (4096 bytes initial request buffer)

**Resource Limits (hardcoded):**
- Max connections: 1000 (`MAX_CONNECTIONS`)
- Max epoll events: 1024 (`MAX_EPOLL_EVENTS`)
- Max headers per request: 50 (`MAX_HEADERS`)
- Max backend upstreams: 16 (`MAX_BACKENDS`)

---

*Stack analysis: 2026-03-17*
