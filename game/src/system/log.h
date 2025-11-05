#ifndef LOG_H
#define LOG_H

#define LOG_INFO 0
#define LOG_WARN 1
#define LOG_ERR  2

void cuno_logf(int lvl, const char *fmt, ...);

#ifdef NDEBUG
#define LOGF(lvl, fmt, ...) \
    do { \
        if (lvl != CUNO_LOG_INFO) \
            cuno_logf(lvl, fmt, __VA_ARGS__) \
    } while (0)
#else
#define LOGF(lvl, fmt, ...) cuno_logf(lvl, fmt, __VA_ARGS__)
#endif

#define LOG(lvl, str) cuno_logf(lvl, "%s", str)

#endif
