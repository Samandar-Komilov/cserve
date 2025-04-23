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
} HTTPServer;

HTTPServer *http_request_parser(HTTPServer *httpserver, char *request);
HTTPServer *parse_request_line(HTTPServer *httpserver, char *request_line);
HTTPServer *parse_headers(HTTPServer *httpserver, char *headers);
HTTPServer *parse_body(HTTPServer *httpserver, char *body);

HTTPServer httpserver_constructor(int port);
int httpserver_destructor(HTTPServer *self);

#endif /* HTTPServer_h */
