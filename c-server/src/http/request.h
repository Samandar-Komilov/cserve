/**
 * @file    request.h
 * @author  Samandar Komil
 * @date    24 April 2025
 * @brief   HTTPRequest struct and method prototypes.
 *
 */

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

typedef struct HTTPRequest
{
    char *request_line[3];
    char **headers;
    char *body;
} HTTPRequest;

HTTPRequest *httprequest_constructor();

#endif