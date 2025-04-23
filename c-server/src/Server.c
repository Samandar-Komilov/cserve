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