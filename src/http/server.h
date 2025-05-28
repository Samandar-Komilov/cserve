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
    int socket;                // client socket
    char *buffer;              // dynamic buffer for request
    size_t buffer_size;        // allocated size for buffer
    size_t buffer_len;         // current data length of buffer
    ConnectionState state;     // connection state
    time_t last_active;        // last active time
    HTTPRequest *curr_request; // current request
    int requests_handled;      // number of requests handled so far
    bool keep_alive;           // keep-alive?
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
    int is_running;
    struct epoll_event events[MAX_EPOLL_EVENTS];

    int (*launch)(struct HTTPServer *self);
} HTTPServer;

HTTPServer *httpserver_init(int port);
int httpserver_setup(HTTPServer *httpserver_ptr);
void httpserver_cleanup(HTTPServer *httpserver_ptr);

static int set_nonblocking(int fd);
static int optimize_server_socket(int fd);

int handle_new_connection(HTTPServer *server_ptr);
HTTPResponse *request_handler(HTTPRequest *request_ptr);
int connect_to_backend(const char *host, const char *port);

#endif
