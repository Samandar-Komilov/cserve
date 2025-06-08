/**
 * @file    HTTPServer.c
 * @author  Samandar Komil
 * @date    17 May 2025
 * @brief   HTTPServer methods and utility function implementations
 *
 */

#include "server.h"

int launch(HTTPServer *self)
{
    LOG("INFO", "Starting server launch sequence");

    if (httpserver_setup(self) != OK)
    {
        LOG("ERROR", "Failed to setup HTTPServer: %s", strerror(errno));
        return -1;
    }

    if (server_socket_epoll_add(self) != OK)
    {
        LOG("ERROR", "Failed to add server socket to epoll event loop: %s", strerror(errno));
        return -1;
    }

    LOG("INFO", "Server is ready and waiting for connections on port %d",
        ntohs(self->server->address.sin_port));

    while (1)
    {
        int n_ready = epoll_wait(self->epoll_fd, self->events, MAX_EPOLL_EVENTS, 60);
        if (n_ready == -1)
        {
            LOG("ERROR", "Failed to wait for epoll events: %s", strerror(errno));
            continue;
        }

        for (int i = 0; i < n_ready; i++)
        {
            if (self->events[i].data.fd == self->server->socket)
            {
                LOG("DEBUG", "New connection event on server socket");
                // Accept a new connection
                int client_fd = handle_new_connection(self);
                if (client_fd < 0)
                {
                    LOG("ERROR", "Failed to handle new connection");
                    continue;
                }

                Connection *conn = connection_pool_acquire(self->conn_pool, client_fd);
                if (!conn)
                {
                    LOG("ERROR", "No free connection slots available: %s", strerror(errno));
                    close(client_fd);
                    continue;
                }
                self->events[i].data.ptr = conn;

                LOG("INFO", "Connected to client on port %d, FD: %d",
                    ntohs(self->server->address.sin_port), client_fd);
            }
            else
            {
                // Handle client data
                LOG("DEBUG", "=======");
                Connection *conn = (Connection *)self->events[i].data.ptr;
                int client_fd    = conn->socket;

                LOG("DEBUG", "Handling data for client FD: %d", client_fd);
                int bytes_read = connection_read_to_buffer(conn);
                if (bytes_read < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        LOG("DEBUG", "EAGAIN || EWOULDBLOCK - No more data for now.");
                        continue;
                    }
                    else
                    {
                        LOG("ERROR", "Failed to read data from client: %s", strerror(errno));
                        connection_set_state(conn, CONN_ERROR);
                    }
                }
                else if (bytes_read == 0)
                {
                    LOG("INFO", "Client FD %d closed connection (EOF)", client_fd);
                    connection_set_state(conn, CONN_CLOSING);
                }
                else
                {
                    LOG("DEBUG", "Read %d bytes from socket FD %d", bytes_read, client_fd);
                }

                if (conn->state != CONN_CLOSING && conn->state != CONN_ERROR)
                {
                    connection_handle_http(conn);
                }
                if (conn->state == CONN_CLOSING || conn->state == CONN_ERROR)
                {
                    LOG("DEBUG", "Connection is closing for client FD %d", client_fd);
                    if (conn->current_request != NULL) http_request_destroy(conn->current_request);
                    connection_pool_release(self->conn_pool, conn);
                    epoll_ctl(self->epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                }
            }
        }
    }

    close(self->epoll_fd);
    return 0;
}

static int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        LOG("ERROR", "Failed to get socket flags: %s", strerror(errno));
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        LOG("ERROR", "Failed to set non-blocking mode: %s", strerror(errno));
        return -1;
    }

    return 0;
}

static int optimize_server_socket(int fd)
{
    int on = 1;

    // Enable immediately reusing the same port once server should be restarted without waiting for
    // timeout.
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
    {
        LOG("ERROR", "Failed to set SO_REUSEADDR: %s", strerror(errno));
        return -1;
    }

    // This option allows multiple processes to bind to the same port, as long as the bind call is
    // made with the SO_REUSEPORT option.
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) == -1)
    {
        LOG("WARNING", "Failed to set SO_REUSEPORT (continuing anyway): %s", strerror(errno));
    }

    // This option sets the receive buffer size for the socket. A larger buffer size can improve
    // performance by allowing the socket to handle more incoming data at once.
    int bufsize = SOCKET_BUFFER_SIZE;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == -1)
    {
        LOG("WARNING", "Failed to set SO_RCVBUF (continuing anyway): %s", strerror(errno));
    }

    // This option sets the send buffer size for the socket. Like SO_RCVBUF, a larger buffer size
    // can improve performance by allowing the socket to handle more outgoing data at once.
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == -1)
    {
        LOG("WARNING", "Failed to set SO_SNDBUF (continuing anyway): %s", strerror(errno));
    }

    return 0;
}

