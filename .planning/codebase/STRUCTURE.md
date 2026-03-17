# Codebase Structure

**Analysis Date:** 2026-03-17

## Directory Layout

```
cserve/
├── CMakeLists.txt              # CMake build config; defines cserve_core library and test_runner target
├── Makefile                    # Top-level build targets (after CMake generation)
├── Doxyfile.in                 # Doxygen config template for API documentation
├── README.md                   # Project overview and quick-start build instructions
├── LICENSE                     # ISC license
├── .clang-format               # Clang formatting rules (code style)
├── src/                        # Source code root
│   ├── main.c                  # Entry point: signal handling, config load, server init/launch
│   ├── common.h                # Shared constants, enums, macro definitions
│   ├── http/                   # HTTP protocol handling
│   │   ├── server.c            # Event loop, connection management, request/response cycle
│   │   ├── server.h            # HTTPServer and Connection structs
│   │   ├── request.c           # HTTPRequest constructor/destructor
│   │   ├── request.h           # HTTPRequest, HTTPRequestLine, HTTPHeader structs
│   │   ├── response.c          # HTTPResponse constructor, serialization, builder
│   │   ├── response.h          # HTTPResponse struct
│   │   ├── parsers.c           # HTTP request parser, MIME type detection
│   │   └── parsers.h           # Parser function declarations
│   ├── sock/                   # Low-level socket abstraction
│   │   ├── server.c            # SocketServer constructor/destructor
│   │   └── server.h            # SocketServer struct
│   └── utils/                  # Utilities
│       ├── config.c            # INI config file parsing
│       ├── config.h            # Config struct, parsing functions
│       ├── logger.c            # Logging implementation with timestamps
│       └── logger.h            # LOG macro definition
├── tests/                      # Test suite
│   └── test_parsers.c          # Unit tests for request parser, response builder, MIME types
└── docs/                       # Documentation
    ├── REPORT.md               # Analysis/vulnerability report
    └── FIXES.md                # Fix documentation
```

## Directory Purposes

**src/:**
- Purpose: All source code for the HTTP server implementation
- Contains: C source files (.c) and header files (.h)
- Key files: `main.c` (entry point), `common.h` (shared constants)

**src/http/:**
- Purpose: HTTP protocol parsing and response generation
- Contains: Request/response structs, parsers, event loop server
- Key files:
  - `server.c`: Main event loop (epoll-based), connection handling
  - `parsers.c`: HTTP request line/header parsing, MIME type mapping
  - `request.h`: HTTPRequest struct with state machine enum
  - `response.h`: HTTPResponse struct and serialization

**src/sock/:**
- Purpose: Low-level TCP socket abstraction
- Contains: Socket creation, binding, configuration (SO_REUSEADDR)
- Key files: `server.c` (constructor/destructor), `server.h` (SocketServer struct)

**src/utils/:**
- Purpose: Cross-cutting utilities
- Contains: Configuration loading, logging infrastructure
- Key files:
  - `config.c`: INI parser for cserver.ini (port, static_dir, backends)
  - `logger.c`: Timestamp + level + file/line formatted logging

**tests/:**
- Purpose: Unit test suite
- Contains: Test harness (simple assert macro), parser tests, response builder tests
- Key files: `test_parsers.c` (7 test cases covering happy paths and error cases)

**docs/:**
- Purpose: Analysis and fix documentation
- Contains: Vulnerability reports, fix tracking

## Key File Locations

**Entry Points:**
- `src/main.c`: Process entry point; initializes signal handlers, parses config, constructs HTTPServer, calls launch()

**Configuration:**
- `src/common.h`: Lines 37-41 define buffer sizes (INITIAL_BUFFER_SIZE=4096, MAX_CONNECTIONS=1000, MAX_HEADERS=50, MAX_BACKENDS=16)
- `src/common.h`: Lines 47-91 define state enums (ConnectionState, HTTPRequestState, ErrorCode, HTTPMethod)

**Core Logic:**
- `src/http/server.c` lines 11-343: launch() function implements the main epoll event loop
- `src/http/server.c` lines 345-516: request_handler() routes requests to file serving or reverse proxy
- `src/http/parsers.c` lines 11-47: parse_request_line() extracts method/uri/protocol
- `src/http/parsers.c` lines 82-142: parse_http_request() assembles request line + headers

