/**
 * @file    HTTPServer.c
 * @author  Samandar Komil
 * @date    19 April 2025
 * @brief   HTTPServer methods and utility function implementations
 *
 */

#include "server.h"

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

        /**
         * HTTPRequest httprequest_ptr = self->parse_http_request(self, buffer);
         *
         * HTTPResponse httpresponse_ptr = self->request_handler(self, httprequest_ptr);
         *
         */

        char *response = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/plain\r\n"
                         "Content-Length: 13\r\n"
                         "\r\n"
                         "Hello, World!";

        write(new_socket, response, strlen(response));
        close(new_socket);
    }
}

HTTPServer *parse_http_request(HTTPServer *httpserver_ptr, char *request)
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
    httpserver_ptr->parse_body(request_ptr, token);

    return httpserver_ptr;
}

HTTPRequest *parse_request_line(HTTPRequest *request_ptr, char *request_line)
{
    char *method  = (char *)malloc(10 * sizeof(char));
    char *path    = (char *)malloc(100 * sizeof(char));
    char *version = (char *)malloc(10 * sizeof(char));

    // 1. HTTP request type
    char *token = strtok(request_line, " ");
    strcpy(method, token);

    // 2. HTTP request path (we need splits again)
    token = strtok(NULL, " ");
    strcpy(path, token);
    // parse_request_path(httpserver, token);

    // 3. HTTP version
    token = strtok(NULL, " ");
    strcpy(version, token);

    printf("Request Line:\n%s %s %s\n", method, path, version);

    request_ptr->request_line[0] = method;
    request_ptr->request_line[1] = path;
    request_ptr->request_line[2] = version;

    return request_ptr;
}

HTTPRequest *parse_headers(HTTPRequest *request_ptr, char *headers)
{
    printf("Headers:\n%s\n", headers);
    return request_ptr;
}

HTTPRequest *parse_body(HTTPRequest *request_ptr, char *body)
{
    printf("Body:\n%s\n", body);
    return request_ptr;
}

// HTTPServer constructor
HTTPServer *httpserver_constructor(int port)
{
    HTTPServer *httpserver_ptr = (HTTPServer *)malloc(sizeof(HTTPServer));

    Server *TCPServer      = server_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, port, 10);
    httpserver_ptr->server = TCPServer;

    httpserver_ptr->launch             = launch;
    httpserver_ptr->parse_http_request = parse_http_request;
    httpserver_ptr->parse_request_line = parse_request_line;
    httpserver_ptr->parse_headers      = parse_headers;
    httpserver_ptr->parse_body         = parse_body;

    return httpserver_ptr;
}

void httpserver_destructor(HTTPServer *httpserver_ptr)
{
    if (httpserver_ptr)
    {
        server_destructor(httpserver_ptr->server);
        free(httpserver_ptr);
    }
}