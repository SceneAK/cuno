#ifndef ENTITY_DOT_H
#define ENTITY_DOT_H
#include "engine/entity/world.h"
#include "engine/component.h"

static inline entity_t entity_dot_attatch_new(struct entity_world *ctx, entity_t parent, vec3 color, enum projection_type proj, float scale)
{
    entity_t dot = entity_record_activate(&ctx->entrec);

    struct comp_transform *transf;
    struct comp_visual *vis;
    struct transform transform;

    transf  = comp_system_transform_emplace(ctx->sys_transf, dot);
    vis     = comp_system_visual_emplace(ctx->sys_vis, dot, proj);
    comp_transform_set_default(transf);
    comp_visual_set_default(vis);
    transf->data.scale   = vec3_all(.1);
    vis->vertecies       = DEFAULT_SQUARE_VERT_GET();
    vis->color           = color;
    vis->draw_pass       = DRAW_PASS_OPAQUE;

    comp_system_family_adopt(ctx->sys_fam, parent, dot);

    transform            = comp_system_transform_get_world(ctx->sys_transf, dot);
    transf->data.scale   = vec3_create(scale/transform.trans.x, scale/transform.trans.y, scale/transform.trans.z);

    return dot;
}

#endif
