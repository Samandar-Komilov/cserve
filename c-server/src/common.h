/**
 * @file    common.h
 * @author  Samandar Komil
 * @date    24 April 2025
 * @brief   Common definitions
 *
 */

#ifndef COMMON_H
#define COMMON_H

#define MAX_BUFFER_SIZE 30000

typedef enum
{
    DA_SUCCESS       = 700,
    DA_ERROR_ALLOC   = -701,
    DA_ERROR_INDEX   = -702,
    DA_ERROR_NULLPTR = -703
} ErrorCode;

#endif // COMMON_H