**Testing:**
- `tests/test_parsers.c`: Manual test runner with ASSERT macro, 7 test cases, CTest integration via CMakeLists.txt

## Naming Conventions

**Files:**
- Implementation: lowercase_with_underscores.c (e.g., parsers.c, server.c)
- Headers: lowercase_with_underscores.h (e.g., parsers.h, server.h)
- Logical grouping: Related files in subdirectories (http/, sock/, utils/)

**Directories:**
- Functional domains: http/ (protocol), sock/ (sockets), utils/ (helpers)
- lowercase, singular or plural based on content type (e.g., utils not util)

**Functions:**
- snake_case with verb prefix for actions: parse_http_request(), httpresponse_serialize(), server_constructor()
- Constructors: {Struct}Constructor (httpserver_constructor)
- Destructors: {Struct}Destructor (httpserver_destructor)
- Getter/setters: Direct struct member access (no getters); utility functions prefixed with struct name or operation

**Variables:**
- Local/parameters: snake_case (ptr, buffer_len, client_fd)
- Struct members: snake_case (buffer_size, request_line, status_code)
- Macros: SCREAMING_SNAKE_CASE (INITIAL_BUFFER_SIZE, MAX_CONNECTIONS, LOG)
- Enums: PascalCase for enum type names (ConnectionState, HTTPMethod); SCREAMING_SNAKE_CASE for values (CONN_ESTABLISHED, GET)

**Types:**
- Structs: PascalCase (HTTPRequest, SocketServer, Connection)
- Typedefs on struct: same name as struct (typedef struct HTTPRequest HTTPRequest;)
- Enums: PascalCase for enum name (HTTPRequestState); SCREAMING_SNAKE_CASE for values (REQ_PARSE_LINE)

## Where to Add New Code

**New Feature (e.g., new request routing):**
- Primary code: `src/http/server.c` lines 345-516 in request_handler() function; add new else-if branch for new URI pattern
- Tests: `tests/test_parsers.c` add test case function and call RUN() in main()

**New Component/Module (e.g., request caching, compression):**
- Implementation: Create new directory under src/ (e.g., `src/cache/`, `src/compress/`)
- Header: `src/{component}/{component}.h` with public interface
- Implementation: `src/{component}/{component}.c` with logic
- Integration: Include in CMakeLists.txt add_library(cserve_core ...) source list
- Tests: Add to `tests/test_parsers.c` or create `tests/test_{component}.c`

**Utilities (shared helpers):**
- Shared helpers: `src/utils/` for general-purpose functions (logging, config, string utilities)
- Logging: Use LOG(level, fmt, ...) macro from `src/utils/logger.h` throughout

**Constants/Enums:**
- Global limits: `src/common.h` lines 37-43 (MAX_*, INITIAL_*)
- Request/response states: `src/common.h` lines 47-91 (ConnectionState, HTTPRequestState, ErrorCode, HTTPMethod)

## Special Directories

**build/:**
- Purpose: CMake output directory (build artifacts, compiled objects, executables, tests)
- Generated: Yes
- Committed: No (in .gitignore)

**docs/:**
- Purpose: Project documentation and analysis reports
- Generated: No (manual documentation)
- Committed: Yes

**.git/:**
- Purpose: Git version control metadata
- Generated: Yes
- Committed: N/A

## Build System

**CMake (CMakeLists.txt):**
- Defines C11 standard, -Wall -Wextra compile flags, -D_GNU_SOURCE (for memmem, asprintf, epoll)
- Creates static library `cserve_core` from all source files except main.c
- Builds executable `cserve` from main.c linked against cserve_core
- Builds test executable `test_runner` from test_parsers.c linked against cserve_core
- Optional Doxygen target for API documentation

**Building:**
```bash
cd /home/voidp/Projects/oss/cserve
mkdir build
cd build
cmake ..
make
./cserve                    # Run server (reads cserver.ini)
ctest --output-on-failure   # Run tests
```

---

*Structure analysis: 2026-03-17*
