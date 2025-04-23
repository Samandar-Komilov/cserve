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
#include "DataStructures/dynamic_array.h"

int main(void)
{
    HTTPServer httpserver = httpserver_constructor(8080);
    httpserver.launch(&httpserver);

    return 0;
}