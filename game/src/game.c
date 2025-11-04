#include <stdio.h>
#include "system/log.h"
#include "system/graphic.h"
#include "system/graphic/text.h"
#include "system/asset.h"
#include "gmath.h"
#include "logic.h"
#include "component.h"
#include "system/time.h"
#include "game.h"

#define ORTHO_WIDTH ortho_height * aspect_ratio

#define COMPFLAG_SYS_FAMILY     1<<0
#define COMPFLAG_SYS_TRANSFORM  1<<1
#define COMPFLAG_SYS_VISUAL     1<<2
#define COMPFLAG_SYS_HITRECT    1<<3
#define COMPFLAG_SYS_INTERP     1<<4

/******* GLOBAL CONST *******/
static const float  NEAR                = 0.2f;
static const float  FAR                 = 2048.0f;
static const float  FOV_DEG             = 90;

static const float  ENTITY_CARD_DIST = 3.5f;

static const rect2D CARD_BOUNDS         = { -1.0f, -1.5f,   1.0f,  1.5f };
static const float  CARD_VERTS_RAW[]    = {
    CARD_BOUNDS.x0,  CARD_BOUNDS.y0, 0.0f, 0.0f, 0.0f,
    CARD_BOUNDS.x0,  CARD_BOUNDS.y1, 0.0f, 0.0f, 0.0f,
    CARD_BOUNDS.x1,  CARD_BOUNDS.y0, 0.0f, 0.0f, 0.0f,

    CARD_BOUNDS.x1,  CARD_BOUNDS.y1, 0.0f, 0.0f, 0.0f,
    CARD_BOUNDS.x0,  CARD_BOUNDS.y1, 0.0f, 0.0f, 0.0f,
    CARD_BOUNDS.x1,  CARD_BOUNDS.y0, 0.0f, 0.0f, 0.0f,
};
static const struct transform           DISCARD_PILE_TRANSFORM = {
    {0, 0, -5},
    {PI/32, PI/32, PI/32},
    {0.4, 0.4, 0.4}
};
static const struct transform           DRAW_PILE_TRANSFORM = {
    {10, 0, -5},
    {0, 0, PI/32},
    {0.4, 0.4, 0.4}
};
const float                             CARD_RAISE_DIST = 5.0f;
static const float                      LINE_HEIGHT = -30.0f;

/******* RESOURCES *******/
static struct game_state                game_state_alt;
static struct game_state                game_state_mut;
static const struct game_state         *game_state = &game_state_mut;

static struct graphic_session          *session;
static struct graphic_session_info      session_info;

static struct baked_font                font_default;
static struct graphic_texture          *font_texture;
static struct graphic_vertecies        *card_vertecies;

static float                            aspect_ratio;
static float                            ortho_height;
static vec3                             clear_color = VEC3_ONE;
static mat4                             perspective;
static mat4                             orthographic;

DEFINE_ARRAY_LIST_WRAPPER(static, struct act_play, play_list);
static char                             game_log_buff[4096];
static card_id_t                        card_ids[ENTITY_MAX] = {(card_id_t)-1};

static struct act_play                  curr_play = {0};
static struct play_list                 pending_plays;
static enum {
    UI_STATE_MAKE_ACT,
    UI_STATE_SELECT_ARG,
}                                       global_ui_state = UI_STATE_MAKE_ACT;

/******* COMPONENT SYSTEMS *******/
static struct entity_record             entity_record;
static struct comp_system_family       *sys_fam;
static struct comp_system_transform    *sys_transf;
static struct comp_system_visual       *sys_visual;
static struct comp_system_hitrect      *sys_hitrect;
static struct comp_system_interpolator *sys_interp;

/******* TRACKED ENTITIES *******/
static struct entity_list               entity_discards;
static entity_t                         entity_draw_button;
static entity_t                         entity_players[2];
static entity_t                         entity_debug_text;
static entity_t                         entity_color_pickers[CARD_COLOR_MAX];

