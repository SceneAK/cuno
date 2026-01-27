#ifndef ENTITY_SHARE_H
#define ENTITY_SHARE_H
#include <stddef.h>
#include "engine/system/graphic.h"
#include "engine/component.h"
#include "engine/utils.h"

#define COMPFLAG_SYS_FAMILY     1<<0
#define COMPFLAG_SYS_TRANSFORM  1<<1
#define COMPFLAG_SYS_VISUAL     1<<2
#define COMPFLAG_SYS_HITRECT    1<<3
#define COMPFLAG_SYS_INTERP     1<<4

struct entity_world {
    struct entity_record             records;
    struct comp_system_family       *sys_fam;
    struct comp_system_transform    *sys_transf;
    struct comp_system_visual       *sys_vis;
    struct comp_system_hitrect      *sys_hitrect;
    struct comp_system_interpolator *sys_interp;
};

static struct graphic_vertecies *_default_square = NULL;
#define DEFAULT_SQUARE_VERT_GET() (_default_square ? _default_square : \
        (_default_square = graphic_vertecies_create(SQUARE_VERTS, VERT_COUNT(SQUARE_VERTS))));


static void entity_world_init(struct entity_world *world)
{
    struct comp_system base = { &world->records };

    entity_record_init(&world->records);

    base.component_flag = COMPFLAG_SYS_FAMILY;
    world->sys_fam     = comp_system_family_create(base);

    base.component_flag = COMPFLAG_SYS_TRANSFORM;
    world->sys_transf  = comp_system_transform_create(base, world->sys_fam);

    base.component_flag = COMPFLAG_SYS_VISUAL;
    world->sys_vis     = comp_system_visual_create(base, world->sys_transf);

    base.component_flag = COMPFLAG_SYS_HITRECT;
    world->sys_hitrect = comp_system_hitrect_create(base, world->sys_transf);

    base.component_flag = COMPFLAG_SYS_INTERP;
    world->sys_interp = comp_system_interpolator_create(base, world->sys_transf);
}

void entity_world_cleanup(struct entity_world *world, entity_t entity)
{
    component_flag_t flags = world->records.component_flags[entity];
    if (flags & COMPFLAG_SYS_FAMILY)
        comp_system_family_disown(world->sys_fam, entity);
    if (flags & COMPFLAG_SYS_TRANSFORM)
        comp_system_transform_erase(world->sys_transf, entity);
    if (flags & COMPFLAG_SYS_VISUAL)
        comp_system_visual_erase(world->sys_vis, entity);
    if (flags & COMPFLAG_SYS_HITRECT)
        comp_system_hitrect_erase(world->sys_hitrect, entity);
    if (flags & COMPFLAG_SYS_INTERP)
        comp_system_interpolator_erase(world->sys_interp, entity);
}

#endif
