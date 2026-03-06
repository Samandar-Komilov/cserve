/**
 * @file    test_parsers.c
 * @brief   Unit tests for the HTTP request parser and response builder.
 *
 * Run via CTest: ctest --test-dir build --output-on-failure
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http/parsers.h"
#include "http/request.h"
#include "http/response.h"

/* ------------------------------------------------------------------ */
/* Minimal test framework                                               */
/* ------------------------------------------------------------------ */

static int g_tests_run    = 0;
static int g_tests_passed = 0;

/* On failure: print the location and exit non-zero so CTest marks as failed */
#define ASSERT(expr)                                                        \
    do {                                                                    \
        if (!(expr)) {                                                      \
            fprintf(stderr, "  [FAIL] %s:%d  assertion failed: %s\n",      \
                    __FILE__, __LINE__, #expr);                             \
            exit(1);                                                        \
        }                                                                   \
    } while (0)

#define RUN(fn)                                                             \
    do {                                                                    \
        g_tests_run++;                                                      \
        fn();                                                               \
        g_tests_passed++;                                                   \
        printf("  [PASS] %s\n", #fn);                                      \
    } while (0)

/* ------------------------------------------------------------------ */
/* Parser tests                                                         */
/* ------------------------------------------------------------------ */

static void test_parse_request_line_get(void)
{
    char raw[] = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";

    HTTPRequest *req = create_http_request();
    ASSERT(req != NULL);

    int consumed = parse_request_line(req, raw, strlen(raw));
    ASSERT(consumed > 0);

    ASSERT(req->request_line.method_len == 3);
    ASSERT(strncmp(req->request_line.method, "GET", 3) == 0);

    ASSERT(req->request_line.uri_len == 11);
    ASSERT(strncmp(req->request_line.uri, "/index.html", 11) == 0);

    ASSERT(req->request_line.protocol_len == 8);
    ASSERT(strncmp(req->request_line.protocol, "HTTP/1.1", 8) == 0);

    free_http_request(req);
}

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

static void test_parse_full_request_headers(void)
{
    char raw[] =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Connection: keep-alive\r\n"
        "Accept: text/html\r\n"
        "\r\n";

    HTTPRequest *req = create_http_request();
    ASSERT(req != NULL);

    int result = parse_http_request(raw, strlen(raw), req);
    ASSERT(result >= 0);
    ASSERT(req->header_count == 3);

    /* First header: Host */
    ASSERT(req->headers[0].name_len == 4);
    ASSERT(strncmp(req->headers[0].name, "Host", 4) == 0);

    /* Second header: Connection */
    ASSERT(req->headers[1].name_len == 10);
    ASSERT(strncmp(req->headers[1].name, "Connection", 10) == 0);

    free_http_request(req);
}

/* ------------------------------------------------------------------ */
/* MIME type tests                                                       */
/* ------------------------------------------------------------------ */

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

/* ------------------------------------------------------------------ */
/* Response builder tests                                               */
/* ------------------------------------------------------------------ */

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

static void test_response_builder_404(void)
{
    const char body[] = "<h1>Not Found</h1>";
    HTTPResponse *res = response_builder(404, "Not Found", body, strlen(body), "text/html");
    ASSERT(res != NULL);
    ASSERT(res->status_code == 404);
    ASSERT(strcmp(res->reason_phrase, "Not Found") == 0);

    httpresponse_free(res);
}

static void test_response_serialize_status_line(void)
{
    const char body[] = "OK";
    HTTPResponse *res = response_builder(200, "OK", body, strlen(body), "text/plain");
    ASSERT(res != NULL);

    size_t out_len    = 0;
    char *serialized  = httpresponse_serialize(res, &out_len);
    ASSERT(serialized != NULL);
    ASSERT(out_len > 0);

    /* Status line must be present */
    ASSERT(strstr(serialized, "HTTP/1.1 200 OK") != NULL);

    free(serialized);
    httpresponse_free(res);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

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
