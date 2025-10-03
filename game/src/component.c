#include <stdlib.h>
#include <string.h>
#include "system/graphic.h"
#include "system/log.h"
#include "gmath.h"
#include "component.h"

#define DEFINE_COMPONENT_POOL_STRUCT(type, name) \
    struct name { \
        short           sparse[ENTITY_MAX]; \
        entity_t        dense[ENTITY_MAX]; \
        size_t          len, \
                        allocated_len; \
        type           *data; \
    };
#define DEFINE_COMPONENT_POOL_INIT(type, name) \
    int name##_init(struct name *pool, size_t initial_alloc_len) \
    { \
        return component_pool_init((struct component_pool *)pool, initial_alloc_len, sizeof(type)); \
    }
#define DEFINE_COMPONENT_POOL_DEINIT(type, name) \
    void name##_deinit(struct name *pool) \
    { \
        component_pool_deinit((struct component_pool *)pool); \
    }
#define DEFINE_COMPONENT_POOL_EMPLACE(type, name) \
    type *name##_emplace(struct name *pool, entity_t entity) \
    { \
        return (type *)component_pool_emplace((struct component_pool *)pool, entity, sizeof(type)); \
    }
#define DEFINE_COMPONENT_POOL_ERASE(type, name) \
    int name##_erase(struct name *pool, entity_t entity) \
    { \
        return component_pool_erase((struct component_pool *)pool, entity, sizeof(type)); \
    }
#define DEFINE_COMPONENT_POOL_TRY_GET(type, name) \
    type *name##_try_get(struct name *pool, entity_t entity) \
    { \
        if (entity == ENTITY_INVALID || pool->sparse[entity] == -1) \
            return NULL; \
        return pool->data + pool->sparse[entity];  \
    }
#define DEFINE_COMPONENT_POOL(static_inline, type, name) \
    DEFINE_COMPONENT_POOL_STRUCT(type, name) \
    static_inline DEFINE_COMPONENT_POOL_INIT(type, name) \
    static_inline DEFINE_COMPONENT_POOL_DEINIT(type, name) \
    static_inline DEFINE_COMPONENT_POOL_EMPLACE(type, name) \
    static_inline DEFINE_COMPONENT_POOL_ERASE(type, name) \
    static_inline DEFINE_COMPONENT_POOL_TRY_GET(type, name)


/* SPARSE ENTITY SET*/
struct component_pool {
    short           sparse[ENTITY_MAX];
    entity_t        dense[ENTITY_MAX];
    size_t          len,
                    allocated_len;
    void           *data;
};

int component_pool_init(struct component_pool *pool, size_t initial_alloc_len, size_t elem_size)
{
    int i;

    for (i = 0; i < ENTITY_MAX; i++)
        pool->sparse[i] = -1;

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
        LOGF("COMP: (warn) Entity %d is invalid. Cannot emplace.", entity);
        return NULL;
    }
    if (pool->sparse[entity] != -1) {
        LOGF("COMP: (warn) Component for entity %d already exists. Cannot emplace.", entity);
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
        LOG("COMP: (warn) Entity invalid. Cannot erase.");
        return -1;
    }
    if (pool->sparse[entity] == -1) {
        LOG("COMP: (warn) Component for this entity doesn't exist. Cannot erase.");
        return -2;
    }

    last_entity = pool->dense[--pool->len];
    pool->sparse[last_entity] = pool->sparse[entity];
    pool->dense[pool->sparse[entity]] = last_entity;
    memcpy((char *)pool->data + pool->sparse[entity] * elem_size, 
           (char *)pool->data + pool->len * elem_size,
           elem_size);
    pool->sparse[entity] = -1;
    return 0;
}



#define COMP_POOL_INITIAL_ALLOC 32

