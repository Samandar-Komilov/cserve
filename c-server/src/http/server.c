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
    if (bind(self->server->socket, (struct sockaddr *)&self->server->address,
             sizeof(self->server->address)) < 0)
    {
        return SOCKET_BIND_ERROR;
    }

    if ((listen(self->server->socket, self->server->queue)) < 0)
    {
        return SOCKET_LISTEN_ERROR;
    }

    printf("\033[32m===== Waiting for connections on port %d =====\033[0m\n", self->server->port);

    while (1)
    {
        char buffer[MAX_BUFFER_SIZE];
        int address_length = sizeof(self->server->address);
        int new_socket     = accept(self->server->socket, (struct sockaddr *)&self->server->address,
                                    (socklen_t *)&address_length);
        read(new_socket, buffer, MAX_BUFFER_SIZE);
        printf("%s\n", buffer);

        HTTPRequest *httprequest_ptr = parse_http_request(buffer);

        HTTPResponse *httpresponse_ptr = request_handler(httprequest_ptr);

        char *response = httpresponse_serialize(httpresponse_ptr, NULL);

        // caller of the request_handler is responsible for freeing the memory
        httpresponse_free(httpresponse_ptr);

        printf("===== Response:\n%s", response);

        write(new_socket, response, strlen(response));

        // Caller owned malloced buffer, so frees it now
        free(response);

        close(new_socket);
    }
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
    char *path  = strdup(request_ptr->path);
    char *token = strtok(path, "/");

    if (token)
    {
        if (strcmp(token, "static") == 0)
        {
            char filepath[PATH_MAX];
            if (snprintf(filepath, sizeof(filepath), "%s/%s", realpath(BASE_DIR, NULL),
                         request_ptr->path) < 0)
            {
                HTTPResponse *response =
                    response_builder(404, "Not Found", "<h1>snprintf() error</h1>");
                return response;
            };
            if (access(filepath, R_OK) != 0)
            {
                HTTPResponse *response =
                    response_builder(403, "Forbidden", "<h1>403 Forbidden</h1>");
                return response;
            }

            int fd = open(filepath, O_RDONLY);
            if (fd == -1)
            {
                HTTPResponse *response =
                    response_builder(404, "Not Found", "<h1>404 Not Found</h1>");
                return response;
            }

            char buffer[MAX_BUFFER_SIZE];
            read(fd, buffer, sizeof(buffer));

            HTTPResponse *response = response_builder(200, "OK", buffer);

            close(fd);
            return response;
        }
        else if (strcmp(token, "api") == 0)
        {
            // TODO: Reverse proxy
            HTTPResponse *response = response_builder(200, "OK", "<h1>200 OK</h1>");
            return response;
        }
        else
        {
            HTTPResponse *response = response_builder(404, "Not Found", "<h1>404 Not Found</h1>");
            return response;
        }
    }

    HTTPResponse *response = response_builder(404, "Not Found", "<h1>404 Not Found</h1>");
    return response;
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
    free(httpserver_ptr);
}