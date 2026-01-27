#ifndef ENTITY_KEYBOARD_H
#define ENTITY_KEYBOARD_H
#include "engine/entity/control.h"
#include "engine/entity/world.h"
#include "engine/component.h"

struct entity_keyboard_args {
    const char                   *layout;
    float                         padding,
                                  keysize;
    struct entity_text_opt       *txtopt;
    void                        (*hit_handler)(entity_t, struct comp_hitrect *);
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

static inline entity_t entity_keyboard_create(struct entity_world *ctx, const struct entity_keyboard_args *args)
{
    char label[2]  = {0};
    entity_t parent = entity_record_activate(&ctx->records);
    entity_t key;
    struct comp_transform *transf;
    struct entity_button_args btnargs = {
        .txtopt      = args->txtopt,
        .scale       = VEC3_ONE,
        .text_color  = VEC3_ONE,
        .text_scale  = args->keysize*2/3,
        .color       = VEC3_ZERO,
        .hit_handler = args->hit_handler,
        .label       = label,

        .tag = 0,
        .pos = 0,
    };

    int col = 0, row = 0;
    int lnlen = -1;
    const float OFFSET = args->keysize + args->padding;
    const char *head = args->layout;

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

        label[0] = head[col];
        btnargs.tag        = head[col],
        btnargs.pos        = vec3_create( (-lnlen/2.0f + col)*OFFSET, -row*OFFSET, -2);
        btnargs.scale      = vec3_all(args->keysize);
        key                = entity_button_create(ctx, &btnargs),

        comp_system_family_adopt(ctx->sys_fam, parent, key);

        col++;
    }
    return parent;
}

#endif