static void comp_systems_init()
{
    struct comp_system base = { &entity_record };

    entity_record_init(&entity_record);

    base.component_flag = COMPFLAG_SYS_FAMILY;
    sys_fam             = comp_system_family_create(base);

    base.component_flag = COMPFLAG_SYS_TRANSFORM;
    sys_transf          = comp_system_transform_create(base, sys_fam);

    base.component_flag = COMPFLAG_SYS_VISUAL;
    sys_visual          = comp_system_visual_create(base, sys_transf);

    base.component_flag = COMPFLAG_SYS_HITRECT;
    sys_hitrect         = comp_system_hitrect_create(base, sys_transf);

    base.component_flag = COMPFLAG_SYS_INTERP;
    sys_interp          = comp_system_interpolator_create(base, sys_transf);
}
void cleanup_recorded_component_flags(entity_t entity)
{
    component_flag_t flags = entity_record.component_flags[entity];
    if (flags & COMPFLAG_SYS_FAMILY)
        comp_system_family_disown(sys_fam, entity);
    if (flags & COMPFLAG_SYS_TRANSFORM)
        comp_system_transform_erase(sys_transf, entity);
    if (flags & COMPFLAG_SYS_VISUAL)
        comp_system_visual_erase(sys_visual, entity);
    if (flags & COMPFLAG_SYS_HITRECT)
        comp_system_hitrect_erase(sys_hitrect, entity);
    if (flags & COMPFLAG_SYS_INTERP)
        comp_system_interpolator_erase(sys_interp, entity);
}


