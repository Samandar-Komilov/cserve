#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#define LOG(level, fmt, ...) log_message(level, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

void log_message(const char *level, const char *file, int line, const char *fmt, ...);