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

#define MAX_BUFFER_SIZE 8192 // 8 KB
#define MAX_HEADERS 50
#define MAX_BACKENDS 16
#define MAX_EPOLL_EVENTS 10
#define HTTPRESPONSE_CAPACITY 1024

#define DEFAULT_CONFIG_PATH "/home/voidp/Projects/samandar/1lang1server/cserver"
#define BASE_DIR "./"

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