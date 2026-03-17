# Coding Conventions

**Analysis Date:** 2026-03-17

## Naming Patterns

**Files:**
- Headers: `module_name.h` (e.g., `request.h`, `logger.h`, `server.h`)
- Implementations: `module_name.c` paired with header file
- Test files: `test_module_name.c` (e.g., `test_parsers.c`)

**Functions:**
- Use snake_case: `parse_request_line()`, `httpresponse_serialize()`, `strip_whitespace()`
- Constructor functions: `module_constructor()` (e.g., `httpserver_constructor()`, `httpresponse_constructor()`)
- Destructor functions: `module_destructor()` or `free_module()` (e.g., `httpserver_destructor()`, `free_http_request()`)
- Parser functions: `parse_entity_type()` (e.g., `parse_request_line()`, `parse_header()`)
- Builder functions: `entity_builder()` (e.g., `response_builder()`)
- Getter functions: `get_property()` (e.g., `get_mime_type()`)

**Variables:**
- Local variables: snake_case (e.g., `ptr`, `consumed`, `raw`, `cfg`, `req`)
- Struct members: snake_case (e.g., `request_line`, `header_count`, `body_len`)
- Global variables: Use prefixes when necessary (e.g., `g_tests_run`, `g_tests_passed`, `httpserver_ptr`)
- Constants: ALL_CAPS (e.g., `MAX_CONNECTIONS`, `INITIAL_BUFFER_SIZE`, `OK`)

**Types:**
- Struct names: PascalCase (e.g., `HTTPRequest`, `HTTPServer`, `HTTPResponse`, `SocketServer`)
- Enum names: PascalCase (e.g., `ConnectionState`, `HTTPRequestState`, `ErrorCode`, `HTTPMethod`)
- Enum values: ALL_CAPS (e.g., `CONN_ESTABLISHED`, `REQ_PARSE_LINE`, `OK`, `GET`)

## Code Style

**Formatting:**
- Tool: clang-format (configuration in `.clang-format`)
- Indentation: 4 spaces (no tabs)
- Column limit: 100 characters
- Brace style: Allman (opening brace on new line)
- Short if statements allowed on single line
- Inline functions permitted for short functions

**Linting:**
- Compiler flags: `-Wall -Wextra -D_GNU_SOURCE`
- GNU C11 standard required (`-std=c11`)
- Header guards: Traditional format `#ifndef MODULE_H` / `#define MODULE_H` / `#endif`

## File Organization

**Header files (.h):**
- Include guards at top
- Doxygen file comment block (example from `request.h`):
  ```c
  /**
   * @file    request.h
   * @author  Samandar Komil
   * @date    24 April 2025
   * @brief   HTTPRequest struct and method prototypes.
   *
   */
  ```
- All required includes
- Typedef structs
- Function declarations
- Closing `#endif` comment with guard name

**Implementation files (.c):**
- Doxygen file comment block with same format
- Includes section (no specific sorting beyond system vs local)
- Function implementations in declaration order
- Helper functions before public functions

## Import Organization

**Order:**
1. System includes: `<stdio.h>`, `<stdlib.h>`, `<string.h>`, etc.
2. Linux/network includes: `<linux/limits.h>`, `<netinet/in.h>`, `<sys/socket.h>`, etc.
3. Local project includes: `"common.h"`, `"utils/logger.h"`, `"http/request.h"`

**Path Aliases:**
- Not used; relative paths from include directories defined in CMakeLists.txt
- `src/` is added as include directory, so imports use module paths: `#include "http/request.h"`

## Error Handling

**Patterns:**
- Return negative error codes for functions that fail (e.g., `-1`, error constants from `ErrorCode` enum)
- Return 0 or `OK` constant for success
- NULL pointer checks before dereferencing: `if (!ptr) return -1;`
- Error code constants defined in `common.h` (e.g., `SOCKET_BIND_ERROR = -701`)
- Use LOG macro for error logging: `LOG("ERROR", "message with format %d", value)`
- Parser functions return consumed bytes on success, negative on error

**Examples from codebase:**
- `parse_request_line()` returns bytes consumed or `-1` on error
- `httpresponse_add_header()` returns `OK` (0) or `-1` on failure
- `parse_config()` returns pointer or `NULL` if file cannot be opened

## Logging

**Framework:** Custom macro-based logger

**Pattern:**
```c
#define LOG(level, fmt, ...) log_message(level, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
```

**When to Log:**
- Use `LOG("DEBUG", ...)` for parsing/low-level details (e.g., in `parse_request_line()`)
- Use `LOG("INFO", ...)` for startup/lifecycle events (e.g., "Waiting for connections on port %d")
- Use `LOG("ERROR", ...)` for failures and error conditions (e.g., "Failed to initialize epoll instance")
- Log output includes timestamp, level, file, line number, and message

**Implementation location:** `src/utils/logger.c` and `src/utils/logger.h`

## Comments

**When to Comment:**
- Doxygen blocks for all public functions in headers and implementations
- Inline comments explain non-obvious algorithm details (e.g., "Putting pointers at start and end of the incoming string")
- Section separators for major code blocks (example from test file):
  ```c
  /* ------------------------------------------------------------------ */
  /* Parser tests                                                         */
  /* ------------------------------------------------------------------ */
  ```

**Doxygen/Doc Comments:**
- All public functions have Doxygen blocks in header files
- Format includes: `@file`, `@author`, `@date`, `@brief`, `@param`, `@return`, `@details`
- Example from `config.h`:
  ```c
  /**
   * @brief   Parses a config file and initializes a Config struct.
   *
   * This function takes a filename as input and returns a pointer to a Config
   * struct. The Config struct contains the parsed values from the config file.
   *
   * The config file should contain the following format:
   * <key>=<value>
   *
   * The following keys are accepted:
   * - port
   * - root
   * - static_dir
   * - backend
   *
   * If a key is not recognized, it will be ignored.
   *
   * If a key is repeated, the last value will be used.
   *
   * The backend key can be repeated multiple times to specify multiple backends.
   *
   * The function returns a pointer to a Config struct if the config file is
   * parsed successfully, otherwise it returns NULL.
   */
  ```

## Function Design

**Size:** Functions typically 20-80 lines; longer functions (like `launch()` in `server.c`) handle complex state machines

**Parameters:**
- Pointer parameters for structs that need modification (e.g., `HTTPRequest *req`)
- Input data as const pointers (e.g., `const char *line`)
- Out parameters via pointers: `size_t *out_len` in `httpresponse_serialize()`
- No variadic user functions (except logger)

**Return Values:**
- Negative integers for errors
- 0 or positive for success/data
- NULL for allocation failures
- Pointer returns for allocated resources (caller must free)

## Module Design

**Exports:**
- Public functions declared in headers
- Constructor/destructor pairs for resource management
- Single responsibility per module (e.g., `parsers.h` for parsing, `logger.h` for logging)

**Barrel Files:**
- `common.h` serves as unified include of common definitions, enums, and constants
- Most modules include `common.h` for access to enum types and shared constants

**Memory Management Pattern:**
- Structs allocated with `malloc()` or `calloc()`
- Destructor functions handle all cleanup (free sub-fields, then free struct)
- Ownership transfer documented in comments: "transferring ownership, caller frees the memory" (from `httpresponse_serialize()`)

---

*Convention analysis: 2026-03-17*
