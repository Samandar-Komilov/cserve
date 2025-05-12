/**
 * @file    request.h
 * @author  Samandar Komil
 * @date    26 April 2025
 * @brief   HTTP request parser
 *
 */

#include "request.h"
#include "response.h"

HTTPRequest *parse_http_request(char *request_str);
int parse_request_line(HTTPRequest *request_ptr, char *request_line);
void parse_headers(HTTPRequest *request_ptr, char *headers);
void parse_body(HTTPRequest *request_ptr, char *body, int content_length);
const char *get_mime_type(const char *filepath);