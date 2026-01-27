#include <stdio.h>
#include "engine/entity/keyboard.h"
#include "engine/entity/control.h"
#include "engine/entity/world.h"
#include "engine/system/graphic.h"
#include "engine/system/network.h"
#include "engine/system/asset.h"
#include "engine/system/time.h"
#include "engine/system/log.h"
#include "engine/component.h"
#include "engine/text.h"
#include "engine/math.h"

#include "engine/game.h"
#include "logic.h"
#include "serialize.h"

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

static const float  ENTITY_CARD_DIST    = 3.0f;

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
static const float                      CARD_RAISE_DIST = 5.0f;
static const float                      LINE_HEIGHT = -30.0f;
static const float                      CUNO_PORT = 7777;

/******* RESOURCES *******/
static struct game_state                game_state_alt;
static struct game_state                game_state_mut;
static const struct game_state         *game_state = &game_state_mut;

static struct graphic_session          *session;
static struct graphic_session_info      session_info;

static struct baked_font                font_spec_default;
static struct graphic_texture          *font_tex;
static struct graphic_vertecies        *card_vertecies;

static float                            aspect_ratio;
static float                            ortho_height;
static vec3                             clear_color = {1, 1, 1};
static mat4                             perspective;
static mat4                             orthographic;

DEFINE_ARRAY_LIST_WRAPPER(static, struct act_play, play_list);
static char                             gamelog_charbuff[4096]  = {0};
static char                             ipv4_chrbuff[64]        = {0};
static card_id_t                        card_ids[ENTITY_MAX] = {(card_id_t)-1};

static struct act_play                  curr_play = {0};
static struct play_list                 staged_plays;
static enum {
    UI_STATE_MAKE_ACT,
    UI_STATE_SELECT_ARG,
}                                       global_ui_state = UI_STATE_MAKE_ACT;

/******* DEFAULTS ********/
struct entity_text_opt default_txtopt;

/******* MAIN WORLD & ENTITIES *******/
static struct entity_world              world_main;
static struct entity_list               main_entity_discard;
static entity_t                         main_entity_btn_draw;
static entity_t                         main_entity_players[2];
static entity_t                         main_entity_txt_debug;
static entity_t                         main_entity_btn_colors[CARD_COLOR_MAX];

/******* MENU WORLD & ENTITIES *******/
static struct entity_world              world_menu;
static entity_t                         menu_entity_txt_ipv4;
static entity_t                         menu_entity_keyboard;
static entity_t                         menu_entity_btn_connect;
static entity_t                         menu_entity_btn_host;

static struct entity_world             *active_world;

