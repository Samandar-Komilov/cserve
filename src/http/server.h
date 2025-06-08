/**
 * @file    HTTPServer.h
 * @author  Samandar Komil
 * @date    19 April 2025
 * @brief   HTTP Server struct and method prototypes.
 *
 */

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "common.h"
#include "sock/server.h"
#include "connection/connection.h"
#include "connection/pool.h"
#include "http/parsers.h"
#include "http/request.h"
#include "http/handlers.h"

typedef struct HTTPServer
{
    SocketServer *server;
    ConnectionPool *conn_pool;
    int epoll_fd;
    int is_running;
    struct epoll_event events[MAX_EPOLL_EVENTS];

    int (*launch)(struct HTTPServer *self);
} HTTPServer;

HTTPServer *httpserver_init(int port);
int httpserver_setup(HTTPServer *httpserver_ptr);
void httpserver_cleanup(HTTPServer *httpserver_ptr);

int server_socket_epoll_add(HTTPServer *self);
int handle_new_connection(HTTPServer *server_ptr);

#endif
