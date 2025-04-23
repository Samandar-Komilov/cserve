/**
 * @file    HTTPServer.c
 * @author  Samandar Komil
 * @date    19 April 2025
 * @brief   HTTPServer methods and utility function implementations
 *
 */

#include "HTTPServer.h"

void launch(HTTPServer *self)
{
    if (bind(self->server->socket, (struct sockaddr *)&self->server->address,
             sizeof(self->server->address)) < 0)
    {
        perror("Failed to bind socket...");
        exit(1);
    }

    if ((listen(self->server->socket, self->server->queue)) < 0)
    {
        perror("Failed to listen socket...");
        exit(1);
    }

    printf("\033[32m===== Waiting for connections on port %d =====\033[0m\n", self->server->port);

    while (1)
    {
        char buffer[MAX_BUFFER_SIZE];
        int address_length = sizeof(self->server->address);
        int new_socket     = accept(self->server->socket, (struct sockaddr *)&self->server->address,
                                    (socklen_t *)&address_length);
        read(new_socket, buffer, MAX_BUFFER_SIZE);
        // printf("%s\n", buffer);
        self->parse_http_request(self, buffer);

        // printf("%s %s %s", self->request[0], self->request[1], self->request[2]);

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
    char *token = strtok(request, "\r\n");

    // 1. Request Line
    parse_request_line(httpserver_ptr, token);
    token = strtok(NULL, "\r\n");

    // 2. Headers
    parse_headers(httpserver_ptr, token);
    token = strtok(NULL, "\r\n");

    // 3. Body
    parse_body(httpserver_ptr, token);

    return httpserver_ptr;
}

HTTPServer *parse_request_line(HTTPServer *httpserver_ptr, char *request_line)
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

    httpserver_ptr->request[0] = method;
    httpserver_ptr->request[1] = path;
    httpserver_ptr->request[2] = version;

    return httpserver_ptr;
}

HTTPServer *parse_headers(HTTPServer *httpserver_ptr, char *headers)
{
    printf("Headers:\n%s\n", headers);
    return httpserver_ptr;
}

HTTPServer *parse_body(HTTPServer *httpserver_ptr, char *body)
{
    printf("Body:\n%s\n", body);
    return httpserver_ptr;
}

// HTTPServer constructor
HTTPServer *httpserver_constructor(int port)
{
    HTTPServer *httpserver_ptr = (HTTPServer *)malloc(sizeof(HTTPServer));

    Server *TCPServer      = server_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, port, 10);
    httpserver_ptr->server = TCPServer;

    httpserver_ptr->launch             = launch;
    httpserver_ptr->parse_http_request = parse_http_request;

    return httpserver_ptr;
}

void httpserver_destructor(HTTPServer *httpserver_ptr)
{
    if (httpserver_ptr)
    {
        server_destructor(httpserver_ptr->server);
        // If you allocate headers/body dynamically, free them too:
        // free(httpserver->headers); free(httpserver->body);
        free(httpserver_ptr);
    }
}