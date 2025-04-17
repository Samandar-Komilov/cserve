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


typedef struct Server Server;

typedef struct Server {
    int domain;
    int service;
    int protocol;
    int port;
    uint32_t interface;
    int queue;
    struct sockaddr_in address;
    int socket;

    /*
    struct sockaddr_in {
        sa_family_t    sin_family; // Address family (AF_INET)
        in_port_t      sin_port;   // Port number in network byte order
        struct in_addr sin_addr;   // Internet address
        char           sin_zero[8];// Padding to make struct same size as sockaddr
    };
    */

    void (*launch) (Server *server);
} Server;


Server server_constructor(int domain, int service, int protocol, uint32_t interface, int port, int queue, void (*launch)(Server *server));


#endif /* Server_h */