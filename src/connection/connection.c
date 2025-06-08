#include "connection/connection.h"
#include "http/request.h"
#include "http/response.h"

Connection *connection_init(int socket_fd)
{
    if (socket_fd < 0) return NULL;

    Connection *conn = calloc(1, sizeof(Connection));
    if (!conn) return NULL;

    conn->buffer = malloc(INITIAL_CONNECTION_BUFFER_SIZE);
    if (!conn->buffer)
    {
        free(conn);
        return NULL;
    }

    conn->socket           = socket_fd;
    conn->buffer_size      = INITIAL_CONNECTION_BUFFER_SIZE;
    conn->buffer_used      = 0;
    conn->state            = CONN_ACCEPTING;
    conn->last_active      = time(NULL);
    conn->current_request  = NULL;
    conn->requests_handled = 0;
    conn->keep_alive       = false;
    conn->state_data       = NULL;

    return conn;
}

void connection_cleanup(Connection *conn)
{
    if (!conn) return;

    if (conn->socket > 0)
    {
        close(conn->socket);
    }

    if (conn->buffer)
    {
        free(conn->buffer);
    }

    if (conn->current_request)
    {
        http_request_destroy(conn->current_request);
    }

    free(conn->state_data);
    free(conn);
}

int connection_reset(Connection *conn)
{
    if (!conn) return ERROR_INVALID_PARAM;

    // Reset buffer
    memset(conn->buffer, 0, conn->buffer_size);
    conn->buffer_used = 0;

    // Free current request
    if (conn->current_request)
    {
        http_request_destroy(conn->current_request);
        conn->current_request = NULL;
    }

    // Reset state
    conn->state       = CONN_KEEP_ALIVE;
    conn->last_active = time(NULL);

    return OK;
}

int connection_set_state(Connection *conn, ConnectionState new_state)
{
    if (!conn) return ERROR_INVALID_PARAM;

    ConnectionState old_state = conn->state;
    conn->state               = new_state;
    conn->last_active         = time(NULL);

    LOG("DEBUG", "Connection state %d: %d -> %d", conn->socket, old_state, new_state);
    return OK;
}

bool connection_is_timeout(Connection *conn, time_t current_time)
{
    if (!conn) return true;
    return (current_time - conn->last_active) > CONNECTION_TIMEOUT;
}

int connection_read_to_buffer(Connection *conn)
{
    if (!conn) return -1;
    int client_fd   = conn->socket;
    int total_bytes = 0;

    while (1)
    {
        // Ensure buffer space
        if (conn->buffer_used >= conn->buffer_size)
        {
            if (connection_ensure_buffer_space(conn, 1) != OK)
            {
                conn->state = CONN_ERROR;
                return -1;
            }
        }
        ssize_t bytes_read = recv(client_fd, conn->buffer + conn->buffer_used,
                                  conn->buffer_size - conn->buffer_used, 0);
        if (bytes_read < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break; // No more data for now
            }
            else
            {
                conn->state = CONN_ERROR;
                return -1;
            }
        }
        else if (bytes_read == 0)
        {
            // EOF
            return 0;
        }
        else
        {
            conn->buffer_used += bytes_read;
            total_bytes += bytes_read;
        }
    }
    return total_bytes;
}

int connection_ensure_buffer_space(Connection *conn, size_t needed)
{
    if (!conn) return ERROR_INVALID_PARAM;

    size_t available = conn->buffer_size - conn->buffer_used;
    if (available >= needed) return OK;

    // Check if we're exceeding maximum request size
    size_t new_size = conn->buffer_size;
    while (new_size - conn->buffer_used < needed)
    {
        new_size *= 2;
        if (new_size > MAX_CONNECTION_BUFFER_SIZE)
        {
            return ERROR_MEMORY;
        }
    }

    char *new_buffer = realloc(conn->buffer, new_size);
    if (!new_buffer) return ERROR_MEMORY;

    conn->buffer      = new_buffer;
    conn->buffer_size = new_size;

    LOG("DEBUG", "Buffer resized to %zu for connection %d", new_size, conn->socket);
    return OK;
}

