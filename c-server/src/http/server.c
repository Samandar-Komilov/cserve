/**
 * @file    HTTPServer.c
 * @author  Samandar Komil
 * @date    19 April 2025
 * @brief   HTTPServer methods and utility function implementations
 *
 */

#include "server.h"

/**
 * @brief   Launches the HTTP server, binding and listening on the port specified in
 *          the HTTPServer struct.
 *
 * @param   self  The HTTPServer struct containing the server config and methods.
 *
 * @returns An error code indicating the success or failure of the server launch.
 *
 * This function takes care of binding and listening on the configured port. It
 * then enters an infinite loop, accepting and processing incoming requests.
 *
 * The function will block until a request is received, process it with the
 * configured request_handler, and then send the response back to the client.
 *
 * If the bind or listen calls fail, an error code is returned. Otherwise, the
 * function will run indefinitely until the program is terminated.
 */
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

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        fprintf(stderr, "[ERROR] epoll_create1() call");
        exit(1);
    }

    struct epoll_event ev, events[MAX_EPOLL_EVENTS];
    ev.events  = EPOLLIN;
    ev.data.fd = self->server->socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, self->server->socket, &ev) == -1)
    {
        close(epoll_fd);
        close(self->server->socket);
        perror("[ERROR] Failed to add server socket to epoll event loop.");
        exit(1);
    }

    printf("\033[32m===== Waiting for connections on port %d =====\033[0m\n", self->server->port);

    while (1)
    {
        char buff[MAX_BUFFER_SIZE];
        int address_length = sizeof(self->server->address);

        int n_ready = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, 60);
        if (n_ready == -1)
        {
            perror("[ERROR] Failed to wait for epoll events.");
            continue;
        }

        for (int i = 0; i < n_ready; i++)
        {
            if (events[i].data.fd == self->server->socket)
            {
                // Accept a new connection
                int client_fd =
                    accept(self->server->socket, (struct sockaddr *)&self->server->address,
                           (socklen_t *)&address_length);
                if (client_fd == -1)
                {
                    perror("[ERROR] Error while accepting a new connection");
                    close(client_fd);
                    continue;
                }

                // Make socket nonblocking
                int flags = fcntl(client_fd, F_GETFL, 0);
                if (flags == -1) return -1;
                fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

                ev.events  = EPOLLIN;
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
                {
                    perror("[ERROR] Error while responding to the client socket: ");
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    close(events[i].data.fd);
                    continue;
                }

                printf("[INFO] Client FD: %d, Assigned FD: %d\n", client_fd, ev.data.fd);

                inet_ntop(self->server->address.sin_family,
                          (struct sockaddr *)&self->server->address, s, sizeof(s));
                printf("[INFO] Got connection from %s\n", s);
            }
            else
            {
                // Read from client socket and handle accordingly
                int client_fd  = events[i].data.fd;
                int total_read = 0;
                while (1)
                {
                    int bytes_read =
                        recv(client_fd, buff + total_read, MAX_BUFFER_SIZE - total_read, 0);
                    if (bytes_read < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            // All data read
                            break;
                        }
                        else
                        {
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                            close(client_fd);
                            perror("[ERROR] recv()");
                            break;
                        }
                    }
                    else if (bytes_read == 0)
                    {
                        // Client closed connection
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                        close(client_fd);
                        perror("[INFO] Client disconnected.\n");
                        break;
                    }
                    else
                    {
                        total_read += bytes_read;
                        if (total_read >= MAX_BUFFER_SIZE) break;
                    }
                }
                buff[total_read] = '\0';

                printf("[INFO] Received: %s\nBytes read: %d\n", buff, total_read);

                HTTPRequest *httprequest_ptr   = parse_http_request(buff);
                if (!httprequest_ptr) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                    perror("[ERROR] Error occured while parsing HTTP request.\n");
                    break;
                }

                HTTPResponse *httpresponse_ptr = request_handler(httprequest_ptr);
                if (!httpresponse_ptr) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                    perror("[ERROR] Error occured while handling HTTP request.\n");
                    break;
                }

                char *response   = httpresponse_serialize(httpresponse_ptr, NULL);
                if (!response){
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                    perror("[ERROR] Error occured while serializing HTTP response.\n");
                    break;
                }
                int response_len = sizeof(response);

                // caller of the request_handler is responsible for freeing the memory
                httpresponse_free(httpresponse_ptr);

                printf("===== Response:\n%s\n\n", response);

                int total_sent = 0;
                while (total_sent < response_len)
                {
                    int bytes_sent = send(events[i].data.fd, response + total_sent,
                                          response_len - total_sent, 0);
                    if (bytes_sent <= 0)
                    {
                        perror("[ERROR] Error while sending response to client socket: ");
                        break;
                    }
                    total_sent += bytes_sent;
                }

                printf("[INFO] Response sent: %d\n", total_sent);

                // Caller owned malloced buffer, so frees it now
                free(response);
            }
        }
    }

    close(epoll_fd);
    return 0;
}

