/**
 * @file    main.c
 * @author  Samandar Komil
 * @date    18 April 2025
 *
 * @brief   Main file to run the program.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "Server.h"
#include "DataStructures/dynamic_array.h"

int main(void)
{
    // Server TCPServer = server_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, 8000, 10);
    // TCPServer.launch(&TCPServer);

    DynamicArray arr = dynamic_array_init(1);

    printf("===== Testing started =====\n");
    arr.append(&arr, 100);
    arr.append(&arr, 200);
    arr.list_elements(&arr);

    arr.insert(&arr, 1, 300);
    arr.list_elements(&arr);

    arr.pop(&arr);
    arr.list_elements(&arr);

    arr.pop_at(&arr, 0);
    arr.list_elements(&arr);

    dynamic_array_free(&arr);

    printf("===== Testing finished =====\n");

    return 0;
}