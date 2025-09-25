#ifndef COMPONENT_H
#define COMPONENT_H
#include "system/graphic/graphic.h"
#include "system/logging.h"
#include "game_logic.h"

typedef unsigned short entity_t;
#define ENTITY_MAX          8192
#define ENTITY_INVALID      ENTITY_MAX
#define COMP_INDEX_INVALID  (unsigned short)(-1)

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

#define DEFINE_COMPONENT_POOL(static_inline, type, name) \
    struct name { \
        unsigned short  sparse[ENTITY_MAX]; \
        entity_t        dense[ENTITY_MAX]; \
        size_t          len, \
                        allocated_len; \
        type           *data; \
    }; \
    static_inline int name##_init(struct name *pool, size_t initial_alloc_len) \
    { \
        return component_pool_init((struct component_pool *)pool, initial_alloc_len, sizeof(type)); \
    } \
    static_inline void name##_deinit(struct name *pool) \
    { \
        component_pool_deinit((struct component_pool *)pool); \
    } \
    static_inline type *name##_emplace(struct name *pool, entity_t entity) \
    { \
        return (type *)component_pool_emplace((struct component_pool *)pool, entity, sizeof(type)); \
    } \
    static_inline int name##_erase(struct name *pool, entity_t entity) \
    { \
        return component_pool_erase((struct component_pool *)pool, entity, sizeof(type)); \
    } \
    static_inline type *name##_try_get(struct name *pool, entity_t entity) \
    { \
        if (entity == ENTITY_INVALID || pool->sparse[entity] == COMP_INDEX_INVALID) \
            return NULL; \
        return pool->data + pool->sparse[entity];  \
    } \

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
#define HITMASK_MOUSE_DOWN  0b0001
#define HITMASK_MOUSE_UP    0b0010
#define HITMASK_MOUSE_HOVER 0b0100
#define HITMASK_MOUSE_HOLD  0b1000

void comp_transform_set_default(struct comp_transform *transf);
DEFINE_COMPONENT_POOL(static, struct comp_transform, comp_transform_pool)
void comp_transform_pool_sync_matrices(struct comp_transform_pool *pool, entity_t *parent_map);

void comp_visual_set_default(struct comp_visual *comp_visual);
DEFINE_COMPONENT_POOL(static, struct comp_visual, comp_visual_pool)
void comp_visual_transform_pool_draw(struct comp_visual_pool *vis_pool, struct comp_transform_pool *transf_pool, const mat4 *perspective);

void comp_hitrect_set_default(struct comp_hitrect *hitrect);
DEFINE_COMPONENT_POOL(static, struct comp_hitrect, comp_hitrect_pool)
void comp_hitrect_transform_pool_update_hitstate(struct comp_hitrect_pool *hitrect_pool, struct comp_transform_pool *transf_pool, 
                                                 const vec2 *mouse_ortho, const vec3 *mouse_camspace_ray, char mask);
DEFINE_COMPONENT_POOL(static, card_id_t, comp_card_pool);
void comp_card_pool_update(struct comp_card_pool *pool, struct game_state *state);


struct entity_system {
    char                        active[ENTITY_MAX];
    entity_t                    parent_map[ENTITY_MAX];
    entity_t                    first_child_map[ENTITY_MAX];
    entity_t                    sibling_map[ENTITY_MAX];
    unsigned int                signature[ENTITY_MAX];

    struct comp_transform_pool  transforms;
    struct comp_visual_pool     visuals_ortho, 
                                visuals_persp;
    struct comp_hitrect_pool    hitrects;
    struct comp_card_pool       cards;
};
static void entity_system_init(struct entity_system *system)
{
    int i;
    for (i = 0; i < ENTITY_MAX; i++) {
        system->active[i]           = 0;
        system->parent_map[i]       = ENTITY_INVALID;
        system->sibling_map[i]      = ENTITY_INVALID;
        system->first_child_map[i]  = ENTITY_INVALID;
    }

}
static entity_t entity_system_activate_new(struct entity_system *system)
{
    int i;
    for (i = 0; i < ENTITY_MAX; i++)
        if (!system->active[i]) {
            system->active[i] = 1;
            return i;
        }
    return ENTITY_INVALID;
}
static void entity_system_deactivate(struct entity_system *system, entity_t entity)
{
    system->active[entity] = 0;
}
enum entity_system_signature {
    SIGN_TRANSFORM,
    SIGN_VISUAL_ORTHO,
    SIGN_VISUAL_PERSP,
    SIGN_HITRECT,

    SIGN_CARD
};
static void entity_system_erase_via_signature(struct entity_system *system, entity_t entity, unsigned int es_signature) 
{
    if (es_signature & 1<<SIGN_TRANSFORM)
        comp_transform_pool_erase(&system->transforms, entity);
    if (es_signature & 1<<SIGN_VISUAL_ORTHO)
        comp_visual_pool_erase(&system->visuals_ortho, entity);
    if (es_signature & 1<<SIGN_VISUAL_PERSP)
        comp_visual_pool_erase(&system->visuals_persp, entity);
    if (es_signature & 1<<SIGN_HITRECT)
        comp_hitrect_pool_erase(&system->hitrects, entity);

    if (es_signature & 1<<SIGN_CARD)
        comp_card_pool_erase(&system->cards, entity);
}

#endif
