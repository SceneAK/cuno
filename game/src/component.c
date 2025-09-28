#include <stdlib.h>
#include <string.h>
#include "system/graphic/graphic.h"
#include "system/logging.h"
#include "game_math.h"
#include "component.h"

/* SPARSE SET */
int component_pool_init(struct component_pool *pool, size_t initial_alloc_len, size_t elem_size)
{
    int i;

    for (i = 0; i < ENTITY_MAX; i++)
        pool->sparse[i] = COMP_INDEX_INVALID;

    pool->len = 0;
    pool->allocated_len = initial_alloc_len;
    pool->data = malloc(initial_alloc_len * elem_size);
    return pool->data != NULL;
}
void component_pool_deinit(struct component_pool *pool)
{
    free(pool->data);
}
void *component_pool_emplace(struct component_pool *pool, entity_t entity, size_t elem_size)
{
    if (entity_is_invalid(entity)) {
        LOG("COMP: (warn) Entity invalid");
        return NULL;
    }
    if (pool->sparse[entity] != COMP_INDEX_INVALID) {
        LOG("COMP: (warn) Component for this entity already exists. Cannot emplace.");
        return NULL;
    }

    pool->sparse[entity] = pool->len;
    pool->dense[pool->len] = entity;
    pool->len++;

    if (pool->len > pool->allocated_len) {
        pool->allocated_len *= 2;
        pool->data = realloc(pool->data, elem_size * pool->allocated_len);
    }
    return (char *)pool->data + (pool->len - 1)*elem_size;
}
int component_pool_erase(struct component_pool *pool, entity_t entity, size_t elem_size)
{
    entity_t last_entity;

    if (entity_is_invalid(entity)) {
        LOG("COMP: (warn) Entity invalid");
        return -1;
    }
    if (pool->sparse[entity] == COMP_INDEX_INVALID) {
        LOG("COMP: (warn) Component for this entity doesn't exist. Cannot erase.");
        return -2;
    }

    last_entity = pool->dense[--pool->len];
    pool->sparse[last_entity] = pool->sparse[entity];
    pool->dense[pool->sparse[entity]] = last_entity;
    memcpy((char *)pool->data + pool->sparse[entity] * elem_size, 
           (char *)pool->data + pool->len * elem_size,
           elem_size);
    pool->sparse[entity] = COMP_INDEX_INVALID;
    return 0;
}


/* TRANSFORM */
void comp_transform_set_default(struct comp_transform *transf)
{
    transf->trans           = VEC3_ZERO;
    transf->rot             = VEC3_ZERO;
    transf->scale           = VEC3_ONE;
    transf->synced          = 0;

    transf->matrix          = MAT4_IDENTITY;
    transf->matrix_version  = 0;
}
void comp_transform_system_mark_family_desync(struct comp_transform_system *system, entity_t family_parent)
{
    entity_t               child;
    struct comp_transform *transf;

    if (entity_is_invalid(family_parent)) {
        LOG("COMP_TRANSF: (warn) Entity invalid");
        return;
    }

    child = system->first_child_map[family_parent];
    transf = comp_transform_pool_try_get(&system->pool, family_parent);
    if (transf)
        transf->synced = 0;
     
    while (!entity_is_invalid(child)) {
        transf = comp_transform_pool_try_get(&system->pool, child);
        if (transf)
            transf->synced = 0;

        child = system->sibling_map[child];
    }
}
/* Should probably sort instead or maybe separate the matrices into their own array, but I can't be bothered lmao */
static void comp_transform_system_sync_matrix(struct comp_transform_system *system, size_t data_index)
{
    struct comp_transform *transf, *parent_transf;

    transf = system->pool.data + data_index;

    if (transf->synced)
        return;

    transf->matrix = mat4_trs(transf->trans, transf->rot, transf->scale);

    parent_transf = comp_transform_pool_try_get(&system->pool, system->parent_map[system->pool.dense[data_index]]);
    if (parent_transf) {
        comp_transform_system_sync_matrix(system, (parent_transf - system->pool.data));
        transf->matrix = mat4_mult(parent_transf->matrix, transf->matrix);
    }

    transf->matrix_version++;
    transf->synced = 1;
}
void comp_transform_system_sync_matrices(struct comp_transform_system *system)

{
    int i;
    struct comp_transform *transf, *parent_transf;

    for (i = 0; i < system->pool.len; i++)
        comp_transform_system_sync_matrix(system, i);
}


/* VISUAL */
void comp_visual_set_default(struct comp_visual *visual)
{
    visual->vertecies       = NULL;
    visual->texture         = NULL;
    visual->color           = VEC3_ZERO;
    visual->draw_pass       = 0;
}
static void comp_visual_transform_pool_draw(struct comp_visual_pool *pool, struct comp_transform_pool *transf_pool, const mat4 *projection)
{
    struct comp_visual    *visual;
    struct comp_transform *transf;
    short                  pass = 0,
                           total_pass = 1;
    int i;

    for(; pass < total_pass; pass++) {
        for (i = 0; i < pool->len; i++) {
            visual = pool->data + i;
            transf = comp_transform_pool_try_get(transf_pool, pool->dense[i]);

            if (visual->draw_pass > pass) {
                total_pass = visual->draw_pass + 1;
                continue;
            }

            graphic_draw(visual->vertecies, 
                         visual->texture, 
                         mat4_mult(*projection, transf ? transf->matrix : MAT4_IDENTITY),
                         visual->color);

        }
    }
}
void comp_visual_system_draw(struct comp_visual_system *vis_sys, const mat4 *persp, const mat4 *ortho)
{
    comp_visual_transform_pool_draw(&vis_sys->pool_ortho, vis_sys->transf_pool, ortho);
    comp_visual_transform_pool_draw(&vis_sys->pool_persp, vis_sys->transf_pool, persp);
}

/* HITRECT */
void comp_hitrect_set_default(struct comp_hitrect *hitrect)
{
    hitrect->cached_matrix_inv  = MAT4_IDENTITY;
    hitrect->cached_version     = 0;

    hitrect->rect_type          = RECT_CAMSPACE;
    hitrect->rect               = RECT2D_ZERO;
    hitrect->hitstate           = 0;
    hitrect->hitmask            = 1;
}
void comp_hitrect_system_update_hitstate(struct comp_hitrect_system *system, const vec2 *mouse_ortho, const vec3 *mouse_camspace_ray, char mask)
{
    struct comp_hitrect *hitrect;
    struct comp_transform *transf;
    int i;

    for (i = 0; i < system->pool.len; i++) {
        hitrect = system->pool.data + i;
        transf  = system->transf_pool->data + system->transf_pool->sparse[system->pool.dense[i]];

        if (!transf | !(hitrect->hitmask & mask))
            continue;

        if (hitrect->cached_version != transf->matrix_version) {
            hitrect->cached_version = transf->matrix_version;
            hitrect->cached_matrix_inv = mat4_invert(transf->matrix);
        }

        if (hitrect->rect_type == RECT_CAMSPACE)
            hitrect->hitstate = origin_ray_intersects_rect(&hitrect->rect, &hitrect->cached_matrix_inv, *mouse_camspace_ray);
        else 
            hitrect->hitstate = point_lands_on_rect(&hitrect->rect, &hitrect->cached_matrix_inv, vec3_create(mouse_ortho->x, mouse_ortho->y, 0));
    }
}
