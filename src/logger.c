#include "logger.h"

#include <stdarg.h>
#include <stdio.h>

static FILE *log_fp = NULL;

int logger_open(const char *path)
{
    log_fp = fopen(path, "a");
    if (!log_fp) {
        return -1;
    }

    return 0;
}

void logger_close(void)
{
    if (log_fp) {
        fclose(log_fp);
    }
}

void log_line(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    fflush(stdout);
    va_end(args);

    if (log_fp) {
        va_start(args, fmt);
        vfprintf(log_fp, fmt, args);
        fflush(log_fp);
        va_end(args);
    }
}