void entity_discards_arrange()
{
    const float LAYER_DIST = 0.8f;
    struct transform target = DISCARD_PILE_TRANSFORM;
    int i;

    for (i = 0; i < main_entity_discard.len; i++) {
        target.trans.z = DISCARD_PILE_TRANSFORM.trans.z - LAYER_DIST * (main_entity_discard.len - 1 - i);
        comp_system_interpolator_change(world_main.sys_interp, main_entity_discard.elems[i], target);
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

    main                    = entity_record_activate(&world_main.records);
    transf                  = comp_system_transform_emplace(world_main.sys_transf, main);
    visual                  = comp_system_visual_emplace(world_main.sys_vis, main, PROJ_PERSP);
    interp                  = comp_system_interpolator_emplace(world_main.sys_interp, main);
    hitrect                 = comp_system_hitrect_emplace(world_main.sys_hitrect, main);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    comp_interpolator_set_default(interp);
    comp_hitrect_set_default(hitrect);
    transf->data.rot        = vec3_create(0, -PI/16, 0);
    transf->data.scale      = vec3_all(1.7);
    hitrect->rect           = CARD_BOUNDS;
    hitrect->type           = HITRECT_CAMSPACE;
    hitrect->hitmask        = HITMASK_MOUSE_DOWN;
    visual->vertecies       = card_vertecies;
    visual->texture         = NULL;
    visual->color           = card_color_to_rgb(card->color);
    interp->opt.ease_in     = 0.1f;
    interp->opt.ease_out    = 0.2f;

    card_ids[main]          = card->id;

    text                    = entity_record_activate(&world_main.records);
    transf                  = comp_system_transform_emplace(world_main.sys_transf, text);
    visual                  = comp_system_visual_emplace(world_main.sys_vis, text, PROJ_PERSP);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    transf->data.scale      = vec3_create(0.014f, 0.014f, 0.014f);
    switch (card->type) { /* should cache verts */
        case CARD_NUMBER:
            snprintf(temp, 8, "[ %i ]", card->num);
            transf->data.trans  = vec3_create(-CHAR_WIDTH*2.5, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_spec_default, LINE_HEIGHT, temp, 0);
            break;
        case CARD_REVERSE:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*3.5, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_spec_default, LINE_HEIGHT, "[ <-> ]", 0);
            break;
        case CARD_SKIP:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*3, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_spec_default, LINE_HEIGHT, "[ >> ]", 0);
            break;
        case CARD_PLUS2:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*3, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_spec_default, LINE_HEIGHT, "[ +2 ]", 0);
            break;
        case CARD_PICK_COLOR:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*4, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_spec_default, LINE_HEIGHT, "[ PICK ]", 0);
            break;
        case CARD_PLUS4:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*3, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_spec_default, LINE_HEIGHT, "[ +4 ]", 0);
            break;
        default:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*3.5, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_spec_default, LINE_HEIGHT, "[ ??? ]", 0);
            break;
    }
    visual->texture      = font_tex;
    visual->color        = card->color == CARD_COLOR_BLACK ? VEC3_ONE : VEC3_ZERO;
    visual->draw_pass    = DRAW_PASS_TRANSPARENT;
    comp_system_family_adopt(world_main.sys_fam, main, text);

    return main;
}
static void entity_card_destroy(entity_t main)
{
    size_t index;
    entity_t text = comp_system_family_view(world_main.sys_fam).first_child_map[main];

    graphic_vertecies_destroy(comp_system_visual_get(world_main.sys_vis, text)->vertecies);

    card_ids[main] = -1;

    entity_world_cleanup(&world_main, main);
    entity_world_cleanup(&world_main, text);

    entity_record_deactivate(&world_main.records, main);
    entity_record_deactivate(&world_main.records, text);
}
/* TODO: Factor a "move_by" system */
static void entity_card_start_raise(entity_t card) 
{
    struct transform    target;

    target = comp_system_transform_get(world_main.sys_transf, card)->data;
    target.trans.y += CARD_RAISE_DIST;
    comp_system_interpolator_finish(world_main.sys_interp, card);
    comp_system_interpolator_start(world_main.sys_interp, card, target);
}
static void entity_card_start_unraise(entity_t card)
{
    struct transform    target;

    target = comp_system_transform_get(world_main.sys_transf, card)->data;
    target.trans.y -= CARD_RAISE_DIST;
    comp_system_interpolator_finish(world_main.sys_interp, card);
    comp_system_interpolator_start(world_main.sys_interp, card, target);
}
static void entity_discards_free_old()
{
    const size_t    FIRST = 0;
    const int       MIN_LEN = 4;
    int             new_len,
                    i;

    if (main_entity_discard.len < MIN_LEN) return;
    new_len = main_entity_discard.len/2;
    
    for (i = 0; i < new_len; i++) 
        entity_card_destroy(main_entity_discard.elems[i]);
    for (i = 0; i < new_len; i++) 
        entity_list_remove_sft(&main_entity_discard, &FIRST, 1);
}

