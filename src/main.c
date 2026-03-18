/**
 * @file    main.c
 * @author  Samandar Komil
 * @date    18 April 2025
 *
 * @brief   Main file to run the program.
 */

#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "common.h"
#include "utils/config.h"
#include "http/server.h"

/* SEC-04: Global shutdown flag -- NOT static, shared via extern in server.h */
volatile sig_atomic_t shutdown_flag = 0;

static void handle_signal(int sig)
{
    (void)sig;
    shutdown_flag = 1;
}

HTTPServer *httpserver_ptr = NULL;

int main(void)
{
    /* SEC-04: Use sigaction instead of signal() for reliable behavior */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* SIGPIPE ignored -- broken pipe should not crash the server */
    signal(SIGPIPE, SIG_IGN);

    /* SIGSEGV deliberately NOT caught -- crash with core dump for debugging */

    Config *cfg = parse_config("cserver.ini");
    if (!cfg)
    {
        LOG("ERROR", "Failed to parse config file.");
        return EXIT_FAILURE;
    }

    httpserver_ptr =
        httpserver_constructor(cfg->port, cfg->static_dir, cfg->backends, cfg->backend_count);
    if (!httpserver_ptr)
    {
        LOG("ERROR", "Failed to create HTTPServer instance.");
        return EXIT_FAILURE;
    }

    int is_launched = httpserver_ptr->launch(httpserver_ptr);
    if (is_launched < 0)
    {
        LOG("ERROR", "HTTPServer launch function faced an error, with code %d.", is_launched);
        httpserver_destructor(httpserver_ptr);
        return EXIT_FAILURE;
    }

    httpserver_destructor(httpserver_ptr);
    free_config(cfg);

    return EXIT_SUCCESS;
}

void init_config(void) {}
