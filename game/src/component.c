#include <string.h>
#include "system/graphic/graphic.h"
#include "system/logging.h"
#include "game_math.h"
#include "component.h"

/* TRANSFORM */
void transform_set_default(struct transform *transf)
{
    transf->parent          = NULL;
    transf->trans           = VEC3_ZERO;
    transf->rot             = VEC3_ZERO;
    transf->scale           = VEC3_ONE;
    transf->synced          = 0;

    transf->matrix          = MAT4_IDENTITY;
    transf->matrix_version  = 0;
}
void transform_sync_matrix(struct transform *transf)
{
    if (transf->synced)
        return;

    transf->matrix = mat4_trs(transf->trans, transf->rot, transf->scale);
    if (transf->parent) {
        transform_sync_matrix(transf->parent);
        transf->matrix = mat4_mult(transf->parent->matrix, transf->matrix);
    }
    transf->matrix_version++;
    transf->synced = 1;
}

void transform_pool_sync_matrix(struct transform_pool *pool)
{
    int i; 
    for (i = 0; i < pool->allocated_len; i++) {
        if (!pool->active[i])
            continue;
        transform_sync_matrix(&pool->elements[i]);
    }
}

/* VISUAL */
void visual_set_default(struct visual *visual)
{
    visual->vertecies   = NULL;
    visual->texture     = NULL;
    visual->model       = &MAT4_IDENTITY;
    visual->color       = VEC3_ZERO;
}
void visual_draw(struct visual *visual, const mat4 *perspective)
{
    if (!visual->vertecies || !visual->model)
        return;
    graphic_draw(visual->vertecies, 
                 visual->texture, 
                 mat4_mult(*perspective, *visual->model),
                 visual->color);
}

void visual_pool_draw(struct visual_pool *pool, const mat4 *perspective)
{
    int i;
    for (i = 0; i < pool->allocated_len; i++) {
        if (!pool->active[i])
            continue;
        visual_draw(&pool->elements[i], perspective);
    }
}

/* HITRECT */
void hitrect_set_default(struct hitrect *hitrect)
{
    hitrect->transf             = NULL;
    hitrect->cached_matrix_inv  = MAT4_IDENTITY;
    hitrect->cached_version     = 0;

    hitrect->rect_type          = RECT_CAMSPACE;
    hitrect->rect               = RECT2D_ZERO;
}

static void hitrect_sync_matrix_inv(struct hitrect *hitrect)
{
    if (hitrect->cached_version == hitrect->transf->matrix_version)
        return;

    hitrect->cached_version = hitrect->transf->matrix_version;
    hitrect->cached_matrix_inv = mat4_invert(hitrect->transf->matrix);
}

int hitrect_check_hit(struct hitrect *hitrect, const vec2 *mouse_ndc, const vec3 *mouse_camspace_ray)
{
    if (!hitrect->transf)
        return 0;
    hitrect_sync_matrix_inv(hitrect);

    if (hitrect->rect_type == RECT_CAMSPACE)
        return origin_ray_intersects_rect(&hitrect->rect, &hitrect->cached_matrix_inv, *mouse_camspace_ray);
    else 
        return point_lands_on_rect(&hitrect->rect, &hitrect->cached_matrix_inv, vec3_create(mouse_ndc->x, mouse_ndc->y, 0));
}