int server_socket_epoll_add(HTTPServer *self)
{
    struct epoll_event ev;
    ev.events  = EPOLLIN;
    ev.data.fd = self->server->socket;
    if (epoll_ctl(self->epoll_fd, EPOLL_CTL_ADD, self->server->socket, &ev) == -1)
    {
        close(self->epoll_fd);
        close(self->server->socket);
        LOG("ERROR", "Failed to add server socket to epoll event loop: %s", strerror(errno));
        return -1;
    }

    return OK;
}

int handle_new_connection(HTTPServer *server_ptr)
{
    // Accept a new connection
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd;

    LOG("DEBUG", "Waiting for new connection...");
    client_fd = accept(server_ptr->server->socket, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            LOG("ERROR", "Failed to accept new connection: %s", strerror(errno));
        }
        return -1;
    }

    LOG("DEBUG", "New connection accepted, FD: %d", client_fd);
    if (set_nonblocking(client_fd) == -1)
    {
        LOG("ERROR", "Failed to set non-blocking mode: %s", strerror(errno));
        close(client_fd);
        return -1;
    }

    struct epoll_event ev;
    ev.events  = EPOLLIN; // Level-triggered mode
    ev.data.fd = client_fd;
    if (epoll_ctl(server_ptr->epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
    {
        LOG("ERROR", "Failed to add client to epoll: %s", strerror(errno));
        close(client_fd);
        return -1;
    }
    LOG("DEBUG", "Client FD %d added to epoll", client_fd);
    return client_fd;
}

HTTPServer *httpserver_init(int port)
{
    LOG("INFO", "Initializing HTTPServer on port %d", port);
    HTTPServer *httpserver_ptr = (HTTPServer *)malloc(sizeof(HTTPServer));

    memset(httpserver_ptr, 0, sizeof(HTTPServer));
    SocketServer *SockServer = server_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, port, 10);
    if (!SockServer)
    {
        LOG("ERROR", "Failed to create SocketServer");
        free(httpserver_ptr);
        return NULL;
    }
    httpserver_ptr->server = SockServer;
    httpserver_ptr->launch = launch;
    LOG("INFO", "HTTPServer initialized successfully");

    return httpserver_ptr;
}

int httpserver_setup(HTTPServer *httpserver_ptr)
{
    if (httpserver_ptr->server == NULL || httpserver_ptr->server->socket < 0)
    {
        LOG("ERROR", "HTTPServer is not initialized: %s", strerror(errno));
        return -1;
    }

    if (optimize_server_socket(httpserver_ptr->server->socket) == -1)
    {
        LOG("ERROR", "Failed to optimize server socket: %s", strerror(errno));
        close(httpserver_ptr->server->socket);
        return -1;
    }

    LOG("INFO", "Attempting to bind to port %d", ntohs(httpserver_ptr->server->address.sin_port));
    if (bind(httpserver_ptr->server->socket, (struct sockaddr *)&httpserver_ptr->server->address,
             sizeof(httpserver_ptr->server->address)) < 0)
    {
        LOG("ERROR", "Failed to bind server socket: %s", strerror(errno));
        close(httpserver_ptr->server->socket);
        return SOCKET_BIND_ERROR;
    }
    LOG("INFO", "Successfully bound to port %d", ntohs(httpserver_ptr->server->address.sin_port));

    httpserver_ptr->is_running = 1;

    LOG("INFO", "Attempting to listen on socket");
    if ((listen(httpserver_ptr->server->socket, httpserver_ptr->server->queue)) < 0)
    {
        LOG("ERROR", "Failed to listen on socket: %s", strerror(errno));
        return SOCKET_LISTEN_ERROR;
    }
    LOG("INFO", "Successfully listening on socket");

    // Initialize epoll
    httpserver_ptr->epoll_fd = epoll_create1(0);
    if (httpserver_ptr->epoll_fd == -1)
    {
        LOG("ERROR", "Failed to initialize epoll instance: %s", strerror(errno));
        exit(1);
    }
    LOG("INFO", "Successfully initialized epoll instance");

    // Initialize connections
    httpserver_ptr->conn_pool = connection_pool_create(MAX_CONNECTIONS, httpserver_ptr->epoll_fd);
    if (!httpserver_ptr->conn_pool)
    {
        LOG("ERROR", "Failed to create connection pool: %s", strerror(errno));
        return -1;
    }
    LOG("INFO", "Successfully created connection pool");

    return OK;
}

void httpserver_cleanup(HTTPServer *httpserver_ptr)
{
    if (httpserver_ptr->server != NULL)
    {
        server_destructor(httpserver_ptr->server);
    }
    free(httpserver_ptr);
}