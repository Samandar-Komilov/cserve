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
#include "HTTPServer.h"
#include "DataStructures/vector.h"

int main(void)
{
    HTTPServer *httpserver_ptr = httpserver_constructor(8080);
    httpserver_ptr->launch(httpserver_ptr);

    printf("%s >>> %s >>> %s\n", httpserver_ptr->request[0], httpserver_ptr->request[1],
           httpserver_ptr->request[2]);

    httpserver_destructor(httpserver_ptr);

    return 0;
}