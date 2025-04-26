/**
 * @file    common.h
 * @author  Samandar Komil
 * @date    24 April 2025
 * @brief   Common definitions
 *
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define str(x) #x
#define xstr(x) str(x)

#define MAX_BUFFER_SIZE 30000
#define MAX_HEADERS 50
#define HTTPRESPONSE_CAPACITY 1024

typedef enum
{
    OK = 0,

    SOCKET_BIND_ERROR   = -701,
    SOCKET_LISTEN_ERROR = -702,

    INVALID_REQUEST_LINE = -711,
    INVALID_METHOD       = -712,
    INVALID_PATH         = -713,
    INVALID_HEADERS      = -714

} ErrorCode;

typedef enum
{
    GET,
    POST,
    PUT,
    PATCH,
    DELETE,
    HEAD,
    OPTIONS,
    TRACE,
    CONNECT
} HTTPMethod;

#endif // COMMON_H