static void entity_player_add_cards(entity_t player, const struct card *cards, int amount)
{
    entity_t                entity_card;
    struct comp_transform  *comp_transf_card;
    struct transform        target,
                            transf_draw_pile_local;
    int                     i, child_count;

    target = comp_system_transform_get(world_main.sys_transf, player)->data;
    transf_draw_pile_local      = transform_relative(&target, &DRAW_PILE_TRANSFORM);

    child_count = comp_system_family_count_children(world_main.sys_fam, player);
    entity_card = comp_system_family_find_last_child(world_main.sys_fam, player);

    for (i = 0; i < amount; i++) {
        entity_card             = entity_card_create(cards + i);
        comp_transf_card        = comp_system_transform_get(world_main.sys_transf, entity_card);

        comp_system_transform_desync(world_main.sys_transf, entity_card);
        comp_system_family_adopt(world_main.sys_fam, player, entity_card);
        
        target = comp_system_transform_get(world_main.sys_transf, entity_card)->data;
        target.trans.x          = ENTITY_CARD_DIST * (child_count + i);
        target.trans.y          = 0;
        target.trans.z          = 0;
        comp_transf_card->data  = transf_draw_pile_local;
        comp_system_interpolator_start(world_main.sys_interp, entity_card, target);
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
    family_view     = comp_system_family_view(world_main.sys_fam);
    current         = family_view.first_child_map[family_view.parent_map[entity_card]];

    
    for (counter = 0; current != entity_card; counter++) current = family_view.sibling_map[current];
    current = family_view.sibling_map[current];
    
    while(!entity_is_invalid(current)) {
        target          = comp_system_transform_get(world_main.sys_transf, current)->data;
        target.trans.x  = ENTITY_CARD_DIST * counter;
        target.trans.y  = 0;
        target.trans.z  = 0;
        comp_system_interpolator_change(world_main.sys_interp, current, target);

        current = family_view.sibling_map[current];
        counter++;
    }
    comp_system_transform_get(world_main.sys_transf, entity_card)->data = comp_system_transform_get_world(world_main.sys_transf, entity_card);
    comp_system_family_disown(world_main.sys_fam, entity_card);
    comp_system_transform_desync(world_main.sys_transf, entity_card);
    return 0;
}

void entity_discards_discard(entity_t entity_card)
{
    entity_player_remove_card(entity_card);
    entity_list_append(&main_entity_discard, &entity_card, 1);
    comp_system_hitrect_get(world_main.sys_hitrect, entity_card)->active = 1;
}

static void scene_remove_player_cards(const struct player *player, entity_t entity_player)
{
    entity_t                        entity_card,
                                    last_discard = ENTITY_INVALID,
                                    next;
    struct comp_system_family_view  family_view;
    int                             children_count;
    int                             i;

    family_view = comp_system_family_view(world_main.sys_fam);
    entity_card = next = family_view.first_child_map[entity_player];

    for (; !entity_is_invalid(entity_card); entity_card = next) {
        for (i = 0; i < player->hand.len; i++) {
            if (player->hand.elems[i].id == card_ids[entity_card])
                break;
        }

        next = family_view.sibling_map[entity_card];

        if (i < player->hand.len)
            continue;

        if (card_ids[entity_card] == game_state->top_card.id)
            last_discard = entity_card;
        else
            entity_discards_discard(entity_card);
    }

    if (last_discard != ENTITY_INVALID)
        entity_discards_discard(last_discard);

    entity_discards_arrange();
}

static void scene_add_player_cards(const struct player *player, entity_t entity_player)
{
    size_t                          entity_card_len;

    entity_discards_free_old();
    /* relies on game_state appending behaviour */
    entity_card_len = comp_system_family_count_children(world_main.sys_fam, entity_player);
    entity_player_add_cards(entity_player, player->hand.elems + entity_card_len, player->hand.len - entity_card_len); 
}

static void scene_mirror_turn()
{
    const struct player            *active_player = game_state->players + game_state->active_player_index;
    entity_t                        entity_player = main_entity_players[game_state->active_player_index];


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
        font_spec_default = create_ascii_baked_font(buffer);
        font_tex = graphic_texture_create(font_spec_default.width, font_spec_default.height, font_spec_default.bitmap, 1);
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

    entity_player = entity_record_activate(&world_main.records);
    transf = comp_system_transform_emplace(world_main.sys_transf, entity_player);
    comp_transform_set_default(transf);
    entity_player_add_cards(entity_player, player->hand.elems, player->hand.len);

    return entity_player;
}


static void on_entity_keyboard_hit(entity_t e, struct comp_hitrect *hitrect)
{
    static int octet_count = 1;
    static int octet_idx = 0;
    static int digits = 0;

    if (octet_count > 4) {
        octet_count = 1;
        digits = 0;
        octet_idx = 0;
        ipv4_chrbuff[octet_idx + digits] = '\0';
    }

    if (digits < 3) {
        ipv4_chrbuff[octet_idx + digits] = (char)hitrect->tag;
        digits++;
        ipv4_chrbuff[octet_idx + digits] = '\0';
    }

    if (digits >= 3) {
        cuno_logf(LOG_INFO, "dot_idx:%d", octet_idx);
        octet_idx += snprintf(ipv4_chrbuff + octet_idx, sizeof(ipv4_chrbuff) - octet_idx, 
                        octet_count < 4 ? "%d." : "%d", atoi(ipv4_chrbuff + octet_idx));

        octet_count++;
        digits = 0;
    }
    entity_text_change(&world_menu, &default_txtopt, menu_entity_txt_ipv4, ipv4_chrbuff);

    cuno_logf(LOG_INFO, "octets: %d, digits: %d, octet_idx: %d", octet_count, digits, octet_idx);
}

static void on_host_btn(entity_t e, struct comp_hitrect *hitrect)
{
}

static void on_connect_btn(entity_t e, struct comp_hitrect *hitrect)
{
    cuno_logf(LOG_INFO, "Connecting to %s port %d", ipv4_chrbuff, CUNO_PORT);
    struct network_connection *conn = network_connection_create(ipv4_chrbuff, CUNO_PORT);
    if (!conn) {
        clear_color = VEC3_RED;
        return;
    }
    clear_color = VEC3_BLUE;
}

static void entities_init()
{
    struct comp_transform       *transf;
    struct comp_visual          *visual;
    struct comp_hitrect         *hitrect;
    struct comp_interpolator    *interp;

    struct entity_button_args btnargs;
    const struct entity_button_args DEFAULT_BTNARGS =  {
        .txtopt      = &default_txtopt,
        .text_color  = VEC3_ZERO,
        .text_scale  = 50,
        .tag         = 0,

        .label       = NULL,
        .pos         = VEC3_ZERO,
        .scale       = VEC3_ONE,
        .color       = VEC3_RED,
        .hit_handler = NULL,
    };

    struct entity_keyboard_args kbargs = {
        .layout      = KBLAYOUT_NUMPAD,
        .padding     = 25,
        .keysize     = 135,
        .txtopt      = &default_txtopt,
        .hit_handler = &on_entity_keyboard_hit,
    };

    entity_t temp;
    int i;

    entity_list_init(&main_entity_discard, 1);
    play_list_init(&staged_plays, 1);

    temp = entity_card_create(&game_state->top_card);
    entity_list_append(&main_entity_discard, &temp, 1);
    comp_system_interpolator_change(world_main.sys_interp, temp, DISCARD_PILE_TRANSFORM);

    main_entity_players[0]  = entity_player_create(game_state->players + 0);
    transf                  = comp_system_transform_get(world_main.sys_transf, main_entity_players[0]);
    transf->data.trans      = vec3_create(-4.2f, -5.5, -10);
    transf->data.rot        = vec3_create(-PI/8, 0, 0);
    transf->data.scale      = vec3_create(0.4, 0.4, 0.4); 
    comp_system_transform_desync(world_main.sys_transf, main_entity_players[0]);

    main_entity_players[1]  = entity_player_create(game_state->players + 1);
    transf                  = comp_system_transform_get(world_main.sys_transf, main_entity_players[1]);
    transf->data.trans      = vec3_create(-4.2f, 5.5, -10);
    transf->data.rot        = vec3_create(PI/8, 0, 0); 
    transf->data.scale      = vec3_create(0.3, 0.3, 0.3);
    comp_system_transform_desync(world_main.sys_transf, main_entity_players[1]);

    btnargs = DEFAULT_BTNARGS;
    btnargs.pos             = vec3_create(-150, 400, -2);
    btnargs.scale           = vec3_create(110, 78, 1);
    btnargs.color           = vec3_create(1, 0.321568627, 0.760784314);
    btnargs.text_scale      = 60;
    btnargs.label           = "Draw";
    main_entity_btn_draw    = entity_button_create(&world_main, &btnargs);

    main_entity_txt_debug   = entity_text_create(&world_main, &default_txtopt, vec3_create(-500, 200, -1), 50, VEC3_ZERO);

    menu_entity_keyboard    = entity_keyboard_create(&world_menu, &kbargs);
    transf                  = comp_system_transform_get(world_menu.sys_transf, menu_entity_keyboard);
    comp_transform_set_default(transf);
    transf->data.trans.z    = -1;
    comp_system_transform_desync(world_menu.sys_transf, menu_entity_keyboard);

    menu_entity_txt_ipv4    = entity_text_create(&world_menu, &default_txtopt, vec3_create(-400, 200, -2), 60, VEC3_ZERO);

    btnargs = DEFAULT_BTNARGS;
    btnargs.pos             = vec3_create(-300, 400, -1);
    btnargs.scale           = vec3_create(180, 45, 1);
    btnargs.color           = VEC3_GREEN;
    btnargs.text_scale      = 25;
    btnargs.label           = "Connect";
    btnargs.hit_handler     = &on_connect_btn;
    menu_entity_btn_connect = entity_button_create(&world_menu, &btnargs);
    cuno_logf(LOG_INFO, "connect: %d", menu_entity_btn_connect);

    btnargs = DEFAULT_BTNARGS;
    btnargs.pos             = vec3_create(0, 400, -1);
    btnargs.scale           = vec3_create(120, 45, 1);
    btnargs.color           = VEC3_RED;
    btnargs.text_scale      = 25;
    btnargs.label           = "Host";
    btnargs.hit_handler     = &on_host_btn;
    menu_entity_btn_host    = entity_button_create(&world_menu, &btnargs);
    cuno_logf(LOG_INFO, "host: %d", menu_entity_btn_host);
    comp_system_transform_desync_everything(world_menu.sys_transf);

    for (i = 0; i < CARD_COLOR_MAX; i++) {
        main_entity_btn_colors[i] = entity_record_activate(&world_main.records);
        transf                  = comp_system_transform_emplace(world_main.sys_transf, main_entity_btn_colors[i]);
        visual                  = comp_system_visual_emplace(world_main.sys_vis, main_entity_btn_colors[i], PROJ_PERSP);
        hitrect                 = comp_system_hitrect_emplace(world_main.sys_hitrect, main_entity_btn_colors[i]);
        interp                  = comp_system_interpolator_emplace(world_main.sys_interp, main_entity_btn_colors[i]);
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
    const float         DIST                    = 0.62f;
    struct transform    card_transf             = comp_system_transform_get_world(world_main.sys_transf, entity_card);
    struct transform    targets[CARD_COLOR_MAX];

    card_transf.rot     = vec3_create(0, 0, PI/4);
    card_transf.scale   = vec3_create(0.7, 0.46, 1);
    for (i = 0; i < CARD_COLOR_MAX; i++)
        targets[i] = card_transf;

    targets[CARD_COLOR_RED].trans       = vec3_create(card_transf.trans.x + DIST, card_transf.trans.y + DIST, card_transf.trans.z + DIST);
    targets[CARD_COLOR_GREEN].trans     = vec3_create(card_transf.trans.x - DIST, card_transf.trans.y + DIST, card_transf.trans.z + DIST);
    targets[CARD_COLOR_BLUE].trans      = vec3_create(card_transf.trans.x - DIST, card_transf.trans.y - DIST, card_transf.trans.z + DIST);
    targets[CARD_COLOR_YELLOW].trans    = vec3_create(card_transf.trans.x + DIST, card_transf.trans.y - DIST, card_transf.trans.z + DIST);

    for (i = 0; i < CARD_COLOR_MAX; i++) {
        comp_system_transform_get(world_main.sys_transf, main_entity_btn_colors[i])->data.trans      = card_transf.trans;
        comp_system_visual_get(world_main.sys_vis, main_entity_btn_colors[i])->draw_pass             = DRAW_PASS_OPAQUE;
        comp_system_hitrect_get(world_main.sys_hitrect, main_entity_btn_colors[i])->active           = 1;

        comp_system_interpolator_start(world_main.sys_interp, main_entity_btn_colors[(enum card_color)i], targets[(enum card_color)i]);
    }
}
static void disable_popup_play_arg_color()
{
    int i;

    for (i = 0; i < CARD_COLOR_MAX; i++) {
        comp_system_visual_get(world_main.sys_vis, main_entity_btn_colors[i])->draw_pass = DRAW_PASS_NONE;
        comp_system_hitrect_get(world_main.sys_hitrect, main_entity_btn_colors[i])->active = 0;
    }
}

static void enable_popup_play_arg_target_player() { /* TODO: Implement. */ }
static void disable_popup_play_arg_target_player() { /* TODO: Implement. */ }

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
static void staged_plays_try_play(entity_t entity_card)
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
    card_get_arg_type_specs(arg_specs, card);
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
 
    if (!staged_plays.len)
        game_state_copy_into(&game_state_alt, game_state);

    if (!game_state_can_act_play(&game_state_alt, curr_play))
        return;

    game_state_act_play(&game_state_alt, curr_play);
    play_list_append(&staged_plays, &curr_play, 1);

    entity_card_start_raise(entity_card);
}
static void staged_plays_try_unplay(entity_t entity_card)
{
    struct transform    target;
    size_t              i, card_index;

    game_state_copy_into(&game_state_alt, game_state);
    for (i = 0; i < staged_plays.len; i++) {
        if (staged_plays.elems[i].card_id == card_ids[entity_card]) {
            card_index = i;
            continue;
        }

        if (game_state_act_play(&game_state_alt, staged_plays.elems[i]) != 0)
            break;
    }
    if (i == staged_plays.len) {
        play_list_remove_sft(&staged_plays, &card_index, 1);
        entity_card_start_unraise(entity_card);
        return;
    }

    game_state_copy_into(&game_state_alt, game_state);
    for (i = 0; i < staged_plays.len; i++)
        game_state_act_play(&game_state_alt, staged_plays.elems[i]);
}
static void staged_plays_clear()
{
    struct comp_system_family_view  view;
    entity_t                        entity_card;
    size_t                          i;

    view        = comp_system_family_view(world_main.sys_fam);
    entity_card = view.first_child_map[main_entity_players[0]];

    for (;!entity_is_invalid(entity_card); entity_card = view.sibling_map[entity_card]) {
        for (i = 0; i < staged_plays.len; i++) {
            if (staged_plays.elems[i].card_id == card_ids[entity_card])
                break;
        }
        if (i == staged_plays.len)
            continue;

        entity_card_start_unraise(entity_card);
        play_list_remove_swp(&staged_plays, &i, 1);
    }
}
static void end_turn()
{
    staged_plays_clear();
    scene_mirror_turn();
    game_state_end_turn(&game_state_mut);
    
    log_game_state(gamelog_charbuff, sizeof(gamelog_charbuff), game_state, 0);
    entity_text_change(&world_main, &default_txtopt, main_entity_txt_debug, gamelog_charbuff);
    cuno_logf(LOG_INFO, gamelog_charbuff);
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
        if (!comp_system_hitrect_check_and_clear_state(world_main.sys_hitrect, main_entity_btn_colors[color]))
            continue;

        curr_play.args[i].type      = PLAY_ARG_COLOR;
        curr_play.args[i].u.color   = color;

        staged_plays_try_play(ENTITY_INVALID);
        disable_popup_play_arg_color();
        return;
    }

    /* Implement Player picking checks here if no colors are chosen
     * foreach clickable player hitrects...
     *         if any of them are hit, update curr_play.args[i] 
     *         staged_plays_try_play();
     *
     * */
    return;
}

