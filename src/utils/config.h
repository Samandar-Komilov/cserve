/**
 * @file    config.h
 * @author  Samandar Komil
 * @date    30 April 2025
 *
 * @brief   Project configurations definitions.
 */

#ifndef CONFIGS_CONFIG_H
#define CONFIGS_CONFIG_H

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "linux/limits.h"
#include "common.h"

typedef struct
{
    int port;
    char *root;
    char *static_dir;
    char **backends;
    size_t backend_count;
} Config;

char *strip_whitespace(char *str);
Config *parse_config(const char *filename);
void free_config(Config *cfg);

#endif /* CONFIGS_CONFIG_H */