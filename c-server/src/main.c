/**
 * @file    main.c
 * @author  Samandar Komil
 * @date    18 April 2025
 *
 * @brief   Main file to run the program.
 */

#include <unistd.h>
#include <signal.h>
#include "common.h"
#include "./http/server.h"

void handle_signal(int sig);

HTTPServer *httpserver_ptr = NULL;

int main(void)
{
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGSEGV, handle_signal);

    HTTPServer *httpserver_ptr = httpserver_constructor(8081);
    if (!httpserver_ptr)
    {
        fprintf(stderr, "Failed to create HTTPServer instance.\n");
        return EXIT_FAILURE;
    }

    int is_launched = httpserver_ptr->launch(httpserver_ptr);
    if (is_launched < 0)
    {
        fprintf(stderr, "HTTPServer launch function faced an error, with code %d.\n", is_launched);
        httpserver_destructor(httpserver_ptr);
        return EXIT_FAILURE;
    }

    httpserver_destructor(httpserver_ptr);
    return EXIT_SUCCESS;
}

void handle_signal(int sig)
{
    switch (sig)
    {
    case SIGINT:
        fprintf(stderr, "\n\033[31m[!] SIGINT received. Cleaning up...\033[0m\n");
        break;
    case SIGSEGV:
        fprintf(stderr, "\n\033[31m[!] SIGSEGV received. Possible segmentation fault.\033[0m\n");
        break;
    case SIGTERM:
        fprintf(stderr, "\n\033[31m[!] SIGTERM received. Terminating gracefully...\033[0m\n");
        break;
    default:
        fprintf(stderr, "\n\03[33m[!] Signal %d received.\033[0m\n", sig);
    }

    if (httpserver_ptr)
    {
        httpserver_destructor(httpserver_ptr);
        httpserver_ptr = NULL;
    }

    exit(EXIT_FAILURE); // Non-zero to indicate abnormal termination
}