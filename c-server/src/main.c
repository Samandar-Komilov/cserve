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


int main(void){
    Server TCPServer = server_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, 8080, 10);
    TCPServer.launch(&TCPServer);

    return 0;
}