void entity_discards_arrange()
{
    const float LAYER_DIST = 0.5f;
    struct transform target = DISCARD_PILE_TRANSFORM;
    int i;

    for (i = 0; i < entity_discards.len; i++) {
        target.trans.z = DISCARD_PILE_TRANSFORM.trans.z - LAYER_DIST * (entity_discards.len - 1 - i);
        comp_system_interpolator_change(sys_interp, entity_discards.elements[i], target);
    }
}
static entity_t entity_card_create(const struct card *card)
{
    char                        temp[8];
    const float                 TEXT_Z = 0.25f,
                                CHAR_WIDTH = 0.2375;
    struct comp_transform      *transf;
    struct comp_visual         *visual;
    struct comp_hitrect        *hitrect;
    struct comp_interpolator   *interp;
    entity_t                    main,
                                text;

    main                    = entity_record_activate(&entity_record);
    transf                  = comp_system_transform_emplace(sys_transf, main);
    visual                  = comp_system_visual_emplace(sys_visual, main, PROJ_PERSP);
    interp                  = comp_system_interpolator_emplace(sys_interp, main);
    hitrect                 = comp_system_hitrect_emplace(sys_hitrect, main);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    comp_interpolator_set_default(interp);
    comp_hitrect_set_default(hitrect);
    transf->data.rot        = vec3_create(0, -PI/24, 0);
    transf->data.scale      = vec3_all(1.8);
    transf->synced          = 0;
    hitrect->rect           = CARD_BOUNDS;
    hitrect->type           = HITRECT_CAMSPACE;
    hitrect->hitmask        = HITMASK_MOUSE_DOWN;
    visual->vertecies       = card_vertecies;
    visual->texture         = NULL;
    visual->color           = card_color_to_rgb(card->color);
    interp->opt.ease_in     = 0.1f;
    interp->opt.ease_out    = 0.2f;

    card_ids[main]          = card->id;

    text                    = entity_record_activate(&entity_record);
    transf                  = comp_system_transform_emplace(sys_transf, text);
    visual                  = comp_system_visual_emplace(sys_visual, text, PROJ_PERSP);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    transf->data.scale      = vec3_create(0.014f, 0.014f, 0.014f);
    switch (card->type) { /* should cache verts */
        case CARD_NUMBER:
            snprintf(temp, 8, "[ %i ]", card->num);
            transf->data.trans  = vec3_create(-CHAR_WIDTH*2.5, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_default, LINE_HEIGHT, temp);
            break;
        case CARD_REVERSE:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*3.5, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ <-> ]");
            break;
        case CARD_SKIP:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*3, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ >> ]");
            break;
        case CARD_PLUS2:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*3, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ +2 ]");
            break;
        case CARD_PICK_COLOR:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*4, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ PICK ]");
            break;
        case CARD_PLUS4:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*3, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ +4 ]");
            break;
        default:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*3.5, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ ??? ]");
            break;
    }
    transf->synced = 0;
    visual->texture      = font_texture;
    visual->color        = card->color == CARD_COLOR_BLACK ? VEC3_ONE : VEC3_ZERO;
    visual->draw_pass    = DRAW_PASS_TRANSPARENT;
    comp_system_family_adopt(sys_fam, main, text);

    return main;
}
static void entity_card_destroy(entity_t main)
{
    size_t index;
    entity_t text = comp_system_family_view(sys_fam).first_child_map[main];

    graphic_vertecies_destroy(comp_system_visual_get(sys_visual, text)->vertecies);

    card_ids[main] = -1;

    cleanup_recorded_component_flags(main);
    cleanup_recorded_component_flags(text);

    entity_record_deactivate(&entity_record, main);
    entity_record_deactivate(&entity_record, text);
}
/* TODO: Factor a "move_by" system */
static void entity_card_start_raise(entity_t card) 
{
    struct transform    target;

    target = comp_system_transform_get(sys_transf, card)->data;
    target.trans.y += CARD_RAISE_DIST;
    comp_system_interpolator_finish(sys_interp, card);
    comp_system_interpolator_start(sys_interp, card, target);
}
static void entity_card_start_unraise(entity_t card)
{
    struct transform    target;

    target = comp_system_transform_get(sys_transf, card)->data;
    target.trans.y -= CARD_RAISE_DIST;
    comp_system_interpolator_finish(sys_interp, card);
    comp_system_interpolator_start(sys_interp, card, target);
}
static void entity_discards_free_old()
{
    const size_t    FIRST = 0;
    const int       MIN_LEN = 4;
    int             new_len,
                    i;

    if (entity_discards.len < MIN_LEN) return;
    new_len = entity_discards.len/2;
    
    for (i = 0; i < new_len; i++) 
        entity_card_destroy(entity_discards.elements[i]);
    for (i = 0; i < new_len; i++) 
        entity_list_remove_sft(&entity_discards, &FIRST, 1);
}

static void entity_player_add_cards(entity_t player, const struct card *cards, int amount)
{
    entity_t                entity_card;
    struct comp_transform  *comp_transf_card;
    struct transform        target,
                            transf_draw_pile_local;
    int                     i, child_count;

    target = comp_system_transform_get(sys_transf, player)->data;
    transf_draw_pile_local      = transform_relative(&target, &DRAW_PILE_TRANSFORM);

    child_count = comp_system_family_count_children(sys_fam, player);
    entity_card = comp_system_family_find_last_child(sys_fam, player);

    for (i = 0; i < amount; i++) {
        entity_card             = entity_card_create(cards + i);
        comp_transf_card        = comp_system_transform_get(sys_transf, entity_card);

        comp_system_transform_desync(sys_transf, entity_card);
        comp_system_family_adopt(sys_fam, player, entity_card);
        
        target = comp_system_transform_get(sys_transf, entity_card)->data;
        target.trans.x          = ENTITY_CARD_DIST * (child_count + i);
        target.trans.y          = 0;
        target.trans.z          = 0;
        comp_transf_card->data  = transf_draw_pile_local;
        comp_system_interpolator_start(sys_interp, entity_card, target);
    }
}
static int entity_player_remove_card(entity_t entity_card)
{
    struct transform                    target;
    entity_t                            current;
    struct comp_system_family_view      family_view;
    int                                 counter;

    if (entity_is_invalid(entity_card))
        return -1;
    family_view     = comp_system_family_view(sys_fam);
    current         = family_view.first_child_map[family_view.parent_map[entity_card]];

    
    for (counter = 0; current != entity_card; counter++) current = family_view.sibling_map[current];
    current = family_view.sibling_map[current];
    
    while(!entity_is_invalid(current)) {
        target          = comp_system_transform_get(sys_transf, current)->data;
        target.trans.x  = ENTITY_CARD_DIST * counter;
        target.trans.y  = 0;
        target.trans.z  = 0;
        comp_system_interpolator_change(sys_interp, current, target);

        current = family_view.sibling_map[current];
        counter++;
    }
    comp_system_transform_get(sys_transf, entity_card)->data = comp_system_transform_get_world(sys_transf, entity_card);
    comp_system_family_disown(sys_fam, entity_card);
    comp_system_transform_desync(sys_transf, entity_card);
    return 0;
}

