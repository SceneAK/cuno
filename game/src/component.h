#ifndef COMPONENT_H
#define COMPONENT_H
#include "system/graphic/graphic.h"
#include "utils.h"

struct transform {
    struct transform       *parent;
    vec3                    trans, rot, scale;
    char                    synced;

    unsigned short          matrix_version;
    mat4                    matrix;
};
void transform_set_default(struct transform *transf);

void transform_sync_matrix(struct transform *transf);

DEFINE_BASIC_POOL_WRAPPER(static, struct transform, transform_pool)

void transform_pool_sync_matrix(struct transform_pool *pool);


struct visual {
    struct graphic_vertecies   *vertecies;
    struct graphic_texture     *texture;
    const mat4                 *model;
    vec3                        color;
};
void visual_set_default(struct visual *visual);

void visual_draw(struct visual *visual, const mat4 *perspective);

DEFINE_BASIC_POOL_WRAPPER(static, struct visual, visual_pool)

void visual_pool_draw(struct visual_pool *pool, const mat4 *perspective);


struct hitrect {
    const struct transform *transf;
    mat4                    cached_matrix_inv;
    unsigned short          cached_version;

    enum {
        RECT_CAMSPACE, 
        RECT_ORTHOSPACE
    }                       rect_type;
    rect2D                  rect;
};
void hitrect_set_default(struct hitrect *hitrect);

int hitrect_check_hit(struct hitrect *hitrect, const vec2 *mouse_ortho, const vec3 *mouse_camspace_ray);

#endif
