#include <android_native_app_glue.h>
#include <stdlib.h>
#include "system/graphic/graphic.h"
#include "system/logging.h"
#include "main.h"

#include "asset_management_andr.h"

static struct graphic_session *session = NULL;

void on_window_resized(struct android_app *app) {   }

int on_init_window(struct android_app *app)
{
    if (!session)
        session = graphic_session_create();

    if (graphic_session_reset_window(session, app->window) != 0) {
        LOG("ERR: Switch window failed");
        return -1;
    }
    return 0;
}

void on_destroy()
{
    if (session != NULL)
        graphic_session_destroy(session);
}

void on_app_cmd(struct android_app *app, int32_t cmd)
{
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (on_init_window(app) != 0)
                exit(EXIT_FAILURE);
            game_init(session);
            break;
        case APP_CMD_WINDOW_RESIZED:
            on_window_resized(app);
            break;
        case APP_CMD_DESTROY:
            on_destroy();
            break;
    }
}

int on_mouse_input(struct android_app *app, AInputEvent *event)
{
    struct mouse_event  mevent;
    int32_t             input_src = AInputEvent_getSource(event);
    int32_t             buttons, action;

    mevent.mouse_x = AMotionEvent_getX(event, 0);
    mevent.mouse_y = AMotionEvent_getY(event, 0);
    mevent.type = MOUSE_HOVER;

    buttons = AMotionEvent_getButtonState(event);
    action  = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;

    if (buttons & AMOTION_EVENT_BUTTON_PRIMARY)
        mevent.type = MOUSE_HOLD;

    if (action == AMOTION_EVENT_ACTION_DOWN)
        mevent.type = MOUSE_DOWN;
    else if (action == AMOTION_EVENT_ACTION_UP)
        mevent.type = MOUSE_UP;

    game_mouse_event(mevent);
    return 1;
}
int on_input(struct android_app *app, AInputEvent *event)
{
    switch (AInputEvent_getType(event)) {
        case AINPUT_EVENT_TYPE_MOTION:
            return on_mouse_input(app, event);
        default:
            return 0;
    }
}

void android_main(struct android_app *app) 
{LOG("ENTERED ANDROID MAIN");
    AAssetManager*      asset_mngr = app->activity->assetManager;

    asset_management_init(asset_mngr);

    app->onAppCmd = on_app_cmd;
    app->onInputEvent = on_input;

    while (!app->destroyRequested) {
        int32_t events;
        struct android_poll_source *source;
        while (ALooper_pollOnce(0, NULL, &events, (void**)&source) >= 0) {
            if (source) 
                source->process(app, source);
        }
        game_update();
    }
}