/**
 * @brief   HTTP request handler.
 *
 * This function takes an HTTPRequest struct as argument and returns an
 * HTTPResponse struct. It handles the request by looking at the path and
 * deciding what to do with it:
 *
 * - If the path is /static, it serves the file at the given path.
 * - If the path is /api, it calls the reverse proxy handler.
 * - Otherwise, it returns a 404 Not Found response.
 *
 * @example
 * HTTPRequest request = { .path = "/static/index.html" };
 * HTTPResponse *response = request_handler(&request);
 * // response is an HTTPResponse struct with status code 200, body containing the file contents
 * // and other fields populated accordingly.
 *
 * @param   request_ptr  The HTTPRequest struct to handle.
 *
 * @returns A pointer to an HTTPResponse struct containing the response to the
 *          request.
 */
HTTPResponse *request_handler(HTTPRequest *request_ptr)
{
    // TODO: prevent segfault - check before strdup()
    if (request_ptr == NULL) return NULL;
    if (request_ptr->path == NULL) return NULL;
    char *path  = strdup(request_ptr->path);
    char *token = strtok(path, "/");

    if (token)
    {
        if (strcmp(token, "static") == 0)
        {
            char filepath[PATH_MAX];
            if (snprintf(filepath, sizeof(filepath), "%s%s", realpath(BASE_DIR, NULL),
                         request_ptr->path) < 0)
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
        else if (strcmp(token, "api") == 0)
        {
            // TODO: Reverse proxy
            char *api_path = request_ptr->path + strlen("/api");
            if (*api_path == '\0') api_path = "/";

            char proxy_request[MAX_BUFFER_SIZE];
            int proxy_request_len =
                snprintf(proxy_request, sizeof(proxy_request),
                         "%s %s HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Content-Length: %zu\r\n"
                         "Connection: close\r\n"
                         "\r\n",
                         request_ptr->method, api_path, request_ptr->body_length);

            if (proxy_request_len < 0)
            {
                char response_buffer[] = "<h1>snprintf() error</h1>";
                return response_builder(500, "Internal Server Error", response_buffer,
                                        sizeof(response_buffer), "text/html");
            }

            // Then, append the request body to the proxy_request string
            strncat(proxy_request, request_ptr->body, request_ptr->body_length);

            int backend_fd = connect_to_backend("localhost", "8000");
            if (backend_fd == -1)
            {
                char response_buffer[] = "<h1>Failed to connect to backend</h1>";
                return response_builder(502, "Bad Gateway", response_buffer,
                                        sizeof(response_buffer), "text/html");
            }

            send(backend_fd, proxy_request, proxy_request_len, 0);

            // Receive response from backend
            char proxy_response[MAX_BUFFER_SIZE];
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

/**
 * @brief   Connect to a backend server using the given host and port.
 *
 * @param   host  The hostname or IP address of the backend server.
 * @param   port  The port number of the backend server.
 *
 * @returns A file descriptor for the connected socket, or -1 on failure.
 *
 * This function takes care of resolving the hostname to an address and
 * connecting to the specified port.
 *
 * The caller is responsible for closing the socket when finished.
 */
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

/**
 * @brief   Constructor for the HTTPServer struct, setting up a server and HTTP
 *          request handler.
 *
 * @param   port  The port number to listen on.
 *
 * @returns A pointer to the newly created HTTPServer struct.
 *
 * The HTTPServer struct contains a SocketServer struct, which is used to
 * configure the server and handle incoming requests. The launch method is
 * also set to launch, which is responsible for binding and listening on the
 * configured port, and then entering an infinite loop to accept and process
 * incoming requests.
 */
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