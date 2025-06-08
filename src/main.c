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
#include "utils/config.h"
#include "http/server.h"

void handle_signal(int sig);

HTTPServer *httpserver_ptr = NULL;

int main(void)
{
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGSEGV, handle_signal);
    signal(SIGPIPE, SIG_IGN);

    Config *cfg = parse_config("cserver.ini");
    if (!cfg)
    {
        LOG("ERROR", "Failed to parse config file.");
        return EXIT_FAILURE;
    }

    HTTPServer *httpserver_ptr = httpserver_init(cfg->port);
    if (!httpserver_ptr)
    {
        LOG("ERROR", "Failed to create HTTPServer instance.");
        return EXIT_FAILURE;
    }

    int is_launched = httpserver_ptr->launch(httpserver_ptr);
    if (is_launched < 0)
    {
        LOG("ERROR", "HTTPServer launch function faced an error, with code %d.", is_launched);
        httpserver_cleanup(httpserver_ptr);
        return EXIT_FAILURE;
    }

    httpserver_cleanup(httpserver_ptr);
    free_config(cfg);

    return EXIT_SUCCESS;
}

void handle_signal(int sig)
{
    // Prevent recursive signal handling
    static volatile sig_atomic_t handling_signal = 0;
    if (handling_signal)
    {
        _exit(EXIT_FAILURE);
    }
    handling_signal = 1;

    // Log the signal
    switch (sig)
    {
    case SIGINT:
        fprintf(stderr, "\n\033[31m[!] SIGINT received. Cleaning up...\033[0m\n");
        break;
    case SIGSEGV:
        fprintf(
            stderr,
            "\n\033[31m[!] SIGSEGV received. Possible segmentation fault. Cleaning up...\033[0m\n");
// Print backtrace if available
#ifdef HAVE_EXECINFO_H
        void *array[10];
        size_t size    = backtrace(array, 10);
        char **strings = backtrace_symbols(array, size);
        if (strings != NULL)
        {
            fprintf(stderr, "\n\033[33m[!] Backtrace:\033[0m\n");
            for (size_t i = 0; i < size; i++)
            {
                fprintf(stderr, "  %s\n", strings[i]);
            }
            free(strings);
        }
#endif
        break;
    case SIGTERM:
        fprintf(stderr, "\n\033[31m[!] SIGTERM received. Terminating gracefully...\033[0m\n");
        break;
    case SIGABRT:
        fprintf(stderr, "\n\033[31m[!] SIGABRT received. Aborting...\033[0m\n");
        break;
    case SIGFPE:
        fprintf(stderr, "\n\033[31m[!] SIGFPE received. Floating point exception...\033[0m\n");
        break;
    case SIGILL:
        fprintf(stderr, "\n\033[31m[!] SIGILL received. Illegal instruction...\033[0m\n");
        break;
    case SIGBUS:
        fprintf(stderr, "\n\033[31m[!] SIGBUS received. Bus error...\033[0m\n");
        break;
    default:
        fprintf(stderr, "\n\033[33m[!] Signal %d received. Cleaning up...\033[0m\n", sig);
    }

    // Clean up resources
    if (httpserver_ptr)
    {
        // First, close all client connections
        if (httpserver_ptr->conn_pool)
        {
            for (int i = 0; i < MAX_CONNECTIONS; i++)
            {
                Connection *conn = httpserver_ptr->conn_pool->connections[i];
                if (conn->socket > 0)
                {
                    close(conn->socket);
                    conn->socket = -1;
                }
                if (conn->buffer)
                {
                    free(conn->buffer);
                    conn->buffer = NULL;
                }
                if (conn->current_request)
                {
                    http_request_destroy(conn->current_request);
                    conn->current_request = NULL;
                }
            }
        }

        // Close epoll instance
        if (httpserver_ptr->epoll_fd > 0)
        {
            close(httpserver_ptr->epoll_fd);
            httpserver_ptr->epoll_fd = -1;
        }

        // Clean up the server
        httpserver_cleanup(httpserver_ptr);
        httpserver_ptr = NULL;
    }

    // Reset signal handlers to default
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGBUS, SIG_DFL);

    // Exit with appropriate status
    if (sig == SIGSEGV || sig == SIGABRT || sig == SIGFPE || sig == SIGILL || sig == SIGBUS)
    {
        _exit(EXIT_FAILURE); // Use _exit for fatal signals to avoid recursive cleanup
    }
    else
    {
        exit(EXIT_FAILURE);
    }
}

// Function to set up signal handlers
void setup_signal_handlers(void)
{
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart system calls if interrupted

    // Set up handlers for various signals
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
}

void init_config(void) {}