//
// Server.h
// c-server
//
// Created by Samandar Komil on 13/04/2025.
// 
// Reference: Eric Meehan's C library - https://github.com/ericomeehan/libeom
//


#ifndef Server_h
#define Server_h


#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>


typedef struct Server {
    int domain;
    int service;
    int protocol;
    int port;
    uint32_t interface;
    int queue;
    struct sockaddr_in address;
    int socket;

    void (*launch)(struct Server *self);
} Server;


Server server_constructor(int domain, int service, int protocol, uint32_t interface, int port, int queue);


#endif /* Server_h */