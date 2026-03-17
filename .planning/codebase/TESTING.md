# Testing Patterns

**Analysis Date:** 2026-03-17

## Test Framework

**Runner:**
- CMake with CTest
- Test executable: `test_runner` built from `tests/test_parsers.c`
- Config file: `CMakeLists.txt` (lines 38-45)

**Assertion Library:**
- Custom minimal framework (no external dependency)
- Macro-based assertions defined in test file

**Run Commands:**
```bash
make test                                    # Build and run test suite
ctest --test-dir build --output-on-failure   # Run tests with output on failure
make build                                   # Build first (if not already built)
```

## Test File Organization

**Location:**
- Separate directory: `tests/` contains all test files
- Currently one test file: `tests/test_parsers.c`
- Test files are co-located but separate from source

**Naming:**
- Pattern: `test_module_name.c`
- Single file `test_parsers.c` tests multiple modules (parsers, MIME types, response builder)

**Structure:**
```
tests/
└── test_parsers.c         # Unit tests for HTTP parsers, MIME type detection, response building
```

## Test Structure

**Suite Organization:**

From `tests/test_parsers.c` (lines 191-212):

```c
int main(void)
{
    printf("=== cserve unit tests ===\n\n");

    printf("[ parsers ]\n");
    RUN(test_parse_request_line_get);
    RUN(test_parse_request_line_post);
    RUN(test_parse_request_line_missing_crlf);
    RUN(test_parse_full_request_headers);

    printf("\n[ mime ]\n");
    RUN(test_get_mime_type);

    printf("\n[ response ]\n");
    RUN(test_response_builder_200);
    RUN(test_response_builder_404);
    RUN(test_response_serialize_status_line);

    printf("\n=== %d/%d passed ===\n", g_tests_passed, g_tests_run);

    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
```

**Patterns:**

1. **Global test counters** (lines 20-21):
   ```c
   static int g_tests_run    = 0;
   static int g_tests_passed = 0;
   ```

2. **Test sections** organized with printf headers:
   - `[ parsers ]` - Request line and header parsing tests
   - `[ mime ]` - MIME type detection tests
   - `[ response ]` - Response building and serialization tests

3. **Individual test functions** follow naming pattern `test_module_behavior()`:
   ```c
   static void test_parse_request_line_get(void)
   {
       // Setup
       char raw[] = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
       HTTPRequest *req = create_http_request();

       // Execute
       int consumed = parse_request_line(req, raw, strlen(raw));

       // Assert
       ASSERT(consumed > 0);
       ASSERT(req->request_line.method_len == 3);
       ASSERT(strncmp(req->request_line.method, "GET", 3) == 0);

       // Cleanup
       free_http_request(req);
   }
   ```

## Custom Test Macros

**ASSERT macro** (lines 24-31):
```c
#define ASSERT(expr)                                                        \
    do {                                                                    \
        if (!(expr)) {                                                      \
            fprintf(stderr, "  [FAIL] %s:%d  assertion failed: %s\n",      \
                    __FILE__, __LINE__, #expr);                             \
            exit(1);                                                        \
        }                                                                   \
    } while (0)
```
- Checks condition and exits test with failure code if false
- Prints file, line number, and condition text
- Exit behavior allows CTest to mark test as failed

**RUN macro** (lines 33-39):
```c
#define RUN(fn)                                                             \
    do {                                                                    \
        g_tests_run++;                                                      \
        fn();                                                               \
        g_tests_passed++;                                                   \
        printf("  [PASS] %s\n", #fn);                                      \
    } while (0)
```
- Increments run counter, calls test function, increments pass counter
- Only increments pass if function doesn't exit via failed ASSERT
- Prints `[PASS]` message with function name

## Mocking

**Framework:** None used

**Patterns:**
- Direct function calls (no mocking)
- No dependency injection or mock frameworks
- Tests operate on real objects and structures

**What to Mock:**
- Not applicable - minimal test structure doesn't use mocking
- Tests call parsing functions directly with test data

**What NOT to Mock:**
- Structs are tested with real allocations
- Memory management is tested by verifying function behavior on allocated structs

## Fixtures and Factories

**Test Data:**

Example from `test_parse_request_line_get()` (lines 46-65):
```c
static void test_parse_request_line_get(void)
{
    char raw[] = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";

    HTTPRequest *req = create_http_request();
    ASSERT(req != NULL);

    int consumed = parse_request_line(req, raw, strlen(raw));
    ASSERT(consumed > 0);

    ASSERT(req->request_line.method_len == 3);
    ASSERT(strncmp(req->request_line.method, "GET", 3) == 0);
    // ... more assertions
}
```

- Test data hardcoded in test functions as char arrays
- Real HTTP request strings used as fixtures (with CRLF line endings)
- Use `create_http_request()` factory from main code to create test objects

