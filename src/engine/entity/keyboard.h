#ifndef ENTITY_KEYBOARD_H
#define ENTITY_KEYBOARD_H
#include "engine/entity/world.h"
#include "engine/entity/dot.h"
#include "engine/component.h"
#include "engine/text.h"

struct entity_keyboard_opt {
    const char                   *layout;
    float                         padding,
                                  keysize;
    void                        (*hit_handler)(entity_t, struct comp_hitrect *);

    struct baked_font            *font_spec;
    struct graphic_texture       *font_tex;
};

static char const KBLAYOUT_QWERTY[] = {
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '\n',
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '\n',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', 
    '\0'
};
static char const KBLAYOUT_NUMPAD[] = {
    '1', '2', '3', '\n',
    '4', '5', '6', '\n',
    '7', '8', '9', '\n',
    '0', 
    '\0'
};


static entity_t entity_keyboard_key_create(struct entity_world *ctx, const struct entity_keyboard_opt *opt, char chr)
{
    const float FONT_SIZE_APPROX = opt->font_spec->cps[chr].x1 - opt->font_spec->cps[chr].x0;
    const char STR[2]  = {chr, 0};

    entity_t text = entity_record_activate(&ctx->entrec);
    entity_t key  = entity_record_activate(&ctx->entrec);
    entity_t bg   = entity_record_activate(&ctx->entrec);
    struct comp_transform *transf;
    struct comp_visual *vis;
    struct comp_hitrect *hitrect;

    transf  = comp_system_transform_emplace(ctx->sys_transf, text);
    vis     = comp_system_visual_emplace(ctx->sys_vis, text, PROJ_ORTHO);
    comp_transform_set_default(transf);
    comp_visual_set_default(vis);
    transf->data.trans   = vec3_create(0.5, 0.5, 0);
    transf->data.scale   = vec3_all(1/FONT_SIZE_APPROX);
    vis->vertecies       = graphic_vertecies_create_text(*opt->font_spec, 1, STR);
    vis->texture         = opt->font_tex;
    vis->color           = VEC3_ONE;
    vis->draw_pass       = DRAW_PASS_TRANSPARENT;

    transf  = comp_system_transform_emplace(ctx->sys_transf, bg);
    vis     = comp_system_visual_emplace(ctx->sys_vis, bg, PROJ_ORTHO);
    hitrect = comp_system_hitrect_emplace(ctx->sys_hitrect, bg);
    comp_transform_set_default(transf);
    comp_visual_set_default(vis);
    comp_hitrect_set_default(hitrect);
    transf->data.trans.z = -.1;
    vis->vertecies       = DEFAULT_SQUARE_VERT_GET();
    vis->color           = VEC3_ZERO;
    vis->draw_pass       = DRAW_PASS_OPAQUE;
    hitrect->type        = HITRECT_ORTHOSPACE;
    hitrect->rect        = SQUARE_BOUNDS;
    hitrect->hitmask     = HITMASK_MOUSE_DOWN;
    hitrect->tag         = chr;
    hitrect->hit_handler = opt->hit_handler;

    transf  = comp_system_transform_emplace(ctx->sys_transf, key);
    comp_transform_set_default(transf);

    comp_system_family_adopt(ctx->sys_fam, key, text);
    comp_system_family_adopt(ctx->sys_fam, key, bg);

    return key;
}

static inline entity_t entity_keyboard_create(struct entity_world *ctx, const struct entity_keyboard_opt *opt)
{
    entity_t parent = entity_record_activate(&ctx->entrec);
    entity_t key;
    struct comp_transform *transf;

    int col = 0, row = 0;
    int lnlen = -1;
    const float OFFSET = opt->keysize + opt->padding;
    const char *head = opt->layout;

    comp_system_transform_emplace(ctx->sys_transf, parent);
    
    while (head[col]) {
        if (col == lnlen) {
            head += col+1;
            col = 0;
            row++;

            lnlen = -1;
        }
        if (lnlen == -1)
            lnlen = strcspn(head, "\n");

        key = entity_keyboard_key_create(ctx, opt, head[col]);
        transf = comp_system_transform_get(ctx->sys_transf, key);
        transf->data.trans = vec3_create( (-lnlen/2.0f + col)*OFFSET, -row*OFFSET, -2);
        transf->data.scale   = vec3_all(opt->keysize);

        comp_system_family_adopt(ctx->sys_fam, parent, key);
        entity_dot_attatch_new(ctx, key, VEC3_GREEN, PROJ_ORTHO, .1);

        col++;
    }
    entity_dot_attatch_new(ctx, parent, VEC3_RED, PROJ_ORTHO, .1);
    return parent;
}

#endif
