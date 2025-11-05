#ifndef MAIN_H
#define MAIN_H
#include "system/graphic.h"

struct mouse_event {
    enum {
        MOUSE_HOLD,
        MOUSE_HOVER,
        MOUSE_DOWN,
        MOUSE_UP
    } type;
    float mouse_x, mouse_y;
};
int gui_init(struct graphic_session *graphic);
void gui_update(void);
void gui_mouse_event(struct mouse_event mouse_event);

#endif