struct comp_system_family {
    struct comp_system          base;
    entity_t                    parent_map[ENTITY_MAX];
    entity_t                    first_child_map[ENTITY_MAX];
    entity_t                    sibling_map[ENTITY_MAX];
};
DEFINE_COMPONENT_POOL(static, struct comp_transform, comp_pool_transform)
struct comp_system_transform {
    struct comp_system          base;
    struct comp_pool_transform  pool;
    struct comp_system_family  *sys_family;
};
DEFINE_COMPONENT_POOL(static, struct comp_hitrect, comp_pool_hitrect)
struct comp_system_hitrect {
    struct comp_system          base;
    struct comp_pool_hitrect    pool;
    struct comp_pool_transform *pool_transf;
};
DEFINE_COMPONENT_POOL(static, struct comp_visual, comp_pool_visual)
struct comp_system_visual {
    struct comp_system          base;
    struct comp_pool_visual     pool_ortho;
    struct comp_pool_visual     pool_persp;
    struct comp_pool_transform *pool_transf;
};

struct comp_system_family *comp_system_family_create(struct comp_system base)
{
    int i;
    struct comp_system_family *sys = malloc(sizeof(struct comp_system_family));
    if (!sys)
        return NULL;

    sys->base = base;

    for (i = 0; i < ENTITY_MAX; i++) {
        sys->parent_map[i] = ENTITY_INVALID;
        sys->first_child_map[i] = ENTITY_INVALID;
        sys->sibling_map[i] = ENTITY_INVALID;
    }
    return sys;
}
struct comp_system_family_view comp_system_family_view(struct comp_system_family *sys)
{
    struct comp_system_family_view view = {
        sys->parent_map,
        sys->first_child_map,
        sys->sibling_map
    };
    return view;
}
size_t comp_system_family_count_children(struct comp_system_family *sys, entity_t entity)
{
    size_t      count = 0;
    entity_t    current;

    if (entity_is_invalid(entity)) {
        LOGF("COMPSYS_FAM: (warn) Entity %d is invalid. Cannot count children.", entity);
        return SIZE_MAX;
    }

    current = sys->first_child_map[entity];
    while (!entity_is_invalid(current)) {
        count++;
        current = sys->sibling_map[current];
    }

    return count;
}
entity_t comp_system_family_find_previous_sibling(struct comp_system_family *sys, entity_t entity)
{
    entity_t current,
             last = ENTITY_INVALID;

    if (entity_is_invalid(sys->parent_map[entity]))
        return -1;
    current = sys->first_child_map[ sys->parent_map[entity] ];

    while (!entity_is_invalid(current)) {
        if (current == entity)
            break;

        last = current;
        current = sys->sibling_map[current];
    }
    return last;
}
entity_t comp_system_family_find_last_child(struct comp_system_family *sys, entity_t parent)
{
    entity_t current;

    if (entity_is_invalid(parent))
        return ENTITY_INVALID;

    current = sys->first_child_map[parent];
    if (entity_is_invalid(current))
        return ENTITY_INVALID;

    while (!entity_is_invalid(sys->sibling_map[current])) {
        current = sys->sibling_map[current];
    }
    return current;
}
void comp_system_family_adopt(struct comp_system_family *sys, entity_t parent, entity_t entity)
{
    sys->parent_map[entity] = parent;
    if (entity_is_invalid(sys->first_child_map[parent]))
        sys->first_child_map[parent] = entity;
    else
        sys->sibling_map[comp_system_family_find_last_child(sys, parent)] = entity;

    entity_record_flag_component(sys->base.entity_record, entity, sys->base.component_flag);
}
int comp_system_family_disown(struct comp_system_family *sys, entity_t entity)
{
    entity_t prev; 

    if (entity_is_invalid(sys->parent_map[entity]))
        return -1;

    prev = comp_system_family_find_previous_sibling(sys, entity);
    if (entity_is_invalid(prev))
        sys->first_child_map[ sys->parent_map[entity] ] = sys->sibling_map[entity];
    else 
        sys->sibling_map[prev] = sys->sibling_map[entity];

    sys->parent_map[entity]  = ENTITY_INVALID;
    sys->sibling_map[entity] = ENTITY_INVALID;

    entity_record_unflag_component(sys->base.entity_record, entity, sys->base.component_flag);
    return 0;
}


void comp_transform_set_default(struct comp_transform *transf)
{
    transf->trans           = VEC3_ZERO;
    transf->rot             = VEC3_ZERO;
    transf->scale           = VEC3_ONE;
    transf->synced          = 0;

    transf->matrix          = MAT4_IDENTITY;
    transf->matrix_version  = 0;
}
struct comp_system_transform *comp_system_transform_create(struct comp_system base, struct comp_system_family *sys_fam)
{
    struct comp_system_transform *sys = malloc(sizeof(struct comp_system_transform));
    if (!sys)
        return NULL;
    
