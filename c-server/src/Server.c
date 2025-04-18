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
#include <unistd.h>
#include <string.h>


#define MAX_BUFFER_SIZE 30000


void launch(Server *self){
    while (1){
        char buffer[MAX_BUFFER_SIZE];
        printf("===== Waiting for a connection... =====");
        int address_length = sizeof(self->address);
        int new_socket = accept(self->socket, (struct sockaddr *)&self->address, (socklen_t *)&address_length);
        read(new_socket, buffer, MAX_BUFFER_SIZE);
        printf("%s\n", buffer);

        char *hello = "HTTP/1.1 200 OK\nDate: Wed, 17 Apr 2024 10:30:45 GMT\nServer: Apache/2.4.41 (Ubuntu)\nContent-Type: application/json\nContent-Length: 102\nConnection: keep-alive\nCache-Control: max-age=3600";
        write(new_socket, hello, strlen(hello));
        close(new_socket);
    }
}



Server server_constructor(int domain, int service, int protocol, uint32_t interface, int port, int queue) {
    printf(">>>");
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

    printf(">>> %d %d", server.socket, server.port);

    if (server.socket == 0){
        perror("Failed to connect socket...");
        exit(1);
    }

    printf(">>>");

    if (bind(server.socket, (struct sockaddr *)&server.address, sizeof(server.address)) < 0) {
        perror("Failed to bind socket...");
        exit(1);
    }

    printf(">>>");

    if ((listen(server.socket, server.queue)) < 0){
        perror("Failed to listen socket...");
        exit(1);
    }

    printf(">>>");

    server.launch = launch;

    printf(">>>");

    return server;
}