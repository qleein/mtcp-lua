#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>

#include "mtcp_lua_log.h"

static int log_level = 0;
static int output = STDOUT_FILENO;

static const char *log_level_string[] = {
    "debug", "info", "warn", "error", "fatal",
};

void
mtcp_lua_log_init(int level, const char *logfile)
{
    if (level < LOG_LEVEL_DEBUG || level > LOG_LEVEL_FATAL)
        return;

    log_level = level;

    if (!logfile)
        return;

    int fd = open(logfile, O_WRONLY | O_APPEND | O_CREAT,
                  S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        fprintf(stderr, "open %s failed, err: %s.\n", logfile, strerror(errno));
        return;
    }

    output = fd;
    return;
}

void
mtcp_lua_log(int level, const char *fmt, ...)
{
    if (level < LOG_LEVEL_DEBUG || level > LOG_LEVEL_FATAL)
        return;

    if (level < log_level)
        return;

    char buf[MAX_LOG_LENGTH];
    char *pos = buf;
    size_t last = MAX_LOG_LENGTH;
    int  n;

    time_t      t;
    struct tm   *tm;
    time(&t);
    tm = localtime(&t);
    n = strftime(pos, last, "%F %r %z ", tm);
    pos += n;
    last -= n;

    n = snprintf(pos, last, "[%s]: ", log_level_string[level]);
    pos += n;
    last -= n;

    va_list ap;
    va_start(ap, fmt);
    n = vsnprintf(pos, last, fmt, ap);
    if (n < 0) {
        return;
    }
    pos += n;
    last -= n;
    va_end(ap);

    *pos++ = '\n';
    write(output, buf, pos - buf);

    if (level >= LOG_LEVEL_FATAL) {
        exit(1);
    }
    return;
}