    sys->base = base;
    sys->sys_family = sys_fam;
    comp_pool_transform_init(&sys->pool, COMP_POOL_INITIAL_ALLOC);

    return sys;
}
struct comp_transform *comp_system_transform_emplace(struct comp_system_transform *sys, entity_t entity)
{
    struct comp_transform *transf = comp_pool_transform_emplace(&sys->pool, entity);
    if (transf)
        entity_record_flag_component(sys->base.entity_record, entity, sys->base.component_flag);
    return transf;
}
void comp_system_transform_erase(struct comp_system_transform *sys, entity_t entity)
{
    if (comp_pool_transform_erase(&sys->pool, entity) == 0)
        entity_record_unflag_component(sys->base.entity_record, entity, sys->base.component_flag);
}
struct comp_transform *comp_system_transform_get(struct comp_system_transform *sys, entity_t entity)
{
    return comp_pool_transform_try_get(&sys->pool, entity);
}
void comp_system_transform_mark_family_desync(struct comp_system_transform *system, entity_t family_parent)
{
    entity_t               child;
    struct comp_transform *transf;

    if (entity_is_invalid(family_parent)) {
        LOG("COMP_TRANSF: (warn) Entity invalid");
        return;
    }

    child = system->sys_family->first_child_map[family_parent];
    transf = comp_pool_transform_try_get(&system->pool, family_parent);
    if (transf)
        transf->synced = 0;
     
    while (!entity_is_invalid(child)) {
        transf = comp_pool_transform_try_get(&system->pool, child);
        if (transf)
            transf->synced = 0;

        child = system->sys_family->sibling_map[child];
    }
}
static void comp_system_transform_sync_matrix(struct comp_system_transform *system, size_t data_index)
{
    struct comp_transform *transf, *parent_transf;

    transf = system->pool.data + data_index;

    if (transf->synced)
        return;

    transf->matrix = mat4_trs(transf->trans, transf->rot, transf->scale);

    parent_transf = comp_pool_transform_try_get(&system->pool, system->sys_family->parent_map[system->pool.dense[data_index]]);
    if (parent_transf) {
        comp_system_transform_sync_matrix(system, (parent_transf - system->pool.data));
        transf->matrix = mat4_mult(parent_transf->matrix, transf->matrix);
    }

    transf->matrix_version++;
    transf->synced = 1;
}
void comp_system_transform_sync_matrices(struct comp_system_transform *system)
{
    int i;
    struct comp_transform *transf, *parent_transf;

    for (i = 0; i < system->pool.len; i++)
        comp_system_transform_sync_matrix(system, i);
}


/* VISUAL */
void comp_visual_set_default(struct comp_visual *visual)
{
    visual->vertecies       = NULL;
    visual->texture         = NULL;
    visual->color           = VEC3_ZERO;
    visual->draw_pass       = 0;
}
struct comp_system_visual *comp_system_visual_create(struct comp_system base, struct comp_system_transform *sys_transf)
{
    struct comp_system_visual *sys = malloc(sizeof(struct comp_system_visual));
    if (!sys)
        return NULL;
    
