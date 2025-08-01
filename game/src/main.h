#ifndef MAIN_H
#define MAIN_H
#include "system/graphic/graphic.h"

enum mouse_state {
    MOUSE_HOLD,
    MOUSE_HOVER,
    MOUSE_DOWN,
    MOUSE_UP
};
struct mouse_event {
    enum mouse_state state;
    float mouse_x, mouse_y;
};

int game_init(struct graphic_session *graphic);

void game_update();

void game_mouse_event(struct mouse_event mevent);

#endif
