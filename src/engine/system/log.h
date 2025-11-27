#ifndef LOG_H
#define LOG_H

#define LOG_INFO 0
#define LOG_WARN 1
#define LOG_ERR  2

void cuno_logf(int lvl, const char *fmt, ...);

#endif
