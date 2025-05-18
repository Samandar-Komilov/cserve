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
    char s[INET6_ADDRSTRLEN];

    if (bind(self->server->socket, (struct sockaddr *)&self->server->address,
             sizeof(self->server->address)) < 0)
    {
        return SOCKET_BIND_ERROR;
    }
    if ((listen(self->server->socket, self->server->queue)) < 0)
    {
        return SOCKET_LISTEN_ERROR;
    }

    // Initialize epoll
    self->epoll_fd = epoll_create1(0);
    if (self->epoll_fd == -1)
    {
        fprintf(stderr, "[ERROR] epoll_create1() call");
        exit(1);
    }

    // Initialize connections
    self->connections = calloc(MAX_CONNECTIONS, sizeof(Connection));
    if (!self->connections)
    {
        fprintf(stderr, "[ERROR] Failed to allocate memory for connections");
        close(self->epoll_fd);
        return -1;
    }
    self->active_count = 0;

    // Add server socket to epoll
    struct epoll_event ev, events[MAX_EPOLL_EVENTS];
    ev.events  = EPOLLIN;
    ev.data.fd = self->server->socket;
    if (epoll_ctl(self->epoll_fd, EPOLL_CTL_ADD, self->server->socket, &ev) == -1)
    {
        close(self->epoll_fd);
        close(self->server->socket);
        fprintf(stderr, "[ERROR] Failed to add server socket to epoll event loop.");
        return -1;
    }

    printf("\033[32m===== Waiting for connections on port %d =====\033[0m\n", self->server->port);

    while (1)
    {
        int n_ready = epoll_wait(self->epoll_fd, events, MAX_EPOLL_EVENTS, 60);
        if (n_ready == -1)
        {
            fprintf(stderr, "[ERROR] Failed to wait for epoll events.");
            continue;
        }

        for (int i = 0; i < n_ready; i++)
        {
            if (events[i].data.fd == self->server->socket)
            {
                // Accept a new connection
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);

                int client_fd =
                    accept(self->server->socket, (struct sockaddr *)&client_addr, &client_len);
                if (client_fd == -1)
                {
                    fprintf(stderr, "[ERROR] Error while accepting a new connection");
                    close(client_fd);
                    continue;
                }

                // Find free connection slot from connections pool
                Connection *conn = NULL;
                for (size_t j = 0; j < MAX_CONNECTIONS; j++)
                {
                    if (self->connections[j].socket == 0)
                    {
                        conn = &self->connections[j];
                        break;
                    }
                }
                if (!conn || self->active_count >= MAX_CONNECTIONS)
                {
                    fprintf(stderr, "[ERROR] No free connection slots available");
                    close(client_fd);
                    continue;
                }

                // Initialize a connection
                conn->socket = client_fd;
                conn->buffer = (char *)malloc(INITIAL_BUFFER_SIZE);
                if (!conn->buffer)
                {
                    fprintf(stderr, "[ERROR] Failed to allocate memory for connection buffer");
                    close(client_fd);
                    continue;
                }
                conn->buffer_size  = INITIAL_BUFFER_SIZE;
                conn->len          = 0;
                conn->parsed_bytes = 0;
                conn->state        = PARSE_REQUEST_LINE;
                memset(&conn->request, 0, sizeof(HTTPRequest));
                self->active_count++;

                // Set socket nonblocking
                int flags = fcntl(client_fd, F_GETFL, 0);
                if (flags == -1 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK))
                {
                    fprintf(stderr, "[ERROR] Failed to set socket nonblocking");
                    free(conn->buffer);
                    close(client_fd);
                    conn->socket = 0;
                    self->active_count--;
                    continue;
                }

                // Add to epoll
                ev.events   = EPOLLIN;
                ev.data.ptr = conn;
                if (epoll_ctl(self->epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
                {
                    fprintf(stderr, "[ERROR] Error while responding to the client socket: ");
                    free(conn->buffer);
                    close(client_fd);
                    conn->socket = 0;
                    self->active_count--;
                    continue;
                }

                inet_ntop(AF_INET, &client_addr.sin_addr, s, sizeof(s));
                printf("[INFO] Connected: %s:%d, FD: %d\n", s, ntohs(client_addr.sin_port),
                       client_fd);
            }
            else
            {
                // Handle client data
                Connection *conn = (Connection *)events[i].data.ptr;
                int client_fd    = conn->socket;

                // Read data in loop (considering partial reads)
                while (1)
                {
                    // Check if buffer is full
                    // if (conn->len >= conn->buffer_size)
                    // {
                    //     size_t new_size  = conn->buffer_size * 2;
                    //     char *new_buffer = realloc(conn->buffer, new_size);
                    //     if (!new_buffer)
                    //     {
                    //         fprintf(stderr, "[ERROR] realloc buffer for FD %d\n", client_fd);
                    //         conn->state = PARSE_ERROR;
                    //         break;
                    //     }
                    //     conn->buffer      = new_buffer;
                    //     conn->buffer_size = new_size;
                    // }

                    int bytes_read =
                        recv(client_fd, conn->buffer + conn->len, conn->buffer_size - conn->len, 0);
                    if (bytes_read < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            // All data read
                            break;
                        }
                        else
                        {
                            perror("[ERROR] recv");
                            conn->state = PARSE_ERROR;
                            break;
                        }
                    }
                    else if (bytes_read == 0)
                    {
                        // Client closed connection
                        if (conn->len > 0)
                        {
                            // Data in buffer, try parsing
                            printf("[INFO] FD %d closed, %zu bytes in buffer\n", client_fd,
                                   conn->len);
                        }
                        else
                        {
                            // No data, treat as error
                            printf("[INFO] FD %d closed with no data\n", client_fd);
                            conn->state = PARSE_ERROR;
                        }
                        break;
                    }
                    else
                    {
                        conn->len += bytes_read;
                        printf("[INFO] FD %d read %d bytes, total %zu\n", client_fd, bytes_read,
                               conn->len);
                    }
                }
                conn->buffer[conn->len] = '\0';

                printf("[INFO] FD %d, Len: %d, buffer: %s\n", client_fd, (int)conn->len,
                       conn->buffer);

                // Parse request if data available
                if (conn->len > conn->parsed_bytes && conn->state != PARSE_DONE)
                {
                    HTTPRequest *req = create_http_request();
                    printf("Req: %p\n", req);
                    int consumed = parse_http_request(conn->buffer, conn->len, req);
                    if (consumed < 0)
                    {
                        conn->state = PARSE_ERROR;
                    }
                    else
                    {
                        conn->request      = *req;
                        conn->state        = PARSE_DONE;
                        conn->parsed_bytes = conn->len;
                    }
                }

                // Handle request if fully parsed
                if (conn->state == PARSE_DONE)
                {
                    printf("[INFO] Request: %.*s %.*s\n",
                           (int)conn->request.request_line.method_len,
                           conn->request.request_line.method,
                           (int)conn->request.request_line.uri_len, conn->request.request_line.uri);
                }

                HTTPResponse *response = request_handler(&conn->request);
                if (!response)
                {
                    conn->state = PARSE_ERROR;
                }
                else
                {
                    size_t response_len;
                    char *response_str = httpresponse_serialize(response, &response_len);
                    httpresponse_free(response);
                    if (!response_str)
                    {
                        conn->state = PARSE_ERROR;
                    }
                    else
                    {
                        // Send response
                        size_t total_sent = 0;
                        while (total_sent < response_len)
                        {
                            int bytes_sent = send(client_fd, response_str + total_sent,
                                                  response_len - total_sent, 0);
                            if (bytes_sent <= 0)
                            {
                                fprintf(stderr,
                                        "[ERROR] Error while sending response to client socket: ");
                                break;
                            }
                            total_sent += bytes_sent;
                        }

                        printf("Response sent: %ld bytes.\n", total_sent);
                    }
                    printf("===== Response:\n%s\n", response_str);
                    free(response_str);
                }

                if (conn->state == PARSE_ERROR)
                {
                    const char *error_response =
                        "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
                    send(client_fd, error_response, strlen(error_response), 0);

                    epoll_ctl(self->epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                    free(conn->buffer);
                    conn->buffer = NULL;
                    conn->socket = 0;
                    self->active_count--;

                    printf("[INFO] FD %d closed.\n", client_fd);
                }
            }
        }
    }

    close(self->epoll_fd);
    return 0;
}

