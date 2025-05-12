/**
 * @file    request.c
 * @author  Samandar Komil
 * @date    26 April 2025
 * @brief   HTTP request parser implementations
 *
 */

#include "parsers.h"

/**
 * @brief   Parse an HTTP request string into an HTTPRequest struct.
 *
 * @param   request         The raw HTTP request string.
 *
 * @returns An HTTPRequest struct containing the parsed request information.
 *
 * This function takes an HTTP request string and breaks it into its components, storing them
 * in an HTTPRequest struct. The HTTPRequest struct is dynamically allocated and the caller
 * is responsible for freeing it.
 */
HTTPRequest *parse_http_request(char *request_str)
{
    HTTPRequest *request_ptr = httprequest_constructor();

    char *token = strtok(request_str, "\r\n");

    // 1. Request Line
    parse_request_line(request_ptr, token);
    token = strtok(NULL, "\r\n");

    // 2. Headers
    parse_headers(request_ptr, token);
    token = strtok(NULL, "\r\n");

    // 3. Body
    parse_body(request_ptr, token, request_ptr->body_length);

    return request_ptr;
}

/**
 * @brief   Parse the request line of an HTTP request string and populate the
 *          fields of an HTTPRequest struct.
 *
 * @param   req       The HTTPRequest struct to populate.
 * @param   line      The request line as a string.
 *
 * @returns An error code indicating the success or failure of the parsing.
 *
 * The request line is expected to be in the format:
 * <method> <path> <version>
 *
 * For example: GET /index.html HTTP/1.1
 *
 * @note    The method must be one of the following: GET, POST, PUT, PATCH, DELETE,
 *          OPTIONS, HEAD, CONNECT, TRACE
 *
 * @note    The path must begin with a forward slash.
 */
int parse_request_line(HTTPRequest *req, char *line)
{
    char *method  = strtok(line, " ");
    char *path    = strtok(NULL, " ");
    char *version = strtok(NULL, " ");

    if (!method || !path || !version)
    {
        printf("Invalid request line: %s\n", line);
        return INVALID_REQUEST_LINE;
    }

    if (strcmp(method, "GET") != 0 && strcmp(method, xstr(POST)) != 0 &&
        strcmp(method, xstr(PUT)) != 0 && strcmp(method, xstr(PATCH)) != 0 &&
        strcmp(method, xstr(DELETE)) != 0 && strcmp(method, xstr(OPTIONS)) != 0 &&
        strcmp(method, xstr(HEAD)) != 0 && strcmp(method, xstr(CONNECT)) != 0 &&
        strcmp(method, xstr(TRACE)) != 0)
    {
        printf("Invalid request method: %s\n", method);
        return INVALID_METHOD;
    }

    if (strncmp(path, "/", 1) != 0)
    {
        printf("Invalid request path: %s\n", path);
        return INVALID_PATH;
    }
    req->method  = strdup(method);
    req->path    = strdup(path);
    req->version = strdup(version);

    return OK;
}

/**
 * @brief   Parse the headers of an HTTP request and populate the request fields.
 * @param   req       The HTTP request to populate.
 * @param   raw_headers  The raw headers as a string.
 *
 * The headers are expected to be in the format:
 * <header_name>: <value>
 *
 * For example: Content-Type: text/html
 *
 * @note    The header name and value are expected to be separated by a colon.
 */
void parse_headers(HTTPRequest *req, char *raw_headers)
{
    char *line = strtok(raw_headers, "\r\n");
    while (line && req->header_count < MAX_HEADERS)
    {
        char *colon = strchr(line, ':');
        if (!colon) break;

        // Header: value -> Header:\0value
        // line = Header, value = colon + 1 = value
        *colon      = '\0';
        char *key   = line;
        char *value = colon + 1;

        // Trim spaces
        while (*value == ' ')
            value++;

        req->headers[req->header_count].key   = strdup(key);
        req->headers[req->header_count].value = strdup(value);
        req->header_count++;

        line = strtok(NULL, "\r\n");
    }
}

/**
 * @brief   Parse the body of an HTTP request and populate the request fields.
 * @param   req           The HTTP request to populate.
 * @param   raw_body      The raw body as a string.
 * @param   content_length  The length of the body as specified in the Content-Length header.
 *
 * The body is expected to be in the request raw string after the headers.
 *
 * @note    The body is copied to a new memory location, and the original memory is not modified.
 */
void parse_body(HTTPRequest *req, char *raw_body, int content_length)
{
    if (!raw_body || content_length == 0)
    {
        req->body        = NULL;
        req->body_length = 0;
        return;
    }

    req->body = malloc(content_length + 1);

    memcpy(req->body, raw_body, content_length);
    req->body[content_length] = '\0';
    req->body_length          = content_length;
}

const char *get_mime_type(const char *filepath)
{
    const char *ext = strrchr(filepath, '.');
    if (!ext) return "application/octet-stream";

    ext++; // skip the dot
    if (strcmp(ext, "html") == 0) return "text/html";
    if (strcmp(ext, "css") == 0) return "text/css";
    if (strcmp(ext, "js") == 0) return "application/javascript";
    if (strcmp(ext, "png") == 0) return "image/png";
    if (strcmp(ext, "jpg") == 0) return "image/jpeg";
    if (strcmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, "gif") == 0) return "image/gif";
    if (strcmp(ext, "svg") == 0) return "image/svg+xml";
    if (strcmp(ext, "ico") == 0) return "image/x-icon";
    if (strcmp(ext, "json") == 0) return "application/json";
    if (strcmp(ext, "pdf") == 0) return "application/pdf";

    return "application/octet-stream";
}
