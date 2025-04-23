/**
 * @file    HTTPServer.h
 * @author  Samandar Komil
 * @date    19 April 2025
 * @brief   HTTP Server struct and method prototypes.
 *
 */

#ifndef HTTPServer_h
#define HTTPServer_h

#include "Server.h"

typedef struct HTTPServer
{
    Server *server;
    char *request[3];
    char **headers;
    char *body;

    // Methods (function pointers)
    void (*launch)(struct HTTPServer *self);
    struct HTTPServer *(*parse_http_request)(struct HTTPServer *self, char *request);
} HTTPServer;

HTTPServer *parse_http_request(HTTPServer *httpserver_ptr, char *request);
HTTPServer *parse_request_line(HTTPServer *httpserver_ptr, char *request_line);
HTTPServer *parse_headers(HTTPServer *httpserver_ptr, char *headers);
HTTPServer *parse_body(HTTPServer *httpserver_ptr, char *body);

HTTPServer *httpserver_constructor(int port);
void httpserver_destructor(HTTPServer *httpserver_ptr);

#endif /* HTTPServer_h */
