#ifndef GRAPHIC_H
#define GRAPHIC_H
#include "system/graphic/matrix.h"
#include <stddef.h>

struct graphic_session;

struct graphic_session *graphic_session_create();

int graphic_session_reset_window(struct graphic_session *session, void *native_window_handle);

int graphic_session_destroy(struct graphic_session *session);

struct graphic_session *graphic_session_create();



struct graphic_draw_ctx;

struct graphic_codepoint {
    float xadv, xoff, yoff, x0, y0, x1, y1;
};
struct graphic_font_ctx;

struct graphic_font_ctx *graphic_font_ctx_create(float width, float height, char *bitmap,
                                                        unsigned int cp_len, struct graphic_codepoint *cps);

struct graphic_draw_ctx *graphic_draw_ctx_create(const float *verts, size_t len);

void graphic_draw(struct graphic_draw_ctx *ctx, mat4 mvp);

void graphic_clear(float r, float g, float b);

void graphic_render(struct graphic_session *session);

#endif
