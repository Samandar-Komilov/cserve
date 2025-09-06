/**
 * @file    response.c
 * @author  Samandar Komil
 * @date    24 April 2025
 * @brief   HTTPResponse struct and method implementations.
 *
 */

#include "response.h"

HTTPResponse *httpresponse_constructor()
{
    HTTPResponse *res = malloc(sizeof(HTTPResponse));
    if (!res) return NULL;

    res->status_code    = 200;
    res->version        = NULL;
    res->reason_phrase  = NULL;
    res->headers        = NULL;
    res->body           = NULL;
    res->content_type   = NULL;
    res->header_count   = 0;
    res->body_length    = 0;
    res->content_length = 0;

    return res;
}

void httpresponse_free(HTTPResponse *res)
{
    if (!res) return;

    free(res->version);
    free(res->reason_phrase);

    for (int i = 0; i < res->header_count; ++i)
    {
        free(res->headers[i]);
    }
    free(res->headers);

    free(res->body);
    free(res->content_type);
    free(res);
}

int httpresponse_add_header(HTTPResponse *res, const char *key, const char *value)
{
    if (!res || !key || !value) return -1;

    char *header = NULL;
    if (asprintf(&header, "%s: %s", key, value) == -1) return -1;

    char **new_headers = realloc(res->headers, sizeof(char *) * (res->header_count + 1));
    if (!new_headers)
    {
        free(header);
        return -1;
    }

    res->headers                      = new_headers;
    res->headers[res->header_count++] = header;

    return OK;
}

char *httpresponse_serialize(HTTPResponse *res, size_t *out_len)
{
    if (!res || !out_len) return NULL;

    size_t capacity = INITIAL_RESPONSE_SIZE;
    char *buffer    = malloc(capacity); // transferring ownership, caller frees the memory
    if (!buffer) return NULL;

    size_t len = 0;

    // Status line
    len += snprintf(buffer + len, capacity - len, "%s %d %s\r\n", res->version, res->status_code,
                    res->reason_phrase);

    // Headers
    for (int i = 0; i < res->header_count; ++i)
    {
        len += snprintf(buffer + len, capacity - len, "%s\r\n", res->headers[i]);
    }

    // Header/body separator
    len += snprintf(buffer + len, capacity - len, "\r\n");

    // Body
    if (res->body && res->body_length > 0)
    {
        if (len + res->body_length >= capacity)
        {
            buffer = realloc(buffer, len + res->body_length + 1);
            if (!buffer) return NULL;
        }
        memcpy(buffer + len, res->body, res->body_length);
        len += res->body_length;
        buffer[len] = '\0';
    }

    if (out_len) *out_len = len;
    return buffer;
}

HTTPResponse *response_builder(int status_code, const char *phrase, const char *body,
                               size_t body_length, const char *content_type)
{
    if (!phrase || !body || !content_type) return NULL;
    HTTPResponse *response = httpresponse_constructor();

    // Ownership moves: caller frees memory
    response->body = (char *)malloc(body_length);
    if (!response->body)
    {
        httpresponse_free(response);
        return NULL;
    }

    memset(response->body, 0, body_length);

    response->status_code   = status_code;
    response->version       = strdup("HTTP/1.1");
    response->reason_phrase = strdup(phrase);
    memcpy(response->body, body, body_length);
    response->body_length  = body_length;
    response->content_type = strdup(content_type);

    return response;
}