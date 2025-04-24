/**
 * @file    HTTPServer.h
 * @author  Samandar Komil
 * @date    19 April 2025
 * @brief   HTTP Server struct and method prototypes.
 *
 */

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "../server.h"
#include "request.h"
#include "response.h"

typedef struct HTTPServer
{
    SocketServer *server;

    int (*launch)(struct HTTPServer *self);
    HTTPRequest *(*parse_http_request)(struct HTTPServer *self, char *request);
    void (*parse_request_line)(HTTPRequest *request, char *request_str);
    void (*parse_headers)(HTTPRequest *request, char *request_str);
    void (*parse_body)(HTTPRequest *request, char *request_str, int content_length);
    HTTPResponse *(*request_handler)(HTTPRequest *request_ptr);
} HTTPServer;

HTTPServer *httpserver_constructor(int port);
void httpserver_destructor(HTTPServer *httpserver_ptr);

#endif
