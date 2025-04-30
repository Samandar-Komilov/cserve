/**
 * @file    response.h
 * @author  Samandar Komil
 * @date    24 April 2025
 * @brief   HTTPResponse struct and method prototypes.
 *
 */

#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "common.h"

typedef struct
{
    int status_code;
    char *version;
    char *reason_phrase;
    char **headers;
    int header_count;
    char *body;
    int body_length;
} HTTPResponse;

HTTPResponse *httpresponse_constructor();
void httpresponse_free(HTTPResponse *httpresponse_ptr);

int httpresponse_add_header(HTTPResponse *res, const char *key, const char *value);
char *httpresponse_serialize(HTTPResponse *res, size_t *out_len);

HTTPResponse *response_builder(int status_code, char *phrase, char *body);

#endif