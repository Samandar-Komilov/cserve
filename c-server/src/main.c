/**
 * @file    main.c
 * @author  Samandar Komil
 * @date    18 April 2025
 *
 * @brief   Main file to run the program.
 */

#include <unistd.h>
#include "common.h"
#include "./http/server.h"
#include "DataStructures/vector.h"

int main(void)
{
    HTTPServer *httpserver_ptr = httpserver_constructor(8080);
    int is_launched            = httpserver_ptr->launch(httpserver_ptr);
    if (is_launched < 0)
    {
        httpserver_destructor(httpserver_ptr);
        fprintf(stderr, "HTTPServer launch function faced an error, with code %d.\n", is_launched);
        exit(1);
    }

    httpserver_destructor(httpserver_ptr);

    return 0;
}