#include <stdlib.h>
#include "engine/system/graphic.h"

struct graphic_session {
    int none;
};
struct graphic_session *graphic_session_create()
{
    struct graphic_session *session = malloc(sizeof(struct graphic_session));
    return session;
}
int graphic_session_destroy(struct graphic_session *session)
{
    free(session);
    return 0;
}
int graphic_session_reset_window(struct graphic_session *session, void *native_window_handle)
{
    return 0;
}
struct graphic_session_info graphic_session_info_get(struct graphic_session *session)
{
    struct graphic_session_info info = { 200, 200 };
    return info;
}
void graphic_clear(float r, float g, float b) { }
void graphic_render(struct graphic_session *session) { }
struct graphic_texture {
    int none;
};
struct graphic_texture *graphic_texture_create(int width, int height, const unsigned char *bitmap, char is_mask)
{
    struct graphic_texture *texture = malloc(sizeof(struct graphic_texture));
    return texture;
}
void graphic_texture_destroy(struct graphic_texture *texture)
{
    free(texture);
}
struct graphic_vertecies {
    int none;
};
struct graphic_vertecies *graphic_vertecies_create(const float *verts, size_t vert_count)
{
    struct graphic_vertecies *vertecies = malloc(sizeof(struct graphic_vertecies));
    return vertecies;
}
void graphic_vertecies_destroy(struct graphic_vertecies *vertecies)
{
    free(vertecies);
}
void graphic_draw(struct graphic_vertecies *vertecies, struct graphic_texture *texture, mat4 mvp, vec3 color) { }
void graphic_construct_3D_quad(float *verts, rect2D dimension, rect2D tex) { }

