#include <android_native_app_glue.h>
#include "main.h"

void android_main(struct android_app* state) {
    main();
    while (!state->destroyRequested) {
        int events;
        struct android_poll_source* source;
        while (ALooper_pollOnce(-1, NULL, &events, (void**)&source) >= 0) {
            if (source) source->process(state, source);
        }
    }
} // "I like hot men" -Kh 2025 