void entity_discards_add(entity_t entity_card)
{
    entity_player_remove_card(entity_card);
    entity_list_append(&entity_discards, &entity_card, 1);
    comp_system_hitrect_get(sys_hitrect, entity_card)->active = 1;
}

static void scene_remove_player_cards(const struct player *player, entity_t entity_player)
{
    entity_t                        entity_card,
                                    last_discard = ENTITY_INVALID;
    struct comp_system_family_view  family_view;
    int                             children_count;
    int                             i;

    family_view = comp_system_family_view(sys_fam);
    entity_card = family_view.first_child_map[entity_player];

    for (;!entity_is_invalid(entity_card); entity_card = family_view.sibling_map[entity_card]) {
        for (i = 0; i < player->hand.len; i++) {
            if (player->hand.elements[i].id == card_ids[entity_card])
                break;
        }
        if (i < player->hand.len)
            continue;

        if (card_ids[entity_card] == game_state->top_card.id)
            last_discard = entity_card;
        else
            entity_discards_add(entity_card);
    }

    if (last_discard != ENTITY_INVALID)
        entity_discards_add(last_discard);


    entity_discards_arrange();
}
static void scene_add_player_cards(const struct player *player, entity_t entity_player)
{
    size_t                          entity_card_len;

    entity_discards_free_old();
    /* relies on game_state appending behaviour */
    entity_card_len = comp_system_family_count_children(sys_fam, entity_player);
    entity_player_add_cards(entity_player, player->hand.elements + entity_card_len, player->hand.len - entity_card_len); 
}
static void scene_mirror_turn()
{
    const struct player            *active_player = game_state->players + game_state->active_player_index;
    entity_t                        entity_player = entity_players[game_state->active_player_index];


    if (game_state->act == ACT_PLAY)
        scene_remove_player_cards(active_player, entity_player);
    else if (game_state->act == ACT_DRAW)
        scene_add_player_cards(active_player, entity_player);
}

static int resources_init(struct graphic_session *created_session)
{
    unsigned char   buffer[1<<19];
    asset_handle    handle;
    int i;

    session = created_session;
    session_info = graphic_session_info_get(created_session);
    
    /* ortho_height mirrors device height, aspect ratio also mirrors device's */
    aspect_ratio = ((float)session_info.width / session_info.height);
    ortho_height = session_info.height;

    handle = asset_open(ASSET_PATH_FONT);
    if (asset_read(buffer, 1<<19, handle)) {
        font_default = create_ascii_baked_font(buffer);
        font_texture = graphic_texture_create(font_default.width, font_default.height, font_default.bitmap, 1);
    }
    asset_close(handle);

    card_vertecies  = graphic_vertecies_create(CARD_VERTS_RAW, 6);

    perspective  = mat4_perspective(DEG_TO_RAD(FOV_DEG), aspect_ratio, NEAR);
    orthographic = mat4_orthographic(ORTHO_WIDTH, ortho_height, FAR); 

    return 1;
}

