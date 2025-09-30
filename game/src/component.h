#ifndef COMPONENT_H
#define COMPONENT_H
#include "system/graphic.h"
#include "logic.h"

typedef unsigned short entity_t;
#define ENTITY_MAX          8192
#define ENTITY_INVALID      ENTITY_MAX
#define COMP_INDEX_INVALID  (unsigned short)(-1)

static int entity_is_invalid(entity_t entity) 
{
    return entity == ENTITY_INVALID || entity > ENTITY_MAX;
}


struct component_pool {
    unsigned short  sparse[ENTITY_MAX];
    entity_t        dense[ENTITY_MAX];
    size_t          len,
                    allocated_len;
    void           *data;
};
int component_pool_init(struct component_pool *pool, size_t initial_alloc_len, size_t elem_size);
void component_pool_deinit(struct component_pool *pool);
void *component_pool_emplace(struct component_pool *pool, entity_t entity, size_t elem_size);
int component_pool_erase(struct component_pool *pool, entity_t entity, size_t elem_size);

#define DEFINE_COMPONENT_POOL_STRUCT(type, name) \
    struct name { \
        unsigned short  sparse[ENTITY_MAX]; \
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
        if (entity == ENTITY_INVALID || pool->sparse[entity] == COMP_INDEX_INVALID) \
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

struct comp_transform_props {
    vec3                    trans, rot, scale;
};
struct comp_transform {
    vec3                    trans, rot, scale;
    char                    synced;

    unsigned short          matrix_version;
    mat4                    matrix;
};
struct comp_visual {
    struct graphic_vertecies   *vertecies;
    struct graphic_texture     *texture;
    vec3                        color;
    short                       draw_pass;
};
struct comp_hitrect {
    mat4                    cached_matrix_inv;
    unsigned short          cached_version;

    enum {
        RECT_CAMSPACE, 
        RECT_ORTHOSPACE
    }                       rect_type;
    rect2D                  rect;
    unsigned char           hitstate,
                            hitmask;
};


void comp_transform_set_default(struct comp_transform *transf);
DEFINE_COMPONENT_POOL(static, struct comp_transform, comp_transform_pool)
struct comp_transform_system {
    struct comp_transform_pool pool;
    entity_t                   *parent_map;
    entity_t                   *first_child_map;
    entity_t                   *sibling_map;
};
void comp_transform_system_init(struct comp_transform_system *system, entity_t *parent_map, entity_t *first_child_map, entity_t *sibling_map, size_t initial_alloc_len);
void comp_transform_system_mark_family_desync(struct comp_transform_system *system, entity_t entity);
void comp_transform_system_sync_matrices(struct comp_transform_system *system);


void comp_visual_set_default(struct comp_visual *visual);
DEFINE_COMPONENT_POOL(static, struct comp_visual, comp_visual_pool)
struct comp_visual_system {
    struct comp_visual_pool     pool_ortho;
    struct comp_visual_pool     pool_persp;
    struct comp_transform_pool *transf_pool;
};
void comp_visual_system_draw(struct comp_visual_system *vis_sys, const mat4 *persp, const mat4 *ortho);


#define HITMASK_MOUSE_DOWN  0b0001
#define HITMASK_MOUSE_UP    0b0010
#define HITMASK_MOUSE_HOVER 0b0100
#define HITMASK_MOUSE_HOLD  0b1000
void comp_hitrect_set_default(struct comp_hitrect *hitrect);
DEFINE_COMPONENT_POOL(static, struct comp_hitrect, comp_hitrect_pool)
struct comp_hitrect_system {
    struct comp_hitrect_pool    pool;
    struct comp_transform_pool *transf_pool;
};
void comp_hitrect_system_update_hitstate(struct comp_hitrect_system *system, const vec2 *mouse_ortho, const vec3 *mouse_camspace_ray, char mask);


DEFINE_COMPONENT_POOL(static, card_id_t, card_id_pool);

#endif
