#include <android/log.h>
#include <android_native_app_glue.h>

void android_main(struct android_app* app) {
    app_dummy();

    __android_log_print(ANDROID_LOG_INFO, "CunoApp", "NativeActivity started!");
}
