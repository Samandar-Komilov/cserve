#ifndef CONNECTION_H
#define CONNECTION_H

#include "common.h"
#include "http/handlers.h"

typedef struct Connection
{
    int socket;                   // client socket
    char *buffer;                 // dynamic buffer for request
    size_t buffer_size;           // allocated size for buffer
    size_t buffer_used;           // current data length of buffer
    ConnectionState state;        // connection state
    time_t last_active;           // last active time
    HTTPRequest *current_request; // current request
    int requests_handled;         // number of requests handled so far
    bool keep_alive;              // keep-alive?

    // State machine context
    void *state_data;
} Connection;

// Connection lifecycle
Connection *connection_init(int socket_fd);
void connection_cleanup(Connection *conn);
int connection_reset(Connection *conn);

// State management
int connection_set_state(Connection *conn, ConnectionState new_state);
bool connection_is_timeout(Connection *conn, time_t current_time);

// Buffer management
int connection_read_to_buffer(Connection *conn);
int connection_ensure_buffer_space(Connection *conn, size_t needed);
int connection_consume_buffer(Connection *conn, size_t bytes);

// Handles HTTP request/response for a connection. Returns 0 on success, <0 on error, sets
// connection state as needed.
int connection_handle_http(Connection *conn);

#endif