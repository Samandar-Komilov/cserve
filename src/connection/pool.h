#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include "connection.h"

typedef struct ConnectionPool
{
    Connection **connections;
    size_t capacity;
    size_t active_count;
    int epoll_fd;
} ConnectionPool;

ConnectionPool *connection_pool_create(size_t capacity, int epoll_fd);
void connection_pool_destroy(ConnectionPool *pool);

Connection *connection_pool_acquire(ConnectionPool *pool, int socket_fd);
int connection_pool_release(ConnectionPool *pool, Connection *conn);
int connection_pool_cleanup_timeouts(ConnectionPool *pool);

#endif