static entity_t entity_player_create(const struct player *player)
{
    struct comp_transform *transf;
    entity_t               entity_player;

    entity_player = entity_record_activate(&entity_record);
    transf = comp_system_transform_emplace(sys_transf, entity_player);
    comp_transform_set_default(transf);
    entity_player_add_cards(entity_player, player->hand.elements, player->hand.len);

    return entity_player;
}
static void change_debug_text(const char* str)
{
    struct comp_visual *visual;
    visual = comp_system_visual_get(sys_visual, entity_debug_text);
    if (visual->vertecies)
        graphic_vertecies_destroy(visual->vertecies);
    visual->vertecies = graphic_vertecies_create_text(font_default, LINE_HEIGHT, str);
}

static void entities_init()
{
    struct comp_transform       *transf;
    struct comp_visual          *visual;
    struct comp_hitrect         *hitrect;
    struct comp_interpolator    *interp;
    int i;

    entity_t temp;

    entity_list_init(&entity_discards, 1);
    play_list_init(&pending_plays, 1);

    temp = entity_card_create(&game_state->top_card);
    entity_list_append(&entity_discards, &temp, 1);
    comp_system_interpolator_change(sys_interp, temp, DISCARD_PILE_TRANSFORM);

    entity_players[0]       = entity_player_create(game_state->players + 0);
    transf                  = comp_system_transform_get(sys_transf, entity_players[0]);
    transf->data.trans      = vec3_create(-4.8f, -5.5, -10);
    transf->data.rot        = vec3_create(-PI/8, 0, 0);
    transf->data.scale      = vec3_create(0.4, 0.4, 0.4); 
    transf->synced          = 0;

    entity_players[1]       = entity_player_create(game_state->players + 1);
    transf                  = comp_system_transform_get(sys_transf, entity_players[1]);
    transf->data.trans      = vec3_create(-4.8f, 5.5, -10);
    transf->data.rot        = vec3_create(PI/8, 0, 0); 
    transf->data.scale      = vec3_create(0.3, 0.3, 0.3);
    transf->synced          = 0;

    entity_draw_button      = entity_record_activate(&entity_record);
    transf                  = comp_system_transform_emplace(sys_transf, entity_draw_button);
    visual                  = comp_system_visual_emplace(sys_visual, entity_draw_button, PROJ_ORTHO);
    hitrect                 = comp_system_hitrect_emplace(sys_hitrect, entity_draw_button);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    comp_hitrect_set_default(hitrect);
    transf->data.trans      = vec3_create(500, -900, -1);
    transf->data.rot        = vec3_create(0, 0, 0);
    transf->data.scale      = vec3_create(80, 40, 1);
    transf->synced          = 0;
    visual->vertecies       = card_vertecies;
    visual->color           = vec3_create(1, 0.321568627, 0.760784314);
    hitrect->rect           = CARD_BOUNDS;
    hitrect->type           = HITRECT_ORTHOSPACE;
    hitrect->hitmask        = HITMASK_MOUSE_DOWN;

    entity_debug_text       = entity_record_activate(&entity_record);
    transf                  = comp_system_transform_emplace(sys_transf, entity_debug_text);
    visual                  = comp_system_visual_emplace(sys_visual, entity_debug_text, PROJ_ORTHO);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    transf->data.trans      = vec3_create(-500, 200, -1);
    transf->data.scale      = vec3_create(0.6, 0.6, 0.6);
    transf->synced          = 0;
    visual->texture         = font_texture;
    visual->color           = VEC3_ZERO;
    visual->draw_pass       = DRAW_PASS_TRANSPARENT;

    for (i = 0; i < CARD_COLOR_MAX; i++) {
        entity_color_pickers[i] = entity_record_activate(&entity_record);
        transf                  = comp_system_transform_emplace(sys_transf, entity_color_pickers[i]);
        visual                  = comp_system_visual_emplace(sys_visual, entity_color_pickers[i], PROJ_PERSP);
        hitrect                 = comp_system_hitrect_emplace(sys_hitrect, entity_color_pickers[i]);
        interp                  = comp_system_interpolator_emplace(sys_interp, entity_color_pickers[i]);
        comp_hitrect_set_default(hitrect);
        comp_transform_set_default(transf);
        comp_visual_set_default(visual);
        comp_interpolator_set_default(interp);
        visual->vertecies       = card_vertecies;
        visual->color           = card_color_to_rgb((enum card_color)i);
        visual->draw_pass       = DRAW_PASS_NONE;
        hitrect->rect           = CARD_BOUNDS;
        hitrect->type           = HITRECT_CAMSPACE;
        hitrect->active         = 0;
        hitrect->hitmask        = HITMASK_MOUSE_DOWN;
        interp->opt.ease_in     = 0.1f;
        interp->opt.ease_out    = 0.2f;
    }
}

