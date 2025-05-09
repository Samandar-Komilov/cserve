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

typedef struct HTTPServer
{
    SocketServer *server;

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
