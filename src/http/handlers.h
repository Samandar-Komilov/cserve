#ifndef HTTP_HANDLERS_H
#define HTTP_HANDLERS_H

#include "http/request.h"
#include "http/response.h"
#include "http/parsers.h"

HTTPResponse *http_handle_request(HTTPRequest *request_ptr);
int connect_to_backend(const char *host, const char *port);

#endif