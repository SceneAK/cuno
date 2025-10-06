#include <stdlib.h>
#include <string.h>
#include "system/graphic.h"
#include "system/log.h"
#include "gmath.h"
#include "system/time.h"
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
#define DEFINE_POOL_BASED_EMPLACE_ERASE_GET(name) \
    struct comp_##name *comp_system_##name##_emplace(struct comp_system_##name *sys, entity_t entity) \
    { \
        struct comp_##name *comp = comp_pool_##name##_emplace(&sys->pool, entity); \
        if (comp) \
            entity_record_flag_component(sys->base.entity_record, entity, sys->base.component_flag); \
        return comp; \
    } \
    void comp_system_##name##_erase(struct comp_system_##name *sys, entity_t entity) \
    { \
        if (comp_pool_##name##_erase(&sys->pool, entity) == 0) \
            entity_record_unflag_component(sys->base.entity_record, entity, sys->base.component_flag); \
    } \
    struct comp_##name *comp_system_##name##_get(struct comp_system_##name *sys, entity_t entity) \
    { \
        return comp_pool_##name##_try_get(&sys->pool, entity); \
    }

struct comp_system_family {
    struct comp_system              base;
    entity_t                        parent_map[ENTITY_MAX];
    entity_t                        first_child_map[ENTITY_MAX];
    entity_t                        sibling_map[ENTITY_MAX];
};
DEFINE_COMPONENT_POOL(static, struct comp_transform, comp_pool_transform)
struct comp_system_transform {
    struct comp_system              base;
    struct comp_pool_transform      pool;

    struct comp_system_family      *sys_family;
};
DEFINE_COMPONENT_POOL(static, struct comp_visual, comp_pool_visual)
struct comp_system_visual {
    struct comp_system              base;
    struct comp_pool_visual         pool_ortho;
    struct comp_pool_visual         pool_persp;

    struct comp_system_transform   *sys_transf;
};
DEFINE_COMPONENT_POOL(static, struct comp_hitrect, comp_pool_hitrect)
struct comp_system_hitrect {
    struct comp_system              base;
    struct comp_pool_hitrect        pool;

    struct comp_system_transform   *sys_transf;
};
DEFINE_COMPONENT_POOL(static, struct comp_interpolator, comp_pool_interpolator)
struct comp_system_interpolator {
    struct comp_system              base;
    struct comp_pool_interpolator   pool;

    struct comp_system_transform   *sys_transf;
};

/* FAMILY */
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


/* TRANSFORM */
void comp_transform_set_default(struct comp_transform *transf)
{
    transf->data.trans      = VEC3_ZERO;
    transf->data.rot        = VEC3_ZERO;
    transf->data.scale      = VEC3_ONE;

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
DEFINE_POOL_BASED_EMPLACE_ERASE_GET(transform);
void comp_system_transform_desync(struct comp_system_transform *sys, entity_t entity)
{
    entity_t               child;
    struct comp_transform *transf;

    if (entity_is_invalid(entity)) {
        LOG("COMP_TRANSF: (warn) Entity invalid. Cannot update changes.");
        return;
    }

    transf = comp_pool_transform_try_get(&sys->pool, entity);
    if (!transf) {
        LOG("COMP_TRANSF: (warn) Entity does not have transform. Cannot update changes.");
        return;
    }

    transf->synced = 0;

    child = sys->sys_family->first_child_map[entity];
    if (entity_is_invalid(entity))
        return;
     
    while (!entity_is_invalid(child)) {
        transf = comp_pool_transform_try_get(&sys->pool, child);
        if (transf)
            transf->synced = 0;

        child = sys->sys_family->sibling_map[child];
    }
}
static void comp_system_transform_sync_matrix(struct comp_system_transform *sys, size_t data_index)
{
    struct comp_transform *transf, *parent_transf;

    transf = sys->pool.data + data_index;

    if (transf->synced)
        return;

    transf->matrix = mat4_trs(transf->data);

    parent_transf = comp_pool_transform_try_get(&sys->pool, sys->sys_family->parent_map[sys->pool.dense[data_index]]);
    if (parent_transf) {
        comp_system_transform_sync_matrix(sys, (parent_transf - sys->pool.data));
        transf->matrix = mat4_mult(parent_transf->matrix, transf->matrix);
    }

    transf->matrix_version++;
    transf->synced = 1;
}
void comp_system_transform_sync_matrices(struct comp_system_transform *sys)
{
    int i;
    struct comp_transform *transf, *parent_transf;

    for (i = 0; i < sys->pool.len; i++)
        comp_system_transform_sync_matrix(sys, i);
}
struct transform comp_system_transform_get_world(struct comp_system_transform *sys, entity_t entity)
{
    struct comp_transform  *transf = comp_system_transform_get(sys, entity);
    if (!transf) {
        LOG("COMP_TRANSF: (warn) Entity does not have transform. Cannot get world transform.");
        return TRANSFORM_ZERO;
    }
    comp_system_transform_sync_matrix(sys, sys->pool.sparse[entity]);
    return transform_from_mat4(transf->matrix);
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
    sys->sys_transf = sys_transf;
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
    comp_visual_transform_pool_draw(&sys->pool_ortho, &sys->sys_transf->pool, ortho);
    comp_visual_transform_pool_draw(&sys->pool_persp, &sys->sys_transf->pool, persp);
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
    sys->sys_transf = sys_transf;
    comp_pool_hitrect_init(&sys->pool, COMP_POOL_INITIAL_ALLOC);
    return sys;
}
DEFINE_POOL_BASED_EMPLACE_ERASE_GET(hitrect)
void comp_system_hitrect_update_hitstate(struct comp_system_hitrect *system, const vec2 *mouse_ortho, const vec3 *mouse_camspace_ray, char mask)
{
    struct comp_hitrect   *hitrect;
    struct comp_transform *transf;
    int i;

    for (i = 0; i < system->pool.len; i++) {
        hitrect = system->pool.data + i;
        transf  = comp_system_transform_get(system->sys_transf, system->pool.dense[i]);

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

/* INTERPOLATOR */
void comp_interpolator_set_default(struct comp_interpolator *interpolator)
{
    memset(interpolator, 0, sizeof(struct comp_interpolator));
}
struct comp_system_interpolator *comp_system_interpolator_create(struct comp_system base, struct comp_system_transform *sys_transf)
{
    struct comp_system_interpolator *sys = malloc(sizeof(struct comp_system_interpolator));
    if (!sys)
        return NULL;

    sys->base = base;
    sys->sys_transf = sys_transf;
    comp_pool_interpolator_init(&sys->pool, COMP_POOL_INITIAL_ALLOC);
    return sys;
}
DEFINE_POOL_BASED_EMPLACE_ERASE_GET(interpolator)
void comp_system_interpolator_to_target(struct comp_system_interpolator *sys, entity_t entity, struct transform target)
{
    struct comp_transform      *transf = comp_system_transform_get(sys->sys_transf, entity);
    struct comp_interpolator   *interp = comp_pool_interpolator_try_get(&sys->pool, entity);
    if (!interp) {
        LOG("COMP_INTERP: (warn) interpolator for entity %d doesn't exist. Can't switch target.");
    }
    interp->target_delta    = transform_delta(transf->data, target);
    interp->start_time      = get_monotonic_time();
    interp->start_transform = transf->data;
}


static float ease_in(float t)
{
    return t*t;
}
static float ease_out(float t)
{
    return 1 - ease_in(1-t);
}
void comp_system_interpolator_update(struct comp_system_interpolator *sys)
{
    int i;
    entity_t                    entity;
    struct comp_interpolator   *interp;
    struct comp_transform      *transf;
    float                       elapsed,
                                duration,
                                factor;
    vec3                        factor_vec;
    double                      now = get_monotonic_time();

    for (i = 0; i < sys->pool.len; i++) {
        interp = sys->pool.data + i;
        if (interp->start_time == 0)
            continue;

        elapsed = now - interp->start_time;
        duration = interp->ease_in + interp->linear2/2 + interp->ease_out;

        entity = sys->pool.dense[i];
        transf = comp_pool_transform_try_get(&sys->sys_transf->pool, entity);
        if (!transf) {
            LOG("COMP_INTERP: (warn) transform for entity %d doesn't exist. Can't lerp.");
            continue;
        }

        if (interp->ease_in >= elapsed)
            factor = ease_in(elapsed/interp->ease_in) * interp->ease_in / duration;

        else if (interp->linear2/2 >= (elapsed - interp->ease_in))
            factor = elapsed / duration;

        else if (interp->ease_out >= (elapsed -= interp->ease_in + interp->linear2/2))
            factor = (interp->ease_in + interp->linear2/2 + ease_out( elapsed / interp->ease_out) * interp->ease_out) / duration;

        else {
            interp->start_time = 0;
            factor = 1;
        }


        factor_vec = vec3_all(factor);
        transf->data.trans  = vec3_sum(interp->start_transform.trans, vec3_mult(interp->target_delta.trans, factor_vec));
        transf->data.rot    = vec3_sum(interp->start_transform.rot, vec3_mult(interp->target_delta.rot, factor_vec));
        transf->data.scale  = vec3_sum(interp->start_transform.scale, vec3_mult(interp->target_delta.scale, factor_vec));
        comp_system_transform_desync(sys->sys_transf, entity);
    }
}