static void enable_popup_play_arg_color(entity_t entity_card)
{
    int                 i;
    const float         DIST                    = 0.6f;
    struct transform    card_transf             = comp_system_transform_get_world(sys_transf, entity_card);
    struct transform    targets[CARD_COLOR_MAX];

    card_transf.rot     = vec3_create(0, 0, PI/4);
    card_transf.scale   = vec3_create(0.6, 0.4, 1);
    for (i = 0; i < CARD_COLOR_MAX; i++)
        targets[i] = card_transf;

    targets[CARD_COLOR_RED].trans       = vec3_create(card_transf.trans.x + DIST, card_transf.trans.y + DIST, card_transf.trans.z + DIST);
    targets[CARD_COLOR_GREEN].trans     = vec3_create(card_transf.trans.x - DIST, card_transf.trans.y + DIST, card_transf.trans.z + DIST);
    targets[CARD_COLOR_BLUE].trans      = vec3_create(card_transf.trans.x - DIST, card_transf.trans.y - DIST, card_transf.trans.z + DIST);
    targets[CARD_COLOR_YELLOW].trans    = vec3_create(card_transf.trans.x + DIST, card_transf.trans.y - DIST, card_transf.trans.z + DIST);

    for (i = 0; i < CARD_COLOR_MAX; i++) {
        comp_system_transform_get(sys_transf, entity_color_pickers[i])->data.trans      = card_transf.trans;
        comp_system_visual_get(sys_visual, entity_color_pickers[i])->draw_pass          = DRAW_PASS_OPAQUE;
        comp_system_hitrect_get(sys_hitrect, entity_color_pickers[i])->active           = 1;

        comp_system_interpolator_start(sys_interp, entity_color_pickers[(enum card_color)i], targets[(enum card_color)i]);
    }
}
static void disable_popup_play_arg_color()
{
    int i;

    for (i = 0; i < CARD_COLOR_MAX; i++) {
        comp_system_visual_get(sys_visual, entity_color_pickers[i])->draw_pass = DRAW_PASS_NONE;
        comp_system_hitrect_get(sys_hitrect, entity_color_pickers[i])->active = 0;
    }
}
static void enable_popup_play_arg_target_player()
{
    /* TODO: Implement. */
}
static void disable_popup_play_arg_target_player()
{
    /* TODO: Implement. */
}
static void enable_popup_play_arg(enum play_arg_type arg, entity_t entity_card)
{
    switch(arg) {
        case PLAY_ARG_COLOR:
            enable_popup_play_arg_color(entity_card);
            return;
        case PLAY_ARG_PLAYER:
            enable_popup_play_arg_target_player();
            return;
        case PLAY_ARG_NONE:
            break;
    }
}
static void pending_plays_try_play(entity_t entity_card)
{
    static entity_t     last_attempt;

    int                 i;
    enum play_arg_type  arg_specs[PLAY_ARG_MAX] = {0};
    const struct card  *card;

    if (entity_card == ENTITY_INVALID)
        entity_card = last_attempt;
    else {
        curr_play = ACT_PLAY_ZERO;
        last_attempt = entity_card;
    }

    card = active_player_find_card(game_state, card_ids[entity_card]);

    curr_play.card_id = card->id;
    card_get_arg_specs(arg_specs, card);
    for (i = 0; i < PLAY_ARG_MAX; i++) {
        if (arg_specs[i] == PLAY_ARG_NONE)
            break;

        if (arg_specs[i] != curr_play.args[i].type) {
            enable_popup_play_arg(arg_specs[i], entity_card);
            global_ui_state = UI_STATE_SELECT_ARG;
            return;
        }
    }
    global_ui_state = UI_STATE_MAKE_ACT;
 
    if (!pending_plays.len)
        game_state_copy(&game_state_alt, game_state);

    if (!game_state_can_act_play(&game_state_alt, curr_play))
        return;

    game_state_act_play(&game_state_alt, curr_play);
    play_list_append(&pending_plays, &curr_play, 1);

    entity_card_start_raise(entity_card);
}
static void pending_plays_try_unplay(entity_t entity_card)
{
    struct transform    target;
    size_t              i, card_index;

    game_state_copy(&game_state_alt, game_state);
    for (i = 0; i < pending_plays.len; i++) {
        if (pending_plays.elements[i].card_id == card_ids[entity_card]) {
            card_index = i;
            continue;
        }

        if (!game_state_can_act_play(&game_state_alt, pending_plays.elements[i]))
            break;
    }
    if (i == pending_plays.len) {
        play_list_remove_sft(&pending_plays, &card_index, 1);
        entity_card_start_unraise(entity_card);
        return;
    }

    game_state_copy(&game_state_alt, game_state);
    for (i = 0; i < pending_plays.len; i++)
        game_state_act_play(&game_state_alt, pending_plays.elements[i]);
}
static void pending_plays_clear()
{
    struct comp_system_family_view  view;
    entity_t                        entity_card;
    size_t                          i;

    view        = comp_system_family_view(sys_fam);
    entity_card = view.first_child_map[entity_players[0]];

    for (;!entity_is_invalid(entity_card); entity_card = view.sibling_map[entity_card]) {
        for (i = 0; i < pending_plays.len; i++) {
            if (pending_plays.elements[i].card_id == card_ids[entity_card])
                break;
        }
        if (i == pending_plays.len)
            continue;

        entity_card_start_unraise(entity_card);
        play_list_remove_swp(&pending_plays, &i, 1);
    }
}
static void on_state_select_arg()
{
    int                             i;
    enum card_color                 color;

    for (i = 0; i < PLAY_ARG_MAX; i++) {
        if (curr_play.args[i].type == PLAY_ARG_NONE)
            break;
    }

    for (color = 0; color < CARD_COLOR_MAX; color++) {
        if (!comp_system_hitrect_check_and_clear_state(sys_hitrect, entity_color_pickers[color]))
            continue;

        curr_play.args[i].type      = PLAY_ARG_COLOR;
        curr_play.args[i].u.color   = color;

        pending_plays_try_play(ENTITY_INVALID);
        disable_popup_play_arg_color();
        return;
    }

    /* Implement Player picking checks here if no colors are chosen
     * foreach clickable player hitrects...
     *         if any of them are hit, update curr_play.args[i] 
     *         pending_plays_try_play();
     *
     * */
    return;
}

