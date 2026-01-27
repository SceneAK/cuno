#ifndef ENTITY_CONTROL_H
#define ENTITY_CONTROL_H
#include "engine/entity/world.h"
#include "engine/component.h"
#include "engine/math.h"
#include "engine/text.h"

struct entity_text_opt {
    struct graphic_texture   *font_tex;
    struct baked_font        *font_spec;
    float lnheight;
    char normalize;
};

struct entity_button_args {
    struct entity_text_opt *txtopt;
    const char      *label;
    vec3             text_color;
    float            text_scale;

    unsigned char    tag;
    vec3             pos;
    vec3             scale;
    vec3             color;
    void (*hit_handler)(entity_t, struct comp_hitrect *);
};

static void entity_text_change(struct entity_world *world, const struct entity_text_opt *opt, entity_t txt, const char *str)
{
    struct comp_visual *visual;

    if (!str)
        return;

    visual = comp_system_visual_get(world->sys_vis, txt);
    if (visual->vertecies)
        graphic_vertecies_destroy(visual->vertecies);
    visual->vertecies = graphic_vertecies_create_text(*opt->font_spec, opt->lnheight, str, opt->normalize);
}

static entity_t entity_text_create(struct entity_world *world, const struct entity_text_opt *opt, vec3 pos, float scale, vec3 color)
{
    struct comp_transform *transf;
    struct comp_visual *visual;
    entity_t txt;

    txt                 = entity_record_activate(&world->records);
    transf              = comp_system_transform_emplace(world->sys_transf, txt);
    visual              = comp_system_visual_emplace(world->sys_vis, txt, PROJ_ORTHO);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    transf->data.trans  = pos;
    transf->data.scale  = vec3_all(scale);
    visual->texture     = opt->font_tex;
    visual->color       = color;
    visual->draw_pass   = DRAW_PASS_TRANSPARENT;

    return txt;
}

static entity_t entity_button_create(struct entity_world *world, const struct entity_button_args *args)
{
    entity_t parent;
    entity_t text;
    entity_t btn;
    struct comp_transform   *transf;
    struct comp_visual      *vis;
    struct comp_hitrect     *hitrect;
    
    parent = entity_record_activate(&world->records);

    text = entity_text_create(world, args->txtopt, VEC3_ONE, args->text_scale, args->text_color);
    entity_text_change(world, args->txtopt, text, args->label);

    btn   = entity_record_activate(&world->records);
    transf  = comp_system_transform_emplace(world->sys_transf, btn);
    vis     = comp_system_visual_emplace(world->sys_vis, btn, PROJ_ORTHO);
    hitrect = comp_system_hitrect_emplace(world->sys_hitrect, btn);
    comp_transform_set_default(transf);
    comp_visual_set_default(vis);
    comp_hitrect_set_default(hitrect);
    transf->data.trans.z = -.1;
    transf->data.scale   = args->scale;
    vis->vertecies       = DEFAULT_SQUARE_VERT_GET();
    vis->color           = args->color;
    vis->draw_pass       = DRAW_PASS_OPAQUE;
    hitrect->type        = HITRECT_ORTHOSPACE;
    hitrect->rect        = SQUARE_BOUNDS;
    hitrect->hitmask     = HITMASK_MOUSE_DOWN;
    hitrect->tag         = args->tag;
    hitrect->hit_handler = args->hit_handler;

    transf = comp_system_transform_emplace(world->sys_transf, parent);
    comp_transform_set_default(transf);
    transf->data.trans   = args->pos;

    comp_system_family_adopt(world->sys_fam, parent, text);
    comp_system_family_adopt(world->sys_fam, parent, btn);

    return parent;
}
#endif