    sys->base = base;
    sys->pool_transf = &sys_transf->pool;
    comp_pool_visual_init(&sys->pool_ortho, COMP_POOL_INITIAL_ALLOC/2);
    comp_pool_visual_init(&sys->pool_persp, COMP_POOL_INITIAL_ALLOC/2);
    return sys;
}
struct comp_visual *comp_system_visual_emplace(struct comp_system_visual *sys, entity_t entity, enum projection_type proj)
{
    struct comp_visual *vis = comp_pool_visual_emplace((proj == PROJ_ORTHO ? &sys->pool_ortho : &sys->pool_persp), entity);
    if (vis)
        entity_record_flag_component(sys->base.entity_record, entity, sys->base.component_flag);
    return vis;
}
void comp_system_visual_erase(struct comp_system_visual *sys, entity_t entity)
{
    char is_ortho = sys->pool_ortho.sparse[entity] != -1;
    if (comp_pool_visual_erase(is_ortho ? &sys->pool_ortho : &sys->pool_persp, entity) == 0)
        entity_record_unflag_component(sys->base.entity_record, entity, sys->base.component_flag);
}
struct comp_visual *comp_system_visual_get(struct comp_system_visual *sys, entity_t entity)
{
    char is_ortho = sys->pool_ortho.sparse[entity] != -1;
    return comp_pool_visual_try_get(is_ortho ? &sys->pool_ortho : &sys->pool_persp, entity);
}
static void comp_visual_transform_pool_draw(struct comp_pool_visual *pool, struct comp_pool_transform *pool_transf, const mat4 *projection)
{
    struct comp_visual    *visual;
    struct comp_transform *transf;
    short                  pass = 0,
                           total_pass = 1;
    int i;

    for(; pass < total_pass; pass++) {
        for (i = 0; i < pool->len; i++) {
            visual = pool->data + i;
            transf = comp_pool_transform_try_get(pool_transf, pool->dense[i]);

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
void comp_system_visual_draw(struct comp_system_visual *sys, const mat4 *persp, const mat4 *ortho)
{
    comp_visual_transform_pool_draw(&sys->pool_ortho, sys->pool_transf, ortho);
    comp_visual_transform_pool_draw(&sys->pool_persp, sys->pool_transf, persp);
}

/* HITRECT */
void comp_hitrect_set_default(struct comp_hitrect *hitrect)
{
    hitrect->cached_matrix_inv  = MAT4_IDENTITY;
    hitrect->cached_version     = 0;

    hitrect->type               = RECT_CAMSPACE;
    hitrect->rect               = RECT2D_ZERO;
    hitrect->hitstate           = 0;
    hitrect->hitmask            = 1;
}
struct comp_system_hitrect *comp_system_hitrect_create(struct comp_system base, struct comp_system_transform *sys_transf)
{
    struct comp_system_hitrect *sys = malloc(sizeof(struct comp_system_hitrect));
    if (!sys)
        return NULL;

    sys->base = base;
    sys->pool_transf = &sys_transf->pool;
    comp_pool_hitrect_init(&sys->pool, COMP_POOL_INITIAL_ALLOC);
    return sys;
}
struct comp_hitrect *comp_system_hitrect_emplace(struct comp_system_hitrect *sys, entity_t entity)
{
    struct comp_hitrect *hitrect = comp_pool_hitrect_emplace(&sys->pool, entity);
    if (hitrect)
        entity_record_flag_component(sys->base.entity_record, entity, sys->base.component_flag);
    return hitrect;
}
void comp_system_hitrect_erase(struct comp_system_hitrect *sys, entity_t entity)
{
    if (comp_pool_hitrect_erase(&sys->pool, entity) == 0)
        entity_record_unflag_component(sys->base.entity_record, entity, sys->base.component_flag);
}
struct comp_hitrect *comp_system_hitrect_get(struct comp_system_hitrect *sys, entity_t entity)
{
    return comp_pool_hitrect_try_get(&sys->pool, entity);
}
void comp_system_hitrect_update_hitstate(struct comp_system_hitrect *system, const vec2 *mouse_ortho, const vec3 *mouse_camspace_ray, char mask)
{
    struct comp_hitrect   *hitrect;
    struct comp_transform *transf;
    int i;

    for (i = 0; i < system->pool.len; i++) {
        hitrect = system->pool.data + i;
        transf  = system->pool_transf->data + system->pool_transf->sparse[system->pool.dense[i]];

        if (!transf | !(hitrect->hitmask & mask))
            continue;

        if (hitrect->cached_version != transf->matrix_version) {
            hitrect->cached_version = transf->matrix_version;
            hitrect->cached_matrix_inv = mat4_invert(transf->matrix);
        }

        if (hitrect->type == RECT_CAMSPACE)
            hitrect->hitstate = origin_ray_intersects_rect(&hitrect->rect, &hitrect->cached_matrix_inv, *mouse_camspace_ray);
        else 
            hitrect->hitstate = point_lands_on_rect(&hitrect->rect, &hitrect->cached_matrix_inv, vec3_create(mouse_ortho->x, mouse_ortho->y, 0));
    }
}
