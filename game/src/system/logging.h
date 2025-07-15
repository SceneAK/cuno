#ifndef LOGGING_H
#define LOGGING_H

#ifdef __ANDROID__
#include <android/log.h>
#define LOG(msg) __android_log_print(ANDROID_LOG_INFO, "CUNO", "%s", (msg))
#define LOGF(msg, ...) __android_log_print(ANDROID_LOG_INFO, "CUNO", (msg), __VA_ARGS__)

#else
#include <stdio.h>
#define LOG(msg) printf(msg)
#define LOGF(msg, ...) printf((msg), __VA_ARGS__)

#endif

#endif
