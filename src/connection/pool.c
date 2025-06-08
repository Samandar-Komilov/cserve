#include "pool.h"

ConnectionPool *connection_pool_create(size_t capacity, int epoll_fd)
{
    if (capacity == 0 || epoll_fd < 0) return NULL;

    ConnectionPool *pool = calloc(1, sizeof(ConnectionPool));
    if (!pool) return NULL;

    pool->connections = calloc(capacity, sizeof(Connection *));
    if (!pool->connections)
    {
        free(pool);
        return NULL;
    }

    pool->capacity     = capacity;
    pool->active_count = 0;
    pool->epoll_fd     = epoll_fd;

    return pool;
}

void connection_pool_destroy(ConnectionPool *pool)
{
    if (!pool) return;

    // Clean up all active connections
    for (size_t i = 0; i < pool->capacity; i++)
    {
        if (pool->connections[i])
        {
            connection_cleanup(pool->connections[i]);
        }
    }

    free(pool->connections);
    free(pool);
}

Connection *connection_pool_acquire(ConnectionPool *pool, int socket_fd)
{
    if (!pool || socket_fd < 0) return NULL;

    if (pool->active_count >= pool->capacity)
    {
        LOG("ERROR", "Connection pool full");
        return NULL;
    }

    // Find empty slot
    for (size_t i = 0; i < pool->capacity; i++)
    {
        if (pool->connections[i] == NULL)
        {
            Connection *conn = connection_init(socket_fd);
            if (!conn) return NULL;

            pool->connections[i] = conn;
            pool->active_count++;

            LOG("DEBUG", "Acquired connection slot %zu for socket %d", i, socket_fd);
            return conn;
        }
    }

    return NULL;
}

int connection_pool_release(ConnectionPool *pool, Connection *conn)
{
    if (!pool || !conn) return ERROR_INVALID_PARAM;

    // Find and remove connection
    for (size_t i = 0; i < pool->capacity; i++)
    {
        if (pool->connections[i] == conn)
        {
            // Remove from epoll
            epoll_ctl(pool->epoll_fd, EPOLL_CTL_DEL, conn->socket, NULL);

            connection_cleanup(conn);
            pool->connections[i] = NULL;
            pool->active_count--;

            LOG("DEBUG", "Released connection slot %zu", i);
            return OK;
        }
    }

    return ERROR_INVALID_PARAM;
}

int connection_pool_cleanup_timeouts(ConnectionPool *pool)
{
    if (!pool) return ERROR_INVALID_PARAM;

    time_t now  = time(NULL);
    int cleaned = 0;

    for (size_t i = 0; i < pool->capacity; i++)
    {
        Connection *conn = pool->connections[i];
        if (conn && connection_is_timeout(conn, now))
        {
            LOG("INFO", "Cleaning up timed out connection %d", conn->socket);
            connection_pool_release(pool, conn);
            cleaned++;
        }
    }

    return cleaned;
}