#include <string.h>
#include "system/graphic/graphic.h"
#include "game_math.h"
#include "component.h"

void comp_visual_set_default(struct comp_visual *visual)
{
    visual->vertecies   = NULL;
    visual->texture     = NULL;
    visual->model       = NULL;
    visual->color       = VEC3_ZERO;
}
void comp_visual_draw(struct comp_visual *visual, mat4 *perspective)
{
    if (!visual->vertecies)
        return;
    graphic_draw(visual->vertecies, visual->texture, mat4_mult(*perspective, *visual->model), visual->color);

}

DEFINE_BASIC_POOL(struct comp_visual, comp_visual_pool);

void comp_visual_pool_render(struct comp_visual_pool *pool, mat4 *perspective)
{
    int i; 
    for (i = 0; i < pool->allocated_len; i++) {
        if (!pool->active[i])
            continue;
        comp_visual_draw(&pool->elements[i], perspective);
    }
}

mat4 mat4_model_transform(const struct comp_transform *transf)
{
    return mat4_model(transf->trans, transf->rot, transf->scale);
}
void comp_transform_set_default(struct comp_transform *transf)
{
    transf->parent  = NULL;
    transf->trans   = VEC3_ZERO;
    transf->rot     = VEC3_ZERO;
    transf->scale   = VEC3_ONE;
    transf->desynced = 0;
}
void comp_transform_sync_model(struct comp_transform *transf)
{
    if (!transf->desynced)
        return;

    transf->model = mat4_model_transform(transf);
    if (transf->parent) {
        comp_transform_sync_model(transf->parent);
        transf->model = mat4_mult(transf->parent->model, transf->model);
    }
    transf->desynced = 0;
}

DEFINE_BASIC_POOL(struct comp_transform, comp_transform_pool)

void comp_transform_pool_sync_model(struct comp_transform_pool *pool)
{
    int i; 
    for (i = 0; i < pool->allocated_len; i++) {
        if (!pool->active[i])
            continue;
        comp_transform_sync_model(&pool->elements[i]);
    }
}
