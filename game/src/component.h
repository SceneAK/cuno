#ifndef COMPONENT_H
#define COMPONENT_H
#include "system/graphic/graphic.h"
#include "utils.h"

struct comp_visual {
    struct graphic_vertecies   *vertecies;
    struct graphic_texture     *texture;
    mat4                       *model;
    vec3                        color;
};
void comp_visual_set_default(struct comp_visual *visual);

void comp_visual_draw(struct comp_visual *visual, mat4 *perspective);

DECLARE_BASIC_POOL(struct comp_visual, comp_visual_pool)

void comp_visual_pool_draw(struct comp_visual_pool *pool, mat4 *perspective);


struct comp_transform {
    struct comp_transform  *parent;
    vec3                    trans, rot, scale;
    char                    desynced; /* User is responsible to mark desynced if above fields change */

    mat4                    model;
};
void comp_transform_set_default(struct comp_transform *transf);

void comp_transform_sync_model(struct comp_transform *transf);


DECLARE_BASIC_POOL(struct comp_transform, comp_transform_pool)

void comp_transform_pool_sync_model(struct comp_transform_pool *pool);

#endif
