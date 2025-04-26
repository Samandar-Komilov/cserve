/**
 * @file    request.h
 * @author  Samandar Komil
 * @date    24 April 2025
 * @brief   HTTPRequest struct and method prototypes.
 *
 */

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "common.h"

typedef struct Header
{
    char *key;
    char *value;
} Header;

typedef struct HTTPRequest
{
    char *method;
    char *path;
    char *version;

    Header *headers;
    int header_count;

    char *body;
    size_t body_length;
} HTTPRequest;

HTTPRequest *httprequest_constructor();
void httprequest_free(HTTPRequest *req);

#endif