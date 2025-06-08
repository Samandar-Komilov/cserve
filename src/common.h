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
#include <stdbool.h>
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

#include "utils/logger.h"

#define SOCKET_BUFFER_SIZE 2 * 1024 * 1024         // 2MB
#define INITIAL_CONNECTION_BUFFER_SIZE 8192        // 8KB
#define MAX_CONNECTION_BUFFER_SIZE 2 * 1024 * 1024 // 2MB
#define CONNECTION_TIMEOUT 300                     // 5 minutes
#define INITIAL_RESPONSE_SIZE 4096
#define MAX_CONNECTIONS 1000
#define MAX_EPOLL_EVENTS 1024
#define MAX_HEADERS 50
#define MAX_BACKENDS 16

#define BASE_DIR "./"

typedef enum
{
    OK                  = 0,
    ERROR_MEMORY        = -1,
    ERROR_INVALID_PARAM = -2,

    // Socket errors
    SOCKET_BIND_ERROR   = -701,
    SOCKET_LISTEN_ERROR = -702,
    SOCKET_ACCEPT_ERROR = -703,

    // HTTP parsing errors
    INVALID_REQUEST_LINE = -711,
    INVALID_METHOD       = -712,
    INVALID_PATH         = -713,
    INVALID_HEADERS      = -714,
    INVALID_REQUEST_BODY = -715,

    // Connection errors
    CONNECTION_POOL_FULL     = -721,
    CONNECTION_TIMEOUT_ERROR = -722,
} ErrorCode;

typedef enum
{
    CONN_IDLE,
    CONN_ACCEPTING,
    CONN_READING_HEADERS,
    CONN_READING_BODY,
    CONN_PROCESSING_REQUEST,
    CONN_SENDING_RESPONSE,
    CONN_KEEP_ALIVE,
    CONN_CLOSING,
    CONN_ERROR,
    CONN_CLOSED
} ConnectionState;

typedef enum
{
    REQ_PARSE_REQUEST_LINE,
    REQ_PARSE_HEADERS,
    REQ_PARSE_BODY,
    REQ_PARSE_COMPLETE,
    REQ_PARSE_ERROR
} HTTPRequestState;

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
    CONNECT,
    UNKNOWN
} HTTPMethod;

#endif // COMMON_H