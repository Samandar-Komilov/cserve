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

typedef struct HTTPServer
{
    Server *server;

    // Methods (function pointers)
    int (*launch)(struct HTTPServer *self);
    struct HTTPServer *(*parse_http_request)(struct HTTPServer *self, char *request);
    HTTPRequest *(*parse_request_line)(HTTPRequest *request, char *request_str);
    HTTPRequest *(*parse_headers)(HTTPRequest *request, char *request_str);
    HTTPRequest *(*parse_body)(HTTPRequest *request, char *request_str);
} HTTPServer;

HTTPServer *httpserver_constructor(int port);
void httpserver_destructor(HTTPServer *httpserver_ptr);

#endif /* HTTPSERVER_H */
