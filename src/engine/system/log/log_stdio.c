#include "engine/log.h"
#include <stdarg.h>
#include <stdio.h>

void cuno_logf(int lvl, const char *fmt, ...)
{
#ifdef NDEBUG
    if (lvl == CUNO_LOG_INFO)
        return;
#endif

    va_list args;
    va_start(args, fmt);
    switch(lvl) {
        case LOG_INFO:
            vfprintf(stdout, fmt, args);
            break;
        case LOG_WARN:
            vfprintf(stderr, fmt, args);
            break;
        case LOG_ERR:
            vfprintf(stderr, fmt, args);
            break;
    };
    va_end(args);
}
