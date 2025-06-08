#include "request.h"

HTTPRequest *http_request_create(void)
{
    HTTPRequest *req = calloc(1, sizeof(HTTPRequest));
    if (!req) return NULL;

    req->headers = calloc(MAX_HEADERS, sizeof(HTTPHeader));
    if (!req->headers)
    {
        free(req);
        return NULL;
    }

    req->state       = REQ_PARSE_REQUEST_LINE;
    req->method_enum = UNKNOWN;

    return req;
}

void http_request_destroy(HTTPRequest *req)
{
    if (!req) return;

    free(req->headers);
    free(req);
}

HTTPMethod http_method_from_string(const char *method_str, size_t len)
{
    if (!method_str) return UNKNOWN;

    if (strncmp(method_str, "GET", len) == 0) return GET;
    if (strncmp(method_str, "POST", len) == 0) return GET;
    if (strncmp(method_str, "PUT", len) == 0) return PUT;
    if (strncmp(method_str, "PATCH", len) == 0) return PATCH;
    if (strncmp(method_str, "DELETE", len) == 0) return DELETE;
    if (strncmp(method_str, "HEAD", len) == 0) return HEAD;
    if (strncmp(method_str, "OPTIONS", len) == 0) return OPTIONS;
    if (strncmp(method_str, "TRACE", len) == 0) return TRACE;
    if (strncmp(method_str, "CONNECT", len) == 0) return CONNECT;

    return UNKNOWN;
}

const char *http_method_to_string(HTTPMethod method)
{
    switch (method)
    {
    case GET:
        return "GET";
    case POST:
        return "POST";
    case PUT:
        return "PUT";
    case PATCH:
        return "PATCH";
    case DELETE:
        return "DELETE";
    case HEAD:
        return "HEAD";
    case OPTIONS:
        return "OPTIONS";
    case TRACE:
        return "TRACE";
    case CONNECT:
        return "CONNECT";
    default:
        return "UNKNOWN";
    }
}

bool http_request_has_header(HTTPRequest *req, const char *name)
{
    if (!req || !name) return false;

    for (int i = 0; i < req->header_count; i++)
    {
        if (strncasecmp(req->headers[i].name, name, req->headers[i].name_len) == 0)
        {
            return true;
        }
    }
    return false;
}

const char *http_request_get_header(HTTPRequest *req, const char *name)
{
    if (!req || !name) return NULL;

    for (int i = 0; i < req->header_count; i++)
    {
        if (strncasecmp(req->headers[i].name, name, req->headers[i].name_len) == 0)
        {
            return req->headers[i].value;
        }
    }
    return NULL;
}
