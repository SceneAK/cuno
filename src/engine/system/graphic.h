#ifndef GRAPHIC_H
#define GRAPHIC_H
#include <stddef.h>
#include "engine/math.h"
#include "engine/utils.h"

/* Attribs arranged as { x, y, z, tex_x, tex_y } */
#define VERT_ELEM_COUNT 5
#define VERT_SIZE_BYTES (VERT_ELEM_COUNT*sizeof(float))
#define VERT_COUNT(arr) (ARRAY_SIZE(arr)/VERT_ELEM_COUNT)

struct graphic_session;
struct graphic_texture;
struct graphic_vertecies;
struct graphic_session_info {
    int width, height;
};

static const rect2D SQUARE_BOUNDS = { 0.0f, 0.0f, 1.0f,  1.0f };
static const float SQUARE_VERTS[]       = {
    SQUARE_BOUNDS.x0,  SQUARE_BOUNDS.y0, 0.0f, 0.0f, 0.0f,
    SQUARE_BOUNDS.x0,  SQUARE_BOUNDS.y1, 0.0f, 0.0f, 0.0f,
    SQUARE_BOUNDS.x1,  SQUARE_BOUNDS.y0, 0.0f, 0.0f, 0.0f,

    SQUARE_BOUNDS.x1,  SQUARE_BOUNDS.y1, 0.0f, 0.0f, 0.0f,
    SQUARE_BOUNDS.x0,  SQUARE_BOUNDS.y1, 0.0f, 0.0f, 0.0f,
    SQUARE_BOUNDS.x1,  SQUARE_BOUNDS.y0, 0.0f, 0.0f, 0.0f,
};

struct graphic_session *graphic_session_create();
int graphic_session_destroy(struct graphic_session *session);
struct graphic_session_info graphic_session_info_get(struct graphic_session *session);
int graphic_session_reset_window(struct graphic_session *session, void *native_window_handle);

struct graphic_texture *graphic_texture_create(int width, int height, const unsigned char *bitmap, char is_mask);
void graphic_texture_destroy(struct graphic_texture *texture);

struct graphic_vertecies *graphic_vertecies_create(const float *verts, size_t vert_count);
void graphic_vertecies_destroy(struct graphic_vertecies *ctx);
void graphic_draw(struct graphic_vertecies *ctx, struct graphic_texture *tex, mat4 mvp, vec3 color);
void graphic_clear(float r, float g, float b);
void graphic_render(struct graphic_session *session);

/* Utils */
void graphic_construct_3D_quad(float *verts, rect2D dimension, rect2D tex);

#endif
