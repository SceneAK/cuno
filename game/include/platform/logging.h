#pragma once

#ifdef __ANDROID__
#include <android/log.h>
#define LOG(msg) __android_log_print(ANDROID_LOG_INFO, "CUNO", msg)

#else
#include <stdio.h>
#define LOG(msg) printf(msg)

#endif
