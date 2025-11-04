#ifndef COMPONENT_H
#define COMPONENT_H
#include <string.h>
#include "system/log.h"
#include "system/graphic.h"
#include "gmath.h"
#include "utils.h"

#define ENTITY_MAX          8192
#define ENTITY_INVALID      ENTITY_MAX

#define entity_is_invalid(entity) (entity == ENTITY_INVALID || entity > ENTITY_MAX)

#define entity_record_init(entrec_ptr) memset(entrec_ptr, 0, sizeof(struct entity_record))
#define entity_record_flag_component(entrec_ptr, entity, flag) (entrec_ptr)->component_flags[entity] |= flag
#define entity_record_unflag_component(entrec, entity, flag) (entrec)->component_flags[entity] &= ~(flag)

typedef unsigned short entity_t;
typedef unsigned int component_flag_t;

struct entity_record {
    char                active[ENTITY_MAX];
    component_flag_t    component_flags[ENTITY_MAX];
};

static entity_t entity_record_activate(struct entity_record *entrec)
{
    int i;
    for (i = 0; i < ENTITY_MAX; i++)
        if (!entrec->active[i]) {
            entrec->active[i] = 1;
            return i;
        }
    LOG("ENTREC: (warn) recorded entity hit ENTITY_MAX");
    return ENTITY_INVALID;
}
static int entity_record_deactivate(struct entity_record *entrec, entity_t entity)
{
    if (entrec->component_flags[entity]) {
        LOGF("ENTREC: (warn) entity %d can't deactivate due to its non-zero component flags (%d)", entity, entrec->component_flags[entity]);
        return -1;
    }
    return entrec->active[entity] = 0;
}
DEFINE_ARRAY_LIST_WRAPPER(static, entity_t, entity_list)



#define HITMASK_MOUSE_DOWN  1<<0
#define HITMASK_MOUSE_UP    1<<1
#define HITMASK_MOUSE_HOVER 1<<2
#define HITMASK_MOUSE_HOLD  1<<3

struct comp_system {
    struct entity_record   *entity_record;
    component_flag_t        component_flag;
};
struct comp_system_family_view {
    const entity_t   *parent_map;
    const entity_t   *first_child_map;
    const entity_t   *sibling_map;
};
struct comp_transform {
    struct transform            data;
    char                        synced;

    unsigned short              matrix_version;
    mat4                        matrix;
};
enum projection_type { 
    PROJ_ORTHO, 
    PROJ_PERSP,
};
enum draw_pass_type {
    DRAW_PASS_NONE = -1,
    DRAW_PASS_OPAQUE,
    DRAW_PASS_TRANSPARENT,
};
struct comp_visual {
    struct graphic_vertecies   *vertecies;
    struct graphic_texture     *texture;
    vec3                        color;
    enum draw_pass_type         draw_pass;
};
enum hitrect_type {
    HITRECT_CAMSPACE, 
    HITRECT_ORTHOSPACE
};
struct comp_hitrect {
    mat4                        cached_matrix_inv;
    unsigned short              cached_version;

    rect2D                      rect;
    enum hitrect_type           type;
    unsigned char               state;
    unsigned char               hitmask;
    unsigned char               active;
    void                        (*hit_handler)(entity_t entity, struct comp_hitrect *hitrect);
};
struct interpolation_opt {
    double                      ease_in,
                                linear2,
                                ease_out;
};
struct comp_interpolator {
    struct transform            target_delta;
    struct transform            start_transform;
    double                      start_time;
    struct interpolation_opt    opt;
};
struct comp_system_family;
struct comp_system_transform;
struct comp_system_visual;
struct comp_system_hitrect;
struct comp_system_interpolator;

struct comp_system_family *comp_system_family_create(struct comp_system base);
void comp_system_family_adopt(struct comp_system_family *sys, entity_t parent, entity_t entity);
int comp_system_family_disown(struct comp_system_family *sys, entity_t entity);
size_t comp_system_family_count_children(struct comp_system_family *sys, entity_t entity);
entity_t comp_system_family_find_previous_sibling(struct comp_system_family *sys, entity_t entity);
entity_t comp_system_family_find_last_child(struct comp_system_family *sys, entity_t parent);
struct comp_system_family_view comp_system_family_view(struct comp_system_family *sys);

void comp_transform_set_default(struct comp_transform *transf);
struct comp_system_transform *comp_system_transform_create(struct comp_system base, struct comp_system_family *sys_fam);
struct comp_transform *comp_system_transform_emplace(struct comp_system_transform *sys, entity_t entity);
void comp_system_transform_erase(struct comp_system_transform *sys, entity_t entity);
struct comp_transform *comp_system_transform_get(struct comp_system_transform *sys, entity_t entity);
void comp_system_transform_desync(struct comp_system_transform *system, entity_t entity);
void comp_system_transform_sync_matrices(struct comp_system_transform *sys);
struct transform comp_system_transform_get_world(struct comp_system_transform *sys, entity_t entity);

void comp_visual_set_default(struct comp_visual *visual);
struct comp_system_visual *comp_system_visual_create(struct comp_system base, struct comp_system_transform *sys_transf);
struct comp_visual *comp_system_visual_emplace(struct comp_system_visual *sys, entity_t entity, enum projection_type proj);
void comp_system_visual_erase(struct comp_system_visual *sys, entity_t entity);
struct comp_visual *comp_system_visual_get(struct comp_system_visual *sys, entity_t entity);
void comp_system_visual_draw(struct comp_system_visual *sys, const mat4 *persp, const mat4 *ortho);

void comp_hitrect_set_default(struct comp_hitrect *hitrect);
struct comp_system_hitrect *comp_system_hitrect_create(struct comp_system base, struct comp_system_transform *sys_transf);
struct comp_hitrect *comp_system_hitrect_emplace(struct comp_system_hitrect *sys, entity_t entity);
void comp_system_hitrect_erase(struct comp_system_hitrect *sys, entity_t entity);
struct comp_hitrect *comp_system_hitrect_get(struct comp_system_hitrect *sys, entity_t entity);
void comp_system_hitrect_clear_states(struct comp_system_hitrect *sys);
int comp_system_hitrect_check_and_clear_state(struct comp_system_hitrect *sys, entity_t entity);
void comp_system_hitrect_update(struct comp_system_hitrect *system, const vec2 *mouse_ortho, const vec3 *mouse_camspace_ray, char mask);

void comp_interpolator_set_default(struct comp_interpolator *interpolator);
void comp_system_interpolator_finish(struct comp_system_interpolator *sys, entity_t entity);
void comp_system_interpolator_start(struct comp_system_interpolator *sys, entity_t entity, struct transform target);
void comp_system_interpolator_change(struct comp_system_interpolator *sys, entity_t entity, struct transform target);
struct comp_system_interpolator *comp_system_interpolator_create(struct comp_system base, struct comp_system_transform *sys_transf);
struct comp_interpolator *comp_system_interpolator_emplace(struct comp_system_interpolator *sys, entity_t entity);
void comp_system_interpolator_erase(struct comp_system_interpolator *sys, entity_t entity);
struct comp_interpolator *comp_system_interpolator_get(struct comp_system_interpolator *sys, entity_t entity);
void comp_system_interpolator_update(struct comp_system_interpolator *sys);
#endif
