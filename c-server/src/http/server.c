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

void parse_request_line(HTTPRequest *req, char *line)
{
    char *method  = strtok(line, " ");
    char *path    = strtok(NULL, " ");
    char *version = strtok(NULL, " ");

    req->method  = strdup(method);
    req->path    = strdup(path);
    req->version = strdup(version);
}

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