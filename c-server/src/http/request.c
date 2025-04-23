/**
 * @file    request.c
 * @author  Samandar Komil
 * @date    24 April 2025
 * @brief   HTTPRequest struct and method implementations.
 *
 */

#include "../common.h"
#include "request.h"

HTTPRequest *httprequest_constructor()
{
    HTTPRequest *request = (HTTPRequest *)malloc(sizeof(HTTPRequest));

    return request;
}