**Factories in use:**
- `create_http_request()` - Creates empty HTTPRequest struct
- `response_builder()` - Creates HTTPResponse with status, body, content-type
- Test data fixtures: Hardcoded HTTP request/response strings

**Location:**
- Test data inline in test functions (no separate fixture files)
- Fixtures represent real HTTP protocol messages

## Coverage

**Requirements:** Not enforced

**View Coverage:** Not configured

**Testing approach:**
- Basic happy-path testing
- Error case testing (e.g., `test_parse_request_line_missing_crlf()`)
- No formal coverage metrics

## Test Types

**Unit Tests:**
- Scope: Individual functions (parsers, response builder, MIME type detection)
- Approach: Call function with test data, verify output via ASSERT macros
- Current tests in `tests/test_parsers.c` (7 test functions):
  1. `test_parse_request_line_get` - GET request parsing
  2. `test_parse_request_line_post` - POST request parsing
  3. `test_parse_request_line_missing_crlf` - Error handling for malformed request
  4. `test_parse_full_request_headers` - Full request with headers parsing
  5. `test_get_mime_type` - MIME type detection for file extensions
  6. `test_response_builder_200` - Response creation with 200 status
  7. `test_response_builder_404` - Response creation with 404 status
  8. `test_response_serialize_status_line` - Response serialization

**Integration Tests:**
- Not present - scope limited to unit testing of parsers and builders

**E2E Tests:**
- Not used - no end-to-end testing framework configured

## Test Examples

**Basic Parser Test:**
```c
static void test_parse_request_line_post(void)
{
    char raw[] = "POST /api/users HTTP/1.1\r\nContent-Type: application/json\r\n\r\n";

    HTTPRequest *req = create_http_request();
    ASSERT(req != NULL);

    int consumed = parse_request_line(req, raw, strlen(raw));
    ASSERT(consumed > 0);

    ASSERT(req->request_line.method_len == 4);
    ASSERT(strncmp(req->request_line.method, "POST", 4) == 0);

    ASSERT(req->request_line.uri_len == 10);
    ASSERT(strncmp(req->request_line.uri, "/api/users", 10) == 0);

    free_http_request(req);
}
```

**Error Case Test:**
```c
static void test_parse_request_line_missing_crlf(void)
{
    /* Malformed: no CRLF at end of request line */
    char raw[] = "GET /index.html HTTP/1.1";

    HTTPRequest *req = create_http_request();
    ASSERT(req != NULL);

    int consumed = parse_request_line(req, raw, strlen(raw));
    ASSERT(consumed < 0);   /* must reject */

    free_http_request(req);
}
```

**Response Builder Test:**
```c
static void test_response_builder_200(void)
{
    const char body[] = "Hello, World!";
    HTTPResponse *res = response_builder(200, "OK", body, strlen(body), "text/plain");
    ASSERT(res != NULL);
    ASSERT(res->status_code == 200);
    ASSERT(res->body != NULL);
    ASSERT(res->body_length == (int)strlen(body));

    httpresponse_free(res);
}
```

**String Matching Test:**
```c
static void test_get_mime_type(void)
{
    ASSERT(strcmp(get_mime_type("file.html"), "text/html") == 0);
    ASSERT(strcmp(get_mime_type("style.css"), "text/css") == 0);
    ASSERT(strcmp(get_mime_type("app.js"), "application/javascript") == 0);
    ASSERT(strcmp(get_mime_type("image.png"), "image/png") == 0);
    ASSERT(strcmp(get_mime_type("data.json"), "application/json") == 0);
    ASSERT(strcmp(get_mime_type("unknown.xyz"), "application/octet-stream") == 0);
    ASSERT(strcmp(get_mime_type("no_extension"), "application/octet-stream") == 0);
}
```

## Build Integration

**CMakeLists.txt Configuration** (lines 38-45):
```cmake
enable_testing()

add_executable(test_runner tests/test_parsers.c)
target_include_directories(test_runner PRIVATE src)
target_link_libraries(test_runner PRIVATE cserve_core)

add_test(NAME unit_tests COMMAND test_runner)
```

**Key points:**
- Test executable links against `cserve_core` library (containing all source modules)
- Test has access to all public headers via `src` include directory
- CTest integration via `add_test()` - registered test named `unit_tests`

**Makefile target** (lines 27-29):
```makefile
## test        — build then run the test suite
test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure
```

## When to Add Tests

Add tests when:
- Implementing new parser functionality
- Adding response building features
- Adding utility functions (e.g., MIME type detection)
- Fixing bugs (add test case to prevent regression)

Follow the pattern:
1. Create `static void test_description(void)` function
2. Setup test data or fixtures
3. Call function under test
4. Assert expected behavior with ASSERT macros
5. Cleanup allocated resources
6. Add `RUN(test_description)` to main() in appropriate section

---

*Testing analysis: 2026-03-17*
