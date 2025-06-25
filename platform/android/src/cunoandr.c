#include <android_native_app_glue.h>
#include "main.h"
#include "system/graphic/graphic.h"
#include "system/logging.h"

static struct graphic_session *session = NULL;

void graphic_init(struct android_app *app)
{
    if (session == NULL)
        session = graphic_session_create();
    if (graphic_session_setup(session, app->window))
        LOG("SETUP FAILED");
}

void graphic_cleanup()
{
    if (session != NULL)
        graphic_session_destroy(session);
}

void onAppCmd(struct android_app *app, int32_t cmd)
{
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            graphic_init(app);
            break;
        case APP_CMD_DESTROY:
            graphic_cleanup();
            break;
    }
}

void android_main(struct android_app *app) 
{
    app->onAppCmd = onAppCmd;
    main();
    while (!app->destroyRequested) {
        int events;
        struct android_poll_source *source;
        while (ALooper_pollOnce(-1, NULL, &events, (void**)&source) >= 0) {
            if (source) 
                source->process(app, source);
            if (session != NULL)
                graphic_gay_test(session);
        }
    }
}