static void on_card_hit(entity_t entity_card)
{
    int i;

    for (i = 0; i < staged_plays.len; i++) {
        if (staged_plays.elems[i].card_id == card_ids[entity_card])
            break;
    }
    if (i == staged_plays.len) {
        staged_plays_try_play(entity_card);
    } else {
        staged_plays_try_unplay(entity_card);
    }

}
static void on_state_make_act()
{
    struct comp_system_family_view  view;
    entity_t                        top_card,
                                    entity_card;
    int                             i;

    view        = comp_system_family_view(world_main.sys_fam);
    entity_card = view.first_child_map[main_entity_players[0]];

    while (!entity_is_invalid(entity_card)) {
        if (comp_system_hitrect_check_and_clear_state(world_main.sys_hitrect, entity_card))
            break;

        entity_card = view.sibling_map[entity_card];
    }
    if (!entity_is_invalid(entity_card))
        on_card_hit(entity_card);

    top_card = main_entity_discard.elems[main_entity_discard.len-1];
    if (comp_system_hitrect_check_and_clear_state(world_main.sys_hitrect, top_card) && staged_plays.len > 0) {
        game_state_copy_into(&game_state_mut, &game_state_alt);
        end_turn();
        return;
    }

    if (comp_system_hitrect_check_and_clear_state(world_main.sys_hitrect, main_entity_btn_draw)) {
        game_state_act_draw(&game_state_mut);
        end_turn();
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
    comp_system_visual_draw(active_world->sys_vis, &perspective, &orthographic);
    graphic_render(session);
}

static double auto_act_time = -1;
void player1_update()
{
    const double ACT_DELAY = 1.4;

    if (game_state->active_player_index != 1)
        return;
    clear_color.x = 0.2;

    if (auto_act_time == -1)
        auto_act_time = get_monotonic_time() + ACT_DELAY;
    else if (auto_act_time <= get_monotonic_time()) {
        game_state_act_auto(&game_state_mut);
        end_turn();
        clear_color.x = 1;
        auto_act_time = -1;
    }
}

/***** GAME.H EVENTS *****/
int game_init(struct graphic_session *created_session)
{
    const int PLAYER_AMOUNT     = 2;
    const int DEAL_PER_PLAYER   = 4;
    /* const unsigned int seed     = 1506742; */
    const unsigned int seed  = get_monotonic_time();

    srand(seed);
    cuno_logf(LOG_INFO, "srand seed: %u", seed);

    if (!resources_init(created_session))
        return -1;

    game_state_init(&game_state_mut);
    game_state_start(&game_state_mut, PLAYER_AMOUNT, DEAL_PER_PLAYER);

    default_txtopt.font_tex     = font_tex,
    default_txtopt.font_spec    = &font_spec_default;
    default_txtopt.lnheight     = LINE_HEIGHT;
    default_txtopt.normalize    = 1;

    entity_world_init(&world_main);
    entity_world_init(&world_menu);
    entities_init();

    active_world = &world_menu;

    log_game_state(gamelog_charbuff, sizeof(gamelog_charbuff), game_state, 0);
    entity_text_change(&world_main, &default_txtopt, main_entity_txt_debug, gamelog_charbuff);
    return 0;
}

void game_update()
{
    if (!session)
        return;

    comp_system_transform_sync_matrices(active_world->sys_transf);
    comp_system_interpolator_update(active_world->sys_interp);

    if (active_world == &world_main) /* should prob be its own comp */
        player1_update();

    render();
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
 
    comp_system_hitrect_update(active_world->sys_hitrect, &mouse_orthospace, &mouse_camspace_ray, mask); 
    on_hitrect_state_update();
    comp_system_hitrect_clear_states(active_world->sys_hitrect); /* Forces checks every frame, wastefully */
}