int connection_consume_buffer(Connection *conn, size_t bytes)
{
    if (!conn || bytes > conn->buffer_used) return ERROR_INVALID_PARAM;

    if (bytes == conn->buffer_used)
    {
        // Consumed all data
        conn->buffer_used = 0;
        memset(conn->buffer, 0, conn->buffer_size);
    }
    else
    {
        // Move remaining data to front
        memmove(conn->buffer, conn->buffer + bytes, conn->buffer_used - bytes);
        conn->buffer_used -= bytes;
        memset(conn->buffer + conn->buffer_used, 0, conn->buffer_size - conn->buffer_used);
    }

    return OK;
}

int connection_handle_http(Connection *conn)
{
    int client_fd = conn->socket;
    if (conn->state == CONN_CLOSING || conn->state == CONN_ERROR) return -1;

    // Initialize HTTPRequest for current request
    if (conn->current_request == NULL)
    {
        conn->current_request = http_request_create();
        if (!conn->current_request)
        {
            LOG("ERROR", "Failed to allocate HTTPRequest");
            connection_set_state(conn, CONN_ERROR);
            return -1;
        }
    }
    connection_set_state(conn, CONN_PROCESSING_REQUEST);

    // Parse request if data available
    if (conn->current_request->state != REQ_PARSE_COMPLETE)
    {
        int consumed = parse_http_request(conn->buffer, conn->buffer_used, conn->current_request);
        if (consumed < 0)
        {
            LOG("ERROR", "Failed to parse HTTP request.");
            conn->current_request->state = REQ_PARSE_ERROR;
            connection_set_state(conn, CONN_ERROR);
            return -1;
        }
        else
        {
            LOG("DEBUG", "Successfully parsed HTTP request.");
            conn->current_request->state = REQ_PARSE_COMPLETE;
        }
    }
    // Handle request if fully parsed
    if (conn->current_request->state == REQ_PARSE_COMPLETE)
    {
        LOG("DEBUG", "Fully parsed HTTP request");
        HTTPResponse *response = http_handle_request(conn->current_request);
        if (!response)
        {
            LOG("ERROR", "Failed to handle HTTP request (no response generated).");
            conn->current_request->state = REQ_PARSE_ERROR;
            connection_set_state(conn, CONN_ERROR);
            return -1;
        }
        size_t response_len;
        char *response_str = httpresponse_serialize(response, &response_len);
        httpresponse_free(response);
        if (!response_str)
        {
            LOG("ERROR", "Failed to serialize HTTP response.");
            conn->current_request->state = REQ_PARSE_ERROR;
            connection_set_state(conn, CONN_ERROR);
            return -1;
        }
        // Send response
        conn->state       = CONN_SENDING_RESPONSE;
        size_t total_sent = 0;
        while (total_sent < response_len)
        {
            int bytes_sent =
                send(client_fd, response_str + total_sent, response_len - total_sent, 0);
            if (bytes_sent <= 0)
            {
                LOG("ERROR", "Error while sending response to client socket.");
                free(response_str);
                connection_set_state(conn, CONN_ERROR);
                return -1;
            }
            total_sent += bytes_sent;
        }
        LOG("DEBUG", "Sent %ld bytes response to client FD %d.", total_sent, client_fd);
        free(response_str);
    }
    // Check for keep-alive
    int keep_alive = 0;
    for (int j = 0; j < conn->current_request->header_count; j++)
    {
        if (strncmp(conn->current_request->headers[j].name, "Connection",
                    conn->current_request->headers[j].name_len) == 0 &&
            strncmp(conn->current_request->headers[j].value, "keep-alive",
                    conn->current_request->headers[j].value_len) == 0)
        {
            keep_alive = 1;
            break;
        }
    }
    if (keep_alive)
    {
        connection_reset(conn);
        LOG("DEBUG", "Connection is keep-alive for client FD %d", client_fd);
    }
    else
    {
        LOG("DEBUG", "Connection is not keep-alive for client FD %d, closing connection...",
            client_fd);
        connection_set_state(conn, CONN_CLOSING);
    }
    return 0;
}