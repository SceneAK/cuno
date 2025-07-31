#ifndef GRAPHIC_H
#define GRAPHIC_H
#include "system/graphic/matrix.h"
#include <stddef.h>

#define VERT_NUM_OF_ELEMENTS 5
#define VERT_SIZE (VERT_NUM_OF_ELEMENTS*sizeof(float))

struct graphic_session;

struct graphic_session *graphic_session_create();

int graphic_session_reset_window(struct graphic_session *session, void *native_window_handle);

int graphic_session_destroy(struct graphic_session *session);

struct graphic_session *graphic_session_create();


struct graphic_draw_ctx;

struct graphic_texture;

struct graphic_draw_ctx *graphic_draw_ctx_create(const float *verts, size_t vert_count, struct graphic_texture *texture);

struct graphic_texture *graphic_texture_create(float width, float height, const unsigned char *bitmap);

void graphic_draw(struct graphic_draw_ctx *ctx, mat4 mvp);

void graphic_clear(float r, float g, float b);

void graphic_render(struct graphic_session *session);

#endif
