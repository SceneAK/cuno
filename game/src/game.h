#ifndef MAIN_H
#define MAIN_H
#include "system/graphic/graphic.h"

struct mouse_event {
    enum {
        MOUSE_HOLD,
        MOUSE_HOVER,
        MOUSE_DOWN,
        MOUSE_UP
    } type;
    float mouse_x, mouse_y;
};
int game_init(struct graphic_session *graphic);
void game_update();
void game_mouse_event(struct mouse_event mevent);

#endif
