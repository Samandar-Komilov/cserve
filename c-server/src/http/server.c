/**
 * @file    HTTPServer.c
 * @author  Samandar Komil
 * @date    19 April 2025
 * @brief   HTTPServer methods and utility function implementations
 *
 */

#include "server.h"
#include "request.h"
#include "response.h"

/**
 * @brief   Launches the HTTP server, binding and listening on the port specified in
 *          the HTTPServer struct.
 *
 * @param   self  The HTTPServer struct containing the server config and methods.
 *
 * @returns An error code indicating the success or failure of the server launch.
 *
 * This function takes care of binding and listening on the configured port. It
 * then enters an infinite loop, accepting and processing incoming requests.
 *
 * The function will block until a request is received, process it with the
 * configured request_handler, and then send the response back to the client.
 *
 * If the bind or listen calls fail, an error code is returned. Otherwise, the
 * function will run indefinitely until the program is terminated.
 */
int launch(HTTPServer *self)
{
    if (bind(self->server->socket, (struct sockaddr *)&self->server->address,
             sizeof(self->server->address)) < 0)
    {
        return SOCKET_BIND_ERROR;
    }

    if ((listen(self->server->socket, self->server->queue)) < 0)
    {
        return SOCKET_LISTEN_ERROR;
    }

    printf("\033[32m===== Waiting for connections on port %d =====\033[0m\n", self->server->port);

    while (1)
    {
        char buffer[MAX_BUFFER_SIZE];
        int address_length = sizeof(self->server->address);
        int new_socket     = accept(self->server->socket, (struct sockaddr *)&self->server->address,
                                    (socklen_t *)&address_length);
        read(new_socket, buffer, MAX_BUFFER_SIZE);
        printf("%s\n", buffer);

        HTTPRequest *httprequest_ptr = self->parse_http_request(self, buffer);

        HTTPResponse *httpresponse_ptr = self->request_handler(httprequest_ptr);

        char *response = httpresponse_serialize(httpresponse_ptr, NULL);

        // caller of the request_handler is responsible for freeing the memory
        httpresponse_free(httpresponse_ptr);

        printf("===== Response:\n%s", response);

        write(new_socket, response, strlen(response));

        // Caller owned malloced buffer, so frees it now
        free(response);

        close(new_socket);
    }
}

/**
 * @brief   Parse an HTTP request string into an HTTPRequest struct.
 *
 * @param   httpserver_ptr  The HTTPServer struct containing the methods to parse the request.
 * @param   request         The raw HTTP request string.
 *
 * @returns An HTTPRequest struct containing the parsed request information.
 *
 * This function takes an HTTP request string and breaks it into its components, storing them
 * in an HTTPRequest struct. The HTTPRequest struct is dynamically allocated and the caller
 * is responsible for freeing it.
 */
HTTPRequest *parse_http_request(HTTPServer *httpserver_ptr, char *request)
{
    HTTPRequest *request_ptr = httprequest_constructor();

    char *token = strtok(request, "\r\n");

    // 1. Request Line
    httpserver_ptr->parse_request_line(request_ptr, token);
    token = strtok(NULL, "\r\n");

    // 2. Headers
    httpserver_ptr->parse_headers(request_ptr, token);
    token = strtok(NULL, "\r\n");

    // 3. Body
    httpserver_ptr->parse_body(request_ptr, token, request_ptr->body_length);

    return request_ptr;
}

/**
 * @brief   Parse the request line of an HTTP request and populate the request fields.
 * @param   req   The HTTP request to populate.
 * @param   line  The request line as a string.
 *
 * The request line is expected to be in the format:
 * <method> <path> <version>
 *
 * For example: GET / HTTP/1.1
 *
 * @note    The method, path, and version are expected to be separated by spaces.
 */
void parse_request_line(HTTPRequest *req, char *line)
{
    char *method  = strtok(line, " ");
    char *path    = strtok(NULL, " ");
    char *version = strtok(NULL, " ");

    req->method  = strdup(method);
    req->path    = strdup(path);
    req->version = strdup(version);
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

/**
 * @brief   Handle an HTTP request and generate an HTTP response.
 *
 * @param   request_ptr  A pointer to the HTTPRequest struct containing the request details.
 *
 * @returns A pointer to an HTTPResponse struct representing the generated response.
 *
 * This function processes the incoming HTTP request, prints the request path for logging,
 * and constructs a basic HTTP response with a status code of 200, HTTP version 1.1,
 * and a "Hello, World!" body. The caller is responsible for freeing the memory allocated
 * for the HTTPResponse.
 */
HTTPResponse *request_handler(HTTPRequest *request_ptr)
{
    // Simulate request processing (HTML response or proxy)
    printf(">>> Processing the request...: %s\n", request_ptr->path);

    // Ownership of returned HTTPResponse* is transferred to caller
    // The caller is responsible for freeing the memory
    HTTPResponse *response = httpresponse_constructor();

    response->status_code   = 200;
    response->version       = strdup("HTTP/1.1");
    response->reason_phrase = strdup("OK");
    response->body          = strdup("Hello, World!");
    response->body_length   = strlen(response->body);

    return response;
}

HTTPServer *httpserver_constructor(int port)
{
    HTTPServer *httpserver_ptr = (HTTPServer *)malloc(sizeof(HTTPServer));

    SocketServer *SockServer = server_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, port, 10);
    httpserver_ptr->server   = SockServer;

    httpserver_ptr->launch             = launch;
    httpserver_ptr->parse_http_request = parse_http_request;
    httpserver_ptr->parse_request_line = parse_request_line;
    httpserver_ptr->parse_headers      = parse_headers;
    httpserver_ptr->parse_body         = parse_body;
    httpserver_ptr->request_handler    = request_handler;

    return httpserver_ptr;
}

void httpserver_destructor(HTTPServer *httpserver_ptr)
{
    if (httpserver_ptr->server != NULL)
    {
        server_destructor(httpserver_ptr->server);
    }

    free(httpserver_ptr);
}