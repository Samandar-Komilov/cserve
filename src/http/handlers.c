#include "http/handlers.h"

static HTTPResponse *handle_static_file(const char *uri, size_t uri_len)
{
    char filepath[PATH_MAX];
    if (snprintf(filepath, sizeof(filepath), "%s%.*s", realpath(BASE_DIR, NULL), (int)uri_len,
                 uri) < 0)
    {
        LOG("ERROR", "Failed to build filepath.");
        return response_builder(404, "Not Found", "<h1>404 Not Found</h1>", 22, "text/html");
    }

    int fd = open(filepath, O_RDONLY);
    if (fd == -1)
    {
        LOG("ERROR", "Failed to open file.");
        return response_builder(404, "Not Found", "<h1>404 Not Found</h1>", 22, "text/html");
    }

    struct stat st;
    fstat(fd, &st);
    size_t filesize = st.st_size;
    char *buffer    = malloc(filesize);
    if (!buffer)
    {
        close(fd);
        return response_builder(500, "Internal Server Error", "<h1>Internal Server Error</h1>", 32,
                                "text/html");
    }

    size_t total_read = 0;
    while (total_read < filesize)
    {
        ssize_t bytes = read(fd, buffer + total_read, filesize - total_read);
        if (bytes <= 0)
        {
            free(buffer);
            close(fd);
            return response_builder(500, "Internal Server Error", "<h1>Internal Server Error</h1>",
                                    32, "text/html");
        }
        total_read += bytes;
    }
    close(fd);

    HTTPResponse *response = response_builder(200, "OK", buffer, filesize, get_mime_type(filepath));
    free(buffer);
    return response;
}

static HTTPResponse *handle_api_request(HTTPRequest *request_ptr)
{
    char *api_path = request_ptr->request_line.uri + strlen("/api");
    if (*api_path == '\0') api_path = "/";

    char proxy_request[INITIAL_CONNECTION_BUFFER_SIZE];
    int proxy_request_len = snprintf(
        proxy_request, sizeof(proxy_request),
        "%s %s HTTP/1.1\r\nHost: localhost\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",
        request_ptr->request_line.method, api_path, request_ptr->body_len);
    if (proxy_request_len < 0)
        return response_builder(500, "Internal Server Error", "<h1>Internal Server Error</h1>", 32,
                                "text/html");

    strncat(proxy_request, request_ptr->body, request_ptr->body_len);

    int backend_fd = connect_to_backend("localhost", "8002");
    if (backend_fd == -1)
        return response_builder(502, "Bad Gateway", "<h1>502 Bad Gateway</h1>", 25, "text/html");

    send(backend_fd, proxy_request, proxy_request_len, 0);

    char proxy_response[INITIAL_CONNECTION_BUFFER_SIZE];
    int proxy_response_len = recv(backend_fd, proxy_response, sizeof(proxy_response), 0);
    if (proxy_response_len < 0)
    {
        close(backend_fd);
        return response_builder(502, "Bad Gateway", "<h1>502 Bad Gateway</h1>", 25, "text/html");
    }

    proxy_response[proxy_response_len] = '\0';
    close(backend_fd);

    char *body = strstr(proxy_response, "\r\n\r\n");
    if (!body)
        body = proxy_response;
    else
        body += 4;

    return response_builder(200, "OK", body, strlen(body), "text/html");
}

HTTPResponse *http_handle_request(HTTPRequest *request_ptr)
{
    if (request_ptr == NULL || request_ptr->request_line.uri == NULL)
        return response_builder(400, "Bad Request", "<h1>400 Bad Request</h1>", 27, "text/html");

    if (request_ptr->request_line.uri_len > 0)
    {
        if (strncmp(request_ptr->request_line.uri, "/static", 7) == 0)
        {
            return handle_static_file(request_ptr->request_line.uri,
                                      request_ptr->request_line.uri_len);
        }
        else if (strncmp(request_ptr->request_line.uri, "/api", 4) == 0)
        {
            return handle_api_request(request_ptr);
        }
    }

    return response_builder(404, "Not Found", "<h1>404 Not Found</h1>", 22, "text/html");
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
        LOG("ERROR", "Failed to create socket while connecting to proxy backend.");
        freeaddrinfo(res);
        return -1;
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0)
    {
        LOG("ERROR", "Failed to connect to proxy backend.");
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sock;
}