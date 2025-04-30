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

typedef struct HTTPServer
{
    SocketServer *server;

    char *static_dir;

    int (*launch)(struct HTTPServer *self);
} HTTPServer;

HTTPResponse *request_handler(HTTPRequest *request_ptr);
HTTPServer *httpserver_constructor(int port);
void httpserver_destructor(HTTPServer *httpserver_ptr);

#endif
