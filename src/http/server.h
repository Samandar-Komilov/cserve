/**
 * @file    HTTPServer.h
 * @author  Samandar Komil
 * @date    19 April 2025
 * @brief   HTTP Server struct and method prototypes.
 *
 */

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "sock/server.h"
#include "parsers.h"
#include "common.h"
#include "request.h"

typedef struct Connection
{
    int socket;          // client socket
    char *buffer;        // dynamic buffer for request
    size_t buffer_size;  // allocated size for buffer
    size_t len;          // current data length of buffer
    size_t parsed_bytes; // bytes parsed
    ParseState state;    // parsing state
    HTTPRequest request; // parsed request
} Connection;

int init_connection(Connection *conn, int client_fd, int epoll_fd);
int free_connection(Connection *conn, int client_fd, int epoll_fd);
int reset_connection(Connection *conn);

typedef struct HTTPServer
{
    SocketServer *server;
    Connection *connections;
    size_t active_count;
    int epoll_fd;

    char *static_dir;
    char **proxy_backends;
    int backend_count;

    int (*launch)(struct HTTPServer *self);
} HTTPServer;

HTTPResponse *request_handler(HTTPRequest *request_ptr);
int connect_to_backend(const char *host, const char *port);

HTTPServer *httpserver_constructor(int port, char *static_dir, char **proxy_backends,
                                   int backend_count);
void httpserver_destructor(HTTPServer *httpserver_ptr);

#endif
