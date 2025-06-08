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

typedef struct HTTPRequestLine
{
    char *method;
    char *uri;
    char *protocol;
    size_t method_len;
    size_t uri_len;
    size_t protocol_len;
} HTTPRequestLine;

typedef struct HTTPHeader
{
    char *name;
    char *value;
    size_t name_len;
    size_t value_len;
} HTTPHeader;

typedef struct HTTPRequest
{
    HTTPRequestLine request_line;
    HTTPHeader *headers;
    int header_count;
    char *body;
    size_t body_len;
    HTTPRequestState state;
    HTTPMethod method_enum;
} HTTPRequest;

HTTPRequest *http_request_create(void);
void http_request_destroy(HTTPRequest *req);
HTTPMethod http_method_from_string(const char *method_str, size_t len);
const char *http_method_to_string(HTTPMethod method);
bool http_request_has_header(HTTPRequest *req, const char *name);
const char *http_request_get_header(HTTPRequest *req, const char *name);

#endif