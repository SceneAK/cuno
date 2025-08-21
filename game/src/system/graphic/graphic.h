#ifndef GRAPHIC_H
#define GRAPHIC_H
#include <stddef.h>
#include "game_math.h"

#define VERT_NUM_OF_ELEMENTS 5
#define VERT_SIZE (VERT_NUM_OF_ELEMENTS*sizeof(float))

struct graphic_session_info {
    int width, height;
};

struct graphic_session;

struct graphic_session *graphic_session_create();

int graphic_session_destroy(struct graphic_session *session);

struct graphic_session_info graphic_session_info_get(struct graphic_session *session);

int graphic_session_reset_window(struct graphic_session *session, void *native_window_handle);


struct graphic_vertecies;

struct graphic_texture;

struct graphic_texture *graphic_texture_create(float width, float height, const unsigned char *bitmap);

void graphic_texture_destroy(struct graphic_texture *texture);

struct graphic_vertecies *graphic_vertecies_create(const float *verts, size_t vert_count);

void graphic_vertecies_destroy(struct graphic_vertecies *ctx);

void graphic_draw(struct graphic_vertecies *ctx, struct graphic_texture *tex, mat4 mvp);

void graphic_clear(float r, float g, float b);

void graphic_render(struct graphic_session *session);

mat4 mat4_perspective(float fov_y, float aspect, float near);

/* Utils */
typedef struct {
    float x0; float y0; 
    float x1; float y1;
} rect2D;

void construct_3D_quad(float *verts, rect2D dimension, rect2D tex);
#endif
