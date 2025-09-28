#ifndef COMPONENT_H
#define COMPONENT_H
#include "system/graphic/graphic.h"
#include "game_logic.h"

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


struct entity_system {
    char                            active[ENTITY_MAX];
    entity_t                        parent_map[ENTITY_MAX];
    entity_t                        first_child_map[ENTITY_MAX];
    entity_t                        sibling_map[ENTITY_MAX];
    unsigned int                    signature[ENTITY_MAX];

    struct comp_transform_system    transform_sys;
    struct comp_visual_system       visual_sys;
    struct comp_hitrect_system      hitrect_sys;
    struct card_id_pool             cards;
};
static void entity_system_init(struct entity_system *ent_system)
{
    const int COMP_INITIAL = 32;
    int i;
    for (i = 0; i < ENTITY_MAX; i++) {
        ent_system->active[i]           = 0;
        ent_system->parent_map[i]       = ENTITY_INVALID;
        ent_system->sibling_map[i]      = ENTITY_INVALID;
        ent_system->first_child_map[i]  = ENTITY_INVALID;
    }

    ent_system->transform_sys.parent_map      = ent_system->parent_map;
    ent_system->transform_sys.first_child_map = ent_system->first_child_map;
    ent_system->transform_sys.sibling_map     = ent_system->sibling_map;
    comp_transform_pool_init(&ent_system->transform_sys.pool, COMP_INITIAL);

    ent_system->visual_sys.transf_pool = &ent_system->transform_sys.pool;
    comp_visual_pool_init(&ent_system->visual_sys.pool_ortho, COMP_INITIAL);
    comp_visual_pool_init(&ent_system->visual_sys.pool_persp, COMP_INITIAL);
    
    ent_system->hitrect_sys.transf_pool = &ent_system->transform_sys.pool;
    comp_hitrect_pool_init(&ent_system->hitrect_sys.pool, COMP_INITIAL);

    card_id_pool_init(&ent_system->cards, COMP_INITIAL);
}
static entity_t entity_system_activate_new(struct entity_system *ent_system)
{
    int i;
    for (i = 0; i < ENTITY_MAX; i++)
        if (!ent_system->active[i]) {
            ent_system->active[i] = 1;
            return i;
        }
    return ENTITY_INVALID;
}
static void entity_system_deactivate(struct entity_system *ent_system, entity_t entity)
{
    ent_system->parent_map[entity]      = ENTITY_INVALID;
    ent_system->sibling_map[entity]     = ENTITY_INVALID;
    ent_system->first_child_map[entity] = ENTITY_INVALID;
    ent_system->active[entity]          = 0;
}
enum entity_system_signature {
    SIGN_TRANSFORM,
    SIGN_VISUAL_ORTHO,
    SIGN_VISUAL_PERSP,
    SIGN_HITRECT,

    SIGN_CARD
};
static void entity_system_erase_via_signature(struct entity_system *ent_system, entity_t entity, unsigned int es_signature) 
{
    if (es_signature & 1<<SIGN_TRANSFORM)
        comp_transform_pool_erase(&ent_system->transform_sys.pool, entity);
    if (es_signature & 1<<SIGN_VISUAL_ORTHO)
        comp_visual_pool_erase(&ent_system->visual_sys.pool_ortho, entity);
    if (es_signature & 1<<SIGN_VISUAL_PERSP)
        comp_visual_pool_erase(&ent_system->visual_sys.pool_persp, entity);
    if (es_signature & 1<<SIGN_HITRECT)
        comp_hitrect_pool_erase(&ent_system->hitrect_sys.pool, entity);

    if (es_signature & 1<<SIGN_CARD)
        card_id_pool_erase(&ent_system->cards, entity);
}
static size_t entity_system_count_children(struct entity_system *ent_system, entity_t entity)
{
    size_t      count = 0;
    entity_t    current;

    if (entity_is_invalid(entity))
        return SIZE_MAX;

    current = ent_system->first_child_map[entity];
    while (!entity_is_invalid(current)) {
        count++;
        current = ent_system->sibling_map[current];
    }

    return count;
}
static entity_t entity_system_find_previous_sibling(struct entity_system *ent_system, entity_t entity)
{
    entity_t current,
             last = ENTITY_INVALID;

    if (entity_is_invalid(ent_system->parent_map[entity]))
        return -1;
    current = ent_system->first_child_map[ ent_system->parent_map[entity] ];

    while (!entity_is_invalid(current)) {
        if (current == entity)
            break;

        last = current;
        current = ent_system->sibling_map[current];
    }
    return last;
}
static entity_t entity_system_find_last_child(struct entity_system *ent_system, entity_t parent)
{
    entity_t current;

    if (entity_is_invalid(parent))
        return ENTITY_INVALID;

    current = ent_system->first_child_map[parent];
    if (entity_is_invalid(current))
        return ENTITY_INVALID;

    while (!entity_is_invalid(ent_system->sibling_map[current])) {
        current = ent_system->sibling_map[current];
    }
    return current;
}
static void entity_system_parent(struct entity_system *ent_system, entity_t parent, entity_t entity)
{
    ent_system->parent_map[entity] = parent;
    if (entity_is_invalid(ent_system->first_child_map[parent]))
        ent_system->first_child_map[parent] = entity;
    else
        ent_system->sibling_map[entity_system_find_last_child(ent_system, parent)] = entity;
}
static int entity_system_unparent(struct entity_system *ent_system, entity_t entity)
{
    entity_t prev; 

    if (entity_is_invalid(ent_system->parent_map[entity]))
        return -1;

    prev = entity_system_find_previous_sibling(ent_system, entity);
    if (entity_is_invalid(prev))
        ent_system->first_child_map[ ent_system->parent_map[entity] ] = ent_system->sibling_map[entity];
    else 
        ent_system->sibling_map[prev] = ent_system->sibling_map[entity];

    ent_system->parent_map[entity]  = ENTITY_INVALID;
    ent_system->sibling_map[entity] = ENTITY_INVALID;

    return 0;
}

#endif
