/**
 * @file    Server.h
 * @author  Samandar Komil
 * @date    13 April 2025
 * @brief   Defines the Server struct and constructor for TCP server behavior.
 *
 * @details Inspired by Eric Meehan's C library:
 *          https://github.com/ericomeehan/libeom
 */

#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "common.h"

typedef enum
{
    TCP,
    UDP
} TransportType;

typedef struct Server
{
    int domain;
    int service;
    int protocol;
    int port;
    uint32_t interface;
    int queue;
    TransportType transport;
    struct sockaddr_in address;
    int socket;

    // void (*launch)(struct Server *self);
} SocketServer;

SocketServer *server_constructor(int domain, int service, int protocol, uint32_t interface,
                                 int port, int queue);
void server_destructor(SocketServer *server);
#endif /* SERVER_H */