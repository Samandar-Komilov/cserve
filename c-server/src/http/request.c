/**
 * @file    request.c
 * @author  Samandar Komil
 * @date    24 April 2025
 * @brief   HTTPRequest struct and method implementations.
 *
 */

#include "../common.h"
#include "request.h"

HTTPRequest *httprequest_constructor()
{
    HTTPRequest *req = malloc(sizeof(HTTPRequest));
    req->method      = NULL;
    req->path        = NULL;
    req->version     = NULL;

    req->headers      = malloc(sizeof(Header) * MAX_HEADERS); // or use realloc dynamically
    req->header_count = 0;

    req->body        = NULL;
    req->body_length = 0;
    return req;
}

void httprequest_free(HTTPRequest *req)
{
    free(req->method);
    free(req->path);
    free(req->version);

    for (int i = 0; i < req->header_count; ++i)
    {
        free(req->headers[i].key);
        free(req->headers[i].value);
    }
    free(req->headers);

    free(req->body);
    free(req);
}