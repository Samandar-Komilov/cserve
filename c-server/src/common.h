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

#define MAX_BUFFER_SIZE 30000

#define MAX_HEADERS 50
#define HTTPRESPONSE_CAPACITY 1024

typedef enum
{
    VECTOR_SUCCESS       = 700,
    VECTOR_ALLOC_ERROR   = -701,
    VECTOR_INDEX_ERROR   = -702,
    VECTOR_NULLPTR_ERROR = -703,

    SOCKET_BIND_ERROR   = -711,
    SOCKET_LISTEN_ERROR = -712

} ErrorCode;

#endif // COMMON_H