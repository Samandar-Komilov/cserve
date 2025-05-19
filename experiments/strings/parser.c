#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_METHOD 16
#define MAX_URI 2048
#define MAX_PROTOCOL 32
#define MAX_HEADER_NAME 256
#define MAX_HEADER_VALUE 4096
#define MAX_HEADERS 100

// -------- Structs

typedef struct HTTPRequestLine
{
    char *method;
    char *uri;
    char *protocol;
    size_t method_len;
    size_t uri_len;
    size_t protocol_len;
} HTTPRequestLine;

typedef struct HTTPHeader
{
    char *name;
    char *value;
    size_t name_len;
    size_t value_len;

} HTTPHeader;

typedef struct HTTPRequest
{
    HTTPRequestLine request_line;
    HTTPHeader headers[MAX_HEADERS];
    int header_count;
    char *body;
    size_t body_len;
} HTTPRequest;

// ------- Methods

int parse_request_line(HTTPRequest *req_t, const char *reqstr, size_t len)
{
    if (!req_t || !reqstr) return -1;
    printf("Req parser is working.\n");
    printf("%s\n", reqstr);

    // Putting pointers at start and end of the incoming string
    const char *ptr = reqstr;
    const char *end = reqstr + len;

    // Parse method (put pointer to buffer)
    const char *space = memchr(ptr, ' ', end - ptr);
    if (!space) return -1;
    req_t->request_line.method     = (char *)ptr;
    req_t->request_line.method_len = space - ptr;
    ptr                            = space + 1;

    // Parse URI (put pointer to buffer)
    space = memchr(ptr, ' ', end - ptr);
    if (!space) return -1;
    req_t->request_line.uri     = (char *)ptr;
    req_t->request_line.uri_len = space - ptr;
    ptr                         = space + 1;

    // Parse protocol (put pointer to buffer)
    const char *crlf = memmem(ptr, end - ptr, "\r\n", 2);
    if (!crlf) return -1;
    req_t->request_line.protocol     = (char *)ptr;
    req_t->request_line.protocol_len = crlf - ptr;
    ptr                              = crlf + 2;

    return ptr - reqstr;
}

int parse_header(HTTPHeader *header, const char *line, size_t len)
{
    const char *ptr = line;
    const char *end = line + len;

    // Find colon
    const char *colon = memchr(ptr, ':', end - ptr);
    if (!colon) return -1;
    header->name     = (char *)ptr;
    header->name_len = colon - ptr;
    ptr              = colon + 1;

    // Skip whitespaces
    while (ptr < end && isspace(*ptr))
        ptr++;

    // Find end of line
    const char *crlf = memmem(ptr, end - ptr, "\r\n", 2);
    if (!crlf) return -1;
    header->value     = (char *)ptr;
    header->value_len = crlf - ptr;
    ptr               = crlf + 2;

    return ptr - line;
}

int parse_http_request(const char *data, size_t len, HTTPRequest *req)
{
    const char *ptr = data;
    const char *end = data + len;
    int consumed;

    consumed = parse_request_line(req, ptr, end - ptr);
    if (consumed < 0) return -1;
    ptr += consumed;

    req->header_count = 0;
    while (ptr < end && memcmp(ptr, "\r\n", 2) != 0)
    {
        if (req->header_count >= MAX_HEADERS) return -1;
        consumed = parse_header(&req->headers[req->header_count], ptr, end - ptr);
        if (consumed < 0) return -1;
        ptr += consumed;
        req->header_count++;
    }

    if (ptr + 2 > end || memcmp(ptr, "\r\n", 2) != 0) return -1;
    ptr += 2;

    // Body
    req->body     = (char *)ptr;
    req->body_len = end - ptr;

    return 0;
}

void print_request(const HTTPRequest *req)
{
    printf("Method: %.*s\n", (int)req->request_line.method_len, req->request_line.method);
    printf("URI: %.*s\n", (int)req->request_line.uri_len, req->request_line.uri);
    printf("Protocol: %.*s\n", (int)req->request_line.protocol_len, req->request_line.protocol);
    for (int i = 0; i < req->header_count; i++)
    {
        printf("Header: %.*s = %.*s\n", (int)req->headers[i].name_len, req->headers[i].name,
               (int)req->headers[i].value_len, req->headers[i].value);
    }
    if (req->body_len > 0)
    {
        printf("Body: %.*s\n", (int)req->body_len, req->body);
    }
}

// ------------------------------------------------------------

int main(void)
{
    const char *request = "GET /index.html HTTP/1.1\r\n"
                          "Host: example.com\r\n"
                          "Content-Length: 13\r\n"
                          "\r\n"
                          "Hello, World!";
    HTTPRequest req     = {0};

    if (parse_http_request(request, strlen(request), &req) == 0)
    {
        print_request(&req);
    }
    else
    {
        printf("Parsing failed\n");
    }
    return 0;
}