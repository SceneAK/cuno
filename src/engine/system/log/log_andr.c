#include "engine/system/log.h"
#include <stdarg.h>
#include <android/log.h>

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
            __android_log_vprint(ANDROID_LOG_INFO, "CUNO", fmt, args);
            break;
        case LOG_WARN:
            __android_log_vprint(ANDROID_LOG_WARN, "CUNO", fmt, args);
            break;
        case LOG_ERR:
            __android_log_vprint(ANDROID_LOG_ERROR, "CUNO", fmt, args);
            break;
    };
    va_end(args);
}
