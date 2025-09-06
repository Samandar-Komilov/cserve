/**
 * @file    request.h
 * @author  Samandar Komil
 * @date    26 April 2025
 * @brief   HTTP request parser
 *
 */

#include "request.h"
#include "response.h"

int parse_request_line(HTTPRequest *req_t, const char *reqstr, size_t len);
int parse_header(HTTPHeader *header, const char *line, size_t len);
int parse_http_request(const char *data, size_t len, HTTPRequest *req);
void print_request(const HTTPRequest *req);
const char *get_mime_type(const char *filepath);