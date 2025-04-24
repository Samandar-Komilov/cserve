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

    res->status_code   = 200;
    res->version       = strdup("HTTP/1.1");
    res->reason_phrase = strdup("OK");
    res->headers       = NULL;
    res->header_count  = 0;
    res->body          = NULL;
    res->body_length   = 0;

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
    free(res);
}

/**
 * @brief Helper functions:
 * - Dynamically Add Headers with realloc()
 * - Serialize HTTPResponse to raw string
 *
 */

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

    return 0;
}

char *httpresponse_serialize(HTTPResponse *res, size_t *out_len)
{
    if (!res) return NULL;

    size_t capacity = HTTPRESPONSE_CAPACITY;
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