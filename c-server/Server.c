//
// =========
// 1lang1server/c-server
//
// A repository to practice pretty much every language I learn by building a web server.
// =========
// 
//
// Created by Samandar Komil on 13/04/2025.
// 
// Reference: Eric Meehan's C library - https://github.com/ericomeehan/libeom
//
// 
// Server.c


#include "Server.h"

#include <stdio.h>
#include <stdlib.h>


Server server_constructor(int domain, int service, int protocol, uint32_t interface, int port, int queue) {
    Server server;
    server.domain = domain;
    server.service = service;
    server.protocol = protocol;
    server.port = port;
    server.interface = interface;
    server.queue = queue;

    server.address.sin_family = domain;
    server.address.sin_port = htons(port);
    server.address.sin_addr.s_addr = htonl(interface);

    server.socket = socket(domain, service, protocol);

    if (server.socket == 0){
        perror("Failed to connect socket...");
        exit(1);
    }

    if (bind(server.socket, (struct sockaddr *)&server.address, sizeof(server.address)) < 0) {
        perror("Failed to bind socket...");
        exit(1);
    }

    if ((listen(server.socket, server.queue)) < 0){
        perror("Failed to listen socket...");
        exit(1);
    }

    return server;
}