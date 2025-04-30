/**
 * @file    config.c
 * @author  Samandar Komil
 * @date    30 April 2025
 *
 * @brief   Project configurations implementations.
 */

#include "config.h"

/**
 * @brief   Parses a config file and initializes a Config struct.
 *
 * This function takes a filename as input and returns a pointer to a Config
 * struct. The Config struct contains the parsed values from the config file.
 *
 * The config file should contain the following format:
 * <key>=<value>
 *
 * The following keys are accepted:
 * - port
 * - root
 * - static_dir
 * - backend
 *
 * If a key is not recognized, it will be ignored.
 *
 * If a key is repeated, the last value will be used.
 *
 * The backend key can be repeated multiple times to specify multiple backends.
 *
 * The function returns a pointer to a Config struct if the config file is
 * parsed successfully, otherwise it returns NULL.
 */
Config *parse_config(const char *filename)
{
    char full_path[PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%s/%s", DEFAULT_CONFIG_PATH, filename);
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        perror("Could not open config file");
        return NULL;
    }

    Config *cfg        = calloc(1, sizeof(Config));
    cfg->backends      = calloc(MAX_BACKENDS, sizeof(char *));
    cfg->backend_count = 0;

    char line[512];
    while (fgets(line, sizeof(line), f))
    {
        if (line[0] == '#' || line[0] == '\n') continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;

        char *key   = strip_whitespace(line);
        char *value = strip_whitespace(eq + 1);

        if (strcmp(key, "port") == 0)
        {
            cfg->port = atoi(value);
        }
        else if (strcmp(key, "root") == 0)
        {
            cfg->root = strdup(value);
        }
        else if (strcmp(key, "static_dir") == 0)
        {
            cfg->static_dir = strdup(value);
        }
        else if (strcmp(key, "backend") == 0)
        {
            if (cfg->backend_count < MAX_BACKENDS)
            {
                cfg->backends[cfg->backend_count++] = strdup(value);
            }
        }
    }

    fclose(f);
    return cfg;
}

/**
 * @brief   Removes leading and trailing whitespace from a string.
 *
 * This function takes a string as input and trims any leading and trailing
 * whitespace characters. The function directly modifies the input string
 * and returns a pointer to the trimmed string.
 *
 * @param   str   The input string to be trimmed.
 *
 * @returns A pointer to the trimmed string, with leading and trailing
 *          whitespace removed.
 *
 * @example
 * char text[] = "  Hello, World!  ";
 * char *trimmed = strip_whitespace(text);
 * // trimmed now points to "Hello, World!"
 */
char *strip_whitespace(char *str)
{
    while (isspace(*str))
        str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
        *end-- = '\0';
    return str;
}

void free_config(Config *cfg)
{
    for (size_t i = 0; i < cfg->backend_count; ++i)
    {
        free(cfg->backends[i]);
    }
    free(cfg->backends);
    free(cfg->root);
    free(cfg->static_dir);
    free(cfg);
}