#include <android_native_app_glue.h>
#include "main.h"
#include "system/graphic/graphic.h"
#include "system/logging.h"

static struct graphic_session *session = NULL;

void on_window_resized(struct android_app *app) { }

void on_init_window(struct android_app *app)
{
    if (graphic_session_switch_window(session, app->window) != 0)
        LOG("ERR: Switch window failed");
}

void on_destroy()
{
    if (session != NULL)
        graphic_session_destroy(session);
}

void onAppCmd(struct android_app *app, int32_t cmd)
{
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            on_init_window(app);
            init(session);
            break;
        case APP_CMD_WINDOW_RESIZED:
            on_window_resized(app);
            break;
        case APP_CMD_DESTROY:
            on_destroy();
            break;
    }
}

void android_main(struct android_app *app) 
{
    app->onAppCmd = onAppCmd;
    session = graphic_session_create();
    while (!app->destroyRequested) {
        int events;
        struct android_poll_source *source;
        while (ALooper_pollOnce(0, NULL, &events, (void**)&source) >= 0) {
            if (source) 
                source->process(app, source);
        }
        update();
    }
}