static void on_card_hit(entity_t entity_card)
{
    int i;

    for (i = 0; i < pending_plays.len; i++) {
        if (pending_plays.elements[i].card_id == card_ids[entity_card])
            break;
    }
    if (i == pending_plays.len) {
        pending_plays_try_play(entity_card);
    } else {
        pending_plays_try_unplay(entity_card);
    }

}
static void on_state_make_act()
{
    struct comp_system_family_view  view;
    entity_t                        top_card,
                                    entity_card;
    int                             i;

    view        = comp_system_family_view(sys_fam);
    entity_card = view.first_child_map[entity_players[0]];

    while (!entity_is_invalid(entity_card)) {
        if (comp_system_hitrect_check_and_clear_state(sys_hitrect, entity_card))
            break;

        entity_card = view.sibling_map[entity_card];
    }
    if (!entity_is_invalid(entity_card))
        on_card_hit(entity_card);

    top_card = entity_discards.elements[entity_discards.len-1];
    if (comp_system_hitrect_check_and_clear_state(sys_hitrect, top_card) && pending_plays.len > 0) {
        game_state_copy(&game_state_mut, &game_state_alt);
        pending_plays_clear();
        scene_mirror_turn();
        game_state_end_turn(&game_state_mut);
        return;
    }

    if (comp_system_hitrect_check_and_clear_state(sys_hitrect, entity_draw_button)) {
        game_state_act_draw(&game_state_mut);
        pending_plays_clear();
        scene_mirror_turn();
        game_state_end_turn(&game_state_mut);
        return;
    }
}
static void on_hitrect_state_update()
{
    if (game_state->active_player_index != 0)
        return;

    if (global_ui_state == UI_STATE_SELECT_ARG) {
        on_state_select_arg();
        return;
    }

    if (global_ui_state == UI_STATE_MAKE_ACT) {
        on_state_make_act();
        return;
    }
}

