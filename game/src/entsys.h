#ifndef ENTITY_H
#define ENTITY_H
#include "component.h"

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
    ent_system->active[entity] = 0;
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

#define SIGNATURE_PARENT        1<<0
#define SIGNATURE_TRANSFORM     1<<1
#define SIGNATURE_VISUAL_ORTHO  1<<2
#define SIGNATURE_VISUAL_PERSP  1<<3
#define SIGNATURE_HITRECT       1<<4
#define SIGNATURE_CARD          1<<5

static void entity_system_cleanup_via_signature(struct entity_system *ent_system, entity_t entity) 
{
    if (ent_system->signature[entity] & SIGNATURE_PARENT)
        entity_system_unparent(ent_system, entity);
    if (ent_system->signature[entity] & SIGNATURE_TRANSFORM)
        comp_transform_pool_erase(&ent_system->transform_sys.pool, entity);
    if (ent_system->signature[entity] & SIGNATURE_VISUAL_ORTHO)
        comp_visual_pool_erase(&ent_system->visual_sys.pool_ortho, entity);
    if (ent_system->signature[entity] & SIGNATURE_VISUAL_PERSP)
        comp_visual_pool_erase(&ent_system->visual_sys.pool_persp, entity);
    if (ent_system->signature[entity] & SIGNATURE_HITRECT)
        comp_hitrect_pool_erase(&ent_system->hitrect_sys.pool, entity);

    if (ent_system->signature[entity] & SIGNATURE_CARD)
        card_id_pool_erase(&ent_system->cards, entity);
}

#endif
