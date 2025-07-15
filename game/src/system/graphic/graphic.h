#ifndef GRAPHIC_H
#define GRAPHIC_H
#include "system/graphic/matrix.h"

struct graphic_session;

struct graphic_session *graphic_session_create();

int graphic_session_reset_window(struct graphic_session *session, void *native_window_handle);

int graphic_session_destroy(struct graphic_session *session);

void graphic_draw_default(struct graphic_session *session, mat4 model);

#endif