static void render()
{
    graphic_clear(clear_color.x, clear_color.y, clear_color.z);
    comp_system_visual_draw(sys_visual, &perspective, &orthographic);
    graphic_render(session);
}

static double auto_act_time = -1;
void player1_update()
{
    const double ACT_DELAY = 1.0;

    if (game_state->active_player_index != 1)
        return;

    if (auto_act_time == -1)
        auto_act_time = get_monotonic_time() + ACT_DELAY;
    else if (auto_act_time <= get_monotonic_time()) {
        game_state_act_auto(&game_state_mut);

        scene_mirror_turn();
        pending_plays_clear();
        game_state_end_turn(&game_state_mut);
        auto_act_time = -1;
    }
}

/***** GAME EVENTS *****/
int game_init(struct graphic_session *created_session)
{
    const int PLAYER_AMOUNT     = 2;
    const int DEAL_PER_PLAYER   = 4;
    /* const unsigned int seed     = 1482941; */
    const unsigned int seed  = get_monotonic_time();

    srand(seed);
    LOGF("srand seed: %u", seed);

    if (!resources_init(created_session))
        return -1;

    game_state_init(&game_state_mut, PLAYER_AMOUNT, DEAL_PER_PLAYER);

    comp_systems_init();
    entities_init();

    return 0;
}

void game_mouse_event(struct mouse_event event)
{
    vec2 mouse_ndc          = screen_to_ndc(session_info.width, session_info.height, event.mouse_x, event.mouse_y);
    vec3 mouse_camspace_ray = ndc_to_camspace(aspect_ratio, DEG_TO_RAD(FOV_DEG), mouse_ndc, -10);
    vec2 mouse_orthospace   = ndc_to_orthospace(ORTHO_WIDTH, ortho_height, mouse_ndc);
    unsigned char mask      = event.type == MOUSE_DOWN ? HITMASK_MOUSE_DOWN :
                              event.type == MOUSE_UP   ? HITMASK_MOUSE_UP   :
                              event.type == MOUSE_HOLD ? HITMASK_MOUSE_HOLD :
                              HITMASK_MOUSE_HOVER;
 
    comp_system_hitrect_update(sys_hitrect, &mouse_orthospace, &mouse_camspace_ray, mask); 
    on_hitrect_state_update();
    comp_system_hitrect_clear_states(sys_hitrect); /* Forces checks every frame, wastefully */
}
void game_update()
{
    if (!session)
        return;

    comp_system_transform_sync_matrices(sys_transf);
    comp_system_interpolator_update(sys_interp);
    player1_update();

    render();
}