HTTPResponse *request_handler(HTTPRequest *request_ptr)
{
    // TODO: prevent segfault - check before strdup()
    if (request_ptr == NULL) return NULL;
    if (request_ptr->request_line.uri == NULL) return NULL;

    if (strlen(request_ptr->request_line.uri) > 0)
    {
        if (strncmp(request_ptr->request_line.uri, "/static", 7) == 0)
        {
            char filepath[PATH_MAX];
            if (snprintf(filepath, sizeof(filepath), "%s%s", realpath(BASE_DIR, NULL),
                         request_ptr->request_line.uri) < 0)
            {
                char response_buffer[] = "<h1>snprintf() error</h1>";
                HTTPResponse *response = response_builder(404, "Not Found", response_buffer,
                                                          sizeof(response_buffer), "text/html");
                return response;
            };

            // printf("[LOG] %s %s %s\n", filepath, token, path);

            if (access(filepath, R_OK) != 0)
            {
                char response_buffer[] = "<h1>snprintf() error</h1>";
                HTTPResponse *response = response_builder(403, "Forbidden", response_buffer,
                                                          sizeof(response_buffer), "text/html");
                return response;
            }

            int fd = open(filepath, O_RDONLY);
            if (fd == -1)
            {
                char response_buffer[] = "<h1>snprintf() error</h1>";
                HTTPResponse *response = response_builder(404, "Not Found", response_buffer,
                                                          sizeof(response_buffer), "text/html");
                return response;
            }

            // Get file size
            struct stat st;
            fstat(fd, &st);
            size_t filesize = st.st_size;

            char *buffer = malloc(filesize);
            memset(buffer, 0, filesize);

            if (!buffer)
            {
                close(fd);
                char response_buffer[] = "<h1>Memory alloc failed</h1>";
                return response_builder(500, "Internal Server Error", response_buffer,
                                        sizeof(response_buffer), "text/html");
            }

            size_t total_read = 0;
            while (total_read < filesize)
            {
                size_t bytes = read(fd, buffer + total_read, filesize - total_read);
                if (bytes <= 0)
                {
                    free(buffer);
                    char response_buffer[] = "<h1>Failed to read file</h1>";
                    return response_builder(500, "Internal Server Error", response_buffer,
                                            sizeof(response_buffer), "text/html");
                }
                total_read += bytes;
            }

            printf("Read %zu bytes\nActual filesize: %zu\n", total_read, filesize);
            if (total_read != filesize)
            {
                free(buffer);
                char response_buffer[] = "<h1>Failed to read file</h1>";
                return response_builder(500, "Internal Server Error", response_buffer,
                                        sizeof(response_buffer), "text/html");
            }

            HTTPResponse *response =
                response_builder(200, "OK", buffer, filesize, get_mime_type(filepath));

            close(fd);
            return response;
        }
        else if (strncmp(request_ptr->request_line.uri, "/api", 4) == 0)
        {
            // TODO: Reverse proxy
            char *api_path = request_ptr->request_line.uri + strlen("/api");
            if (*api_path == '\0') api_path = "/";

            char proxy_request[INITIAL_BUFFER_SIZE];
            int proxy_request_len =
                snprintf(proxy_request, sizeof(proxy_request),
                         "%s %s HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Content-Length: %zu\r\n"
                         "Connection: close\r\n"
                         "\r\n",
                         request_ptr->request_line.method, api_path, request_ptr->body_len);

            if (proxy_request_len < 0)
            {
                char response_buffer[] = "<h1>snprintf() error</h1>";
                return response_builder(500, "Internal Server Error", response_buffer,
                                        sizeof(response_buffer), "text/html");
            }

            // Then, append the request body to the proxy_request string
            strncat(proxy_request, request_ptr->body, request_ptr->body_len);

            int backend_fd = connect_to_backend("localhost", "8000");
            if (backend_fd == -1)
            {
                char response_buffer[] = "<h1>Failed to connect to backend</h1>";
                return response_builder(502, "Bad Gateway", response_buffer,
                                        sizeof(response_buffer), "text/html");
            }

            send(backend_fd, proxy_request, proxy_request_len, 0);

            // Receive response from backend
            char proxy_response[INITIAL_BUFFER_SIZE];
            int proxy_response_len = recv(backend_fd, proxy_response, sizeof(proxy_response), 0);

            if (proxy_response_len < 0)
            {
                char response_buffer[] = "<h1>Failed to read from backend</h1>";
                return response_builder(502, "Bad Gateway", response_buffer,
                                        sizeof(response_buffer), "text/html");
            }

            proxy_response[proxy_response_len] = '\0';
            close(backend_fd);

            // TODO: parse response headers, here we directly return body
            char *body = strstr(proxy_response, "\r\n\r\n");
            if (!body)
                body = proxy_response;
            else
                body += 4;

            HTTPResponse *response =
                response_builder(200, "OK", body, proxy_request_len, "text/html");
            return response;
        }
        else
        {
            char response_buffer[] = "<h1>404 Not Found</h1>";
            HTTPResponse *response = response_builder(404, "Not Found", response_buffer,
                                                      sizeof(response_buffer), "text/html");
            return response;
        }
    }

    char response_buffer[] = "<h1>404 Not Found</h1>";
    HTTPResponse *response =
        response_builder(404, "Not Found", response_buffer, sizeof(response_buffer), "text/html");
    return response;
}

int connect_to_backend(const char *host, const char *port)
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) return -1;

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0)
    {
        freeaddrinfo(res);
        return -1;
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0)
    {
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sock;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

HTTPServer *httpserver_constructor(int port, char *static_dir, char **proxy_backends,
                                   int backend_count)
{
    HTTPServer *httpserver_ptr = (HTTPServer *)malloc(sizeof(HTTPServer));

    SocketServer *SockServer   = server_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, port, 10);
    httpserver_ptr->server     = SockServer;
    httpserver_ptr->static_dir = strdup(static_dir);
    httpserver_ptr->proxy_backends = proxy_backends;
    httpserver_ptr->backend_count  = backend_count;
    httpserver_ptr->launch         = launch;

    return httpserver_ptr;
}

void httpserver_destructor(HTTPServer *httpserver_ptr)
{
    if (httpserver_ptr->server != NULL)
    {
        server_destructor(httpserver_ptr->server);
    }
    free(httpserver_ptr->static_dir);
    free(httpserver_ptr);
}