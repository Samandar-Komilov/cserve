/**
 * @file    Server.c
 * @author  Samandar Komil
 * @date    13 April 2025
 * @brief   Implements TCP server setup and launch routine.
 *
 * @details Inspired by Eric Meehan's C library:
 *          https://github.com/ericomeehan/libeom
 */

#include "Server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAX_BUFFER_SIZE 30000

void launch(Server *self)
{
    if (bind(self->socket, (struct sockaddr *)&self->address, sizeof(self->address)) < 0)
    {
        perror("Failed to bind socket...");
        exit(1);
    }

    if ((listen(self->socket, self->queue)) < 0)
    {
        perror("Failed to listen socket...");
        exit(1);
    }

    printf("\033[32m===== Waiting for connections on port %d =====\033[0m\n", self->port);

    while (1)
    {
        char buffer[MAX_BUFFER_SIZE];
        int address_length = sizeof(self->address);
        int new_socket =
            accept(self->socket, (struct sockaddr *)&self->address, (socklen_t *)&address_length);
        read(new_socket, buffer, MAX_BUFFER_SIZE);
        printf("%s\n", buffer);

        char *response = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/plain\r\n"
                         "Content-Length: 13\r\n"
                         "\r\n"
                         "Hello, World!";

        write(new_socket, response, strlen(response));
        close(new_socket);
    }
}

Server *server_constructor(int domain, int service, int protocol, uint32_t interface, int port,
                           int queue)
{
    Server *server_ptr    = (Server *)malloc(sizeof(Server));
    server_ptr->domain    = domain;
    server_ptr->service   = service;
    server_ptr->protocol  = protocol;
    server_ptr->port      = port;
    server_ptr->interface = interface;
    server_ptr->queue     = queue;

    server_ptr->address.sin_family      = domain;
    server_ptr->address.sin_port        = htons(port);
    server_ptr->address.sin_addr.s_addr = htonl(interface);

    server_ptr->socket = socket(domain, service, protocol);

    if (server_ptr->socket == 0)
    {
        perror("Failed to connect socket...");
        exit(1);
    }

    server_ptr->launch = launch;

    return server_ptr;
}

void server_destructor(Server *server)
{
    if (server)
    {
        close(server->socket);
        free(server);
    }
}