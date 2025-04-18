//
// test.h
// c-server
//
// Created by Samandar Komil on 16/04/2025.
// 
// Reference: Eric Meehan's C library - https://github.com/ericomeehan/libeom
//

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "Server.h"


#define MAX_BUFFER_SIZE 30000


void launch(Server *server){
    while (1){
        char buffer[MAX_BUFFER_SIZE];
        printf("===== Waiting for a connection... =====");
        int address_length = sizeof(server->address);
        int new_socket = accept(server->socket, (struct sockaddr *)&server->address, (socklen_t *)&address_length);
        read(new_socket, buffer, MAX_BUFFER_SIZE);
        printf("%s\n", buffer);

        char *hello = "HTTP/1.1 200 OK\nDate: Wed, 17 Apr 2024 10:30:45 GMT\nServer: Apache/2.4.41 (Ubuntu)\nContent-Type: application/json\nContent-Length: 102\nConnection: keep-alive\nCache-Control: max-age=3600";
        write(new_socket, hello, strlen(hello));
        close(new_socket);
    }
}


int main(void){
    printf(">>>>>\n");
}