#include "logger.h"

void log_message(const char *level, const char *file, int line, const char *fmt, ...)
{
    time_t now   = time(NULL);
    struct tm *t = localtime(&now);

    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);

    fprintf(stdout, "[%s] [%s] (%s:%d) ", buf, level, file, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    fprintf(stdout, "\n");
    fflush(stdout);
}
