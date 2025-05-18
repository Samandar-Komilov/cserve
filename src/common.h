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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

#include <linux/limits.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>

#define str(x) #x
#define xstr(x) str(x)

#define INITIAL_BUFFER_SIZE 30000
#define MAX_EPOLL_EVENTS 1024
#define MAX_CONNECTIONS 1000
#define MAX_HEADERS 50
#define MAX_BACKENDS 16
#define INITIAL_RESPONSE_SIZE 10000

#define DEFAULT_CONFIG_PATH "/home/voidp/Projects/samandar/1lang1server/cserver"
#define BASE_DIR "./"

typedef enum
{
    PARSE_REQUEST_LINE,
    PARSE_HEADERS,
    PARSE_BODY,
    PARSE_DONE,
    PARSE_ERROR
} ParseState;

typedef enum
{
    OK = 0,

    SOCKET_BIND_ERROR   = -701,
    SOCKET_LISTEN_ERROR = -702,

    INVALID_REQUEST_LINE = -711,
    INVALID_METHOD       = -712,
    INVALID_PATH         = -713,
    INVALID_HEADERS      = -714,
    INVALID_REQUEST_BODY = -715,

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