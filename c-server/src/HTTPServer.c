/**
 * @file    HTTPServer.c
 * @author  Samandar Komil
 * @date    19 April 2025
 * @brief   HTTPServer methods and utility function implementations
 *
 */

#include <string.h>
#include "HTTPServer.h"

HTTPServer *http_request_parser(HTTPServer *httpserver, char *request)
{
    /**
     * @Plan
     *
     * - Parse request line
     * - Parse request headers
     * - Parse request body
     *
     * GET /index.html HTTP/1.1
     * Host: example.com
     * User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko)
     * Chrome/58.0.3029.110 Safari/537.36 Accept: text/html,application/json Accept-Language:
     * en-US,en;q=0.8
     * {
     *     "name": "John",
     * }
     */

    char *token = strtok(request, "\r\n");

    // 1. Request Line
    parse_request_line(httpserver, token);
    token = strtok(NULL, "\r\n");

    // 2. Headers
    parse_headers(httpserver, token);
    token = strtok(NULL, "\r\n");

    // 3. Body
    parse_body(httpserver, token);

    return httpserver;
}

HTTPServer *parse_request_line(HTTPServer *httpserver, char *request_line)
{
    char method[10];
    char *path;
    char version[10];

    // 1. HTTP request type
    char *token = strtok(request_line, " ");
    strncpy(method, token, strlen(token));

    // 2. HTTP request path (we need splits again)
    parse_request_path(httpserver, token);

    // 3. HTTP version
    char *token = strtok(NULL, " ");
    strncpy(version, token, strlen(token));

    return httpserver;
}

HTTPServer *parse_headers(HTTPServer *httpserver, char *headers)
{
    return httpserver;
}

HTTPServer *parse_body(HTTPServer *httpserver, char *body)
{
    return httpserver;
}

// HTTPServer constructor
HTTPServer httpserver_constructor(int port)
{
    HTTPServer HTTPServer;

    Server TCPServer  = server_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, port, 10);
    HTTPServer.server = TCPServer;
    return NULL;
}
