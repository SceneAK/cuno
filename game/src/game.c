#include <stdio.h>
#include "system/log.h"
#include "system/graphic.h"
#include "system/graphic/text.h"
#include "system/asset.h"
#include "gmath.h"
#include "logic.h"
#include "component.h"
#include "entsys.h"
#include "game.h"

#define ORTHO_WIDTH ortho_height * aspect_ratio

/* GLOBAL CONST */
static const float  NEAR                = 0.2f;
static const float  FAR                 = 2048.0f;
static const float  FOV_DEG             = 90;

static const rect2D CARD_BOUNDS         = { -1.0f, -1.5f,   1.0f,  1.5f };
static const float  CARD_VERTS_RAW[]    = {
    CARD_BOUNDS.x0,  CARD_BOUNDS.y0, 0.0f, 0.0f, 0.0f,
    CARD_BOUNDS.x0,  CARD_BOUNDS.y1, 0.0f, 0.0f, 0.0f,
    CARD_BOUNDS.x1,  CARD_BOUNDS.y0, 0.0f, 0.0f, 0.0f,

    CARD_BOUNDS.x1,  CARD_BOUNDS.y1, 0.0f, 0.0f, 0.0f,
    CARD_BOUNDS.x0,  CARD_BOUNDS.y1, 0.0f, 0.0f, 0.0f,
    CARD_BOUNDS.x1,  CARD_BOUNDS.y0, 0.0f, 0.0f, 0.0f,
};

static const float  LINE_HEIGHT = -30.0f;

/* RESOURCES */
static struct game_state            game_state_mut;
static const struct game_state     *game_state = &game_state_mut;

static struct graphic_session      *session;
static struct graphic_session_info  session_info;

static struct baked_font            font_default;
static struct graphic_texture      *font_texture;
static struct graphic_vertecies    *card_vertecies;

static float                        aspect_ratio;
static float                        ortho_height;
static vec3                         clear_color = VEC3_ONE;
static mat4                         perspective;
static mat4                         orthographic;

static struct entity_system         entity_system;

/* ENTITIES */
static entity_t                     entity_act_button;
static entity_t                     entity_players[2];
static entity_t                     entity_top_card;
static entity_t                     entity_debug_text;

static void set_top_card(entity_t card)
{
    struct comp_transform   *transf;
    transf              = comp_transform_pool_try_get(&entity_system.transform_sys.pool, card);
    transf->trans       = vec3_create(0, 0, -5);
    transf->rot         = vec3_create(0, 0, PI/32);
    transf->scale       = vec3_create(0.4, 0.4, 0.4);
    comp_transform_system_mark_family_desync(&entity_system.transform_sys, card);
    entity_top_card = card;
}
static entity_t entity_card_create(const struct card *card)
{
    char                   temp[8];
    const float            TEXT_Z = 0.5f,
                           CHAR_WIDTH = 0.2375;
    struct comp_transform *transf;
    struct comp_visual    *visual;
    entity_t               main,
                           text;
    card_id_t             *card_id;

    main = entity_system_activate_new(&entity_system);
    transf  = comp_transform_pool_emplace(&entity_system.transform_sys.pool, main);
    visual  = comp_visual_pool_emplace(&entity_system.visual_sys.pool_persp, main);
    card_id = card_id_pool_emplace(&entity_system.cards, main);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    transf->scale           = vec3_create(2, 2, 2);
    transf->synced          = 0;
    visual->vertecies       = card_vertecies;
    visual->texture         = NULL;
    visual->color           = card_color_to_rgb(card->color);
    *card_id                = card->id;
    entity_system.signature[main] = SIGNATURE_TRANSFORM | SIGNATURE_VISUAL_PERSP | SIGNATURE_CARD;

    text = entity_system_activate_new(&entity_system);
    transf = comp_transform_pool_emplace(&entity_system.transform_sys.pool, text);
    visual = comp_visual_pool_emplace(&entity_system.visual_sys.pool_persp, text);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    transf->scale           = vec3_create(0.014f, 0.014f, 0.014f);
    switch (card->type) { /* should cache verts */
        case NUMBER:
            snprintf(temp, 8, "[ %i ]", card->num);
            transf->trans           = vec3_create(-CHAR_WIDTH*2.5, 0, TEXT_Z);
            visual->vertecies       = graphic_vertecies_create_text(font_default, LINE_HEIGHT, temp);
            break;
        case REVERSE:
            transf->trans           = vec3_create(-CHAR_WIDTH*3.5, 0, TEXT_Z);
            visual->vertecies       = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ <=> ]");
            break;
        case SKIP:
            transf->trans           = vec3_create(-CHAR_WIDTH*3, 0, TEXT_Z);
            visual->vertecies       = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ >> ]");
            break;
        case PLUS2:
            transf->trans           = vec3_create(-CHAR_WIDTH*3, 0, TEXT_Z);
            visual->vertecies       = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ +2 ]");
            break;
        case PICK_COLOR:
            transf->trans           = vec3_create(-CHAR_WIDTH*4, 0, TEXT_Z);
            visual->vertecies       = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ PICK ]");
            break;
        case PLUS4:
            transf->trans           = vec3_create(-CHAR_WIDTH*3, 0, TEXT_Z);
            visual->vertecies       = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ +4 ]");
            break;
        default:
            transf->trans           = vec3_create(-CHAR_WIDTH*3.5, 0, TEXT_Z);
            visual->vertecies       = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ ??? ]");
            break;
    }
    transf->synced = 0;
    visual->texture      = font_texture;
    visual->color        = card->color == BLACK ? VEC3_ONE : VEC3_ZERO;
    visual->draw_pass    = 1;
    entity_system_parent(&entity_system, main, text);
    entity_system.signature[text] = SIGNATURE_TRANSFORM | SIGNATURE_VISUAL_PERSP | SIGNATURE_PARENT;

    return main;
}
static void entity_card_destroy(entity_t main)
{
    int i;
    entity_t text = entity_system.first_child_map[main];
    graphic_vertecies_destroy(comp_visual_pool_try_get(&entity_system.visual_sys.pool_persp, text)->vertecies);

    entity_system_cleanup_via_signature(&entity_system, main);
    entity_system_cleanup_via_signature(&entity_system, text);

    entity_system_deactivate(&entity_system, main);
    entity_system_deactivate(&entity_system, text);
}

static void entity_player_add_cards(entity_t player, const struct card *cards, int amount)
{
    const float            DIST = 3.5f;
    entity_t               entity_card,
                           entity_card_last;
    struct comp_transform *transf,
                           transf_last;
    int                    i;

    entity_card_last = entity_system_find_last_child(&entity_system, player);
    if (entity_card_last == ENTITY_INVALID)
        comp_transform_set_default(&transf_last);
    else
        transf_last = *comp_transform_pool_try_get(&entity_system.transform_sys.pool, entity_card_last);

    for (i = 0; i < amount; i++) {
        entity_card     = entity_card_create(cards + i);
        entity_system_parent(&entity_system, player, entity_card);
        entity_system.signature[entity_card] |= SIGNATURE_PARENT;

        transf          = comp_transform_pool_try_get(&entity_system.transform_sys.pool, entity_card);
        transf->trans   = vec3_create(transf_last.trans.x + DIST, 0, 0);
        transf->rot     = vec3_create(0, -PI/12, 0);
        comp_transform_system_mark_family_desync(&entity_system.transform_sys, entity_card);

        transf_last = *transf;
    }
}
static int entity_player_remove_card(entity_t entity_card)
{
    struct comp_transform  *transf, 
                            transf_current,
                            transf_last;
    entity_t                current;

    current = entity_card;
    if (entity_is_invalid(current))
        return -1;
    
    while (!entity_is_invalid(current)) {
        transf = comp_transform_pool_try_get(&entity_system.transform_sys.pool, current);
        transf_current = *transf;
        comp_transform_system_mark_family_desync(&entity_system.transform_sys, current);
        transf->trans = transf_last.trans;

        transf_last = transf_current;
        current = entity_system.sibling_map[current];
    }
    entity_system_unparent(&entity_system, entity_card);
    return 0;
}

static void represent_current_turn() /* very stupid */
{
    struct player  *active_player = game_state->players + game_state->active_player_index;
    entity_t        entity_player = entity_players[game_state->active_player_index], /* This relies on entity_player and game_state's player to mirror */
                    current,
                    next;
    size_t          entity_card_len;
    card_id_t      *card_id;
    int i;

    if (game_state->acted == PLAY) {
        current = entity_system.first_child_map[entity_player];
        while (!entity_is_invalid(current)) {
            next = entity_system.sibling_map[current];

            card_id = card_id_pool_try_get(&entity_system.cards, current);
            if (card_find_index(active_player, *card_id) == -1) {
                entity_player_remove_card(current);
                if (*card_id == game_state->top_card.id) {
                    entity_card_destroy(entity_top_card);
                    set_top_card(current);
                } else
                    entity_card_destroy(current);
            }

            current = next;
        }
    } else if (game_state->acted == DRAW) {
        entity_card_len = entity_system_count_children(&entity_system, entity_player);
        entity_player_add_cards(entity_player, active_player->hand.elements + entity_card_len, active_player->hand.len - entity_card_len); /* relies on game_state appending behaviour */
    }
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

static entity_t entity_player_create(struct player *player)
{
    struct comp_transform *transf;
    entity_t               entity_player;

    entity_player = entity_system_activate_new(&entity_system);
    transf = comp_transform_pool_emplace(&entity_system.transform_sys.pool, entity_player);
    comp_transform_set_default(transf);
    entity_player_add_cards(entity_player, player->hand.elements, player->hand.len);

    return entity_player;
}
static void change_debug_text(const char* str)
{
    struct comp_visual *visual;
    visual = comp_visual_pool_try_get(&entity_system.visual_sys.pool_ortho, entity_debug_text);
    if (visual->vertecies)
        graphic_vertecies_destroy(visual->vertecies);
    visual->vertecies = graphic_vertecies_create_text(font_default, LINE_HEIGHT, str);
}
static void entities_init()
{
    struct comp_transform   *transf;
    struct comp_visual      *visual;
    struct comp_hitrect     *hitrect;

    entity_players[0] = entity_player_create(game_state->players + 0);
    transf              = comp_transform_pool_try_get(&entity_system.transform_sys.pool, entity_players[0]);
    transf->trans       = vec3_create(-4.5f, -5.5, -10);
    transf->rot         = vec3_create(-PI/8, 0, 0);
    transf->scale       = vec3_create(0.4, 0.4, 0.4); 
    transf->synced      = 0;
    entity_system.signature[entity_players[0]] = SIGNATURE_TRANSFORM;

    entity_players[1] = entity_player_create(game_state->players + 1);
    transf              = comp_transform_pool_try_get(&entity_system.transform_sys.pool, entity_players[1]);
    transf->trans       = vec3_create(-4.5f, 5.5, -10);
    transf->rot         = vec3_create(PI/8, 0, 0); 
    transf->scale       = vec3_create(0.3, 0.3, 0.3);
    transf->synced      = 0;
    entity_system.signature[entity_players[1]] = SIGNATURE_TRANSFORM;

    entity_top_card = entity_card_create(&game_state->top_card);
    set_top_card(entity_top_card);

    entity_act_button = entity_system_activate_new(&entity_system);
    transf  = comp_transform_pool_emplace(&entity_system.transform_sys.pool, entity_act_button);
    visual  = comp_visual_pool_emplace(&entity_system.visual_sys.pool_ortho, entity_act_button);
    hitrect = comp_hitrect_pool_emplace(&entity_system.hitrect_sys.pool, entity_act_button);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    comp_hitrect_set_default(hitrect);
    transf->trans       = vec3_create(0, -900, -1);
    transf->rot         = vec3_create(0, 0, PI/4);
    transf->scale       = vec3_create(100, 50, 1);
    transf->synced      = 0;
    visual->vertecies   = card_vertecies;
    visual->color       = vec3_create(1, 0.321568627, 0.760784314);
    hitrect->rect       = CARD_BOUNDS;
    hitrect->rect_type  = RECT_ORTHOSPACE;
    hitrect->hitmask    = HITMASK_MOUSE_DOWN | HITMASK_MOUSE_UP;
    entity_system.signature[entity_act_button] = SIGNATURE_TRANSFORM | SIGNATURE_VISUAL_ORTHO | SIGNATURE_HITRECT;

    entity_debug_text   = entity_system_activate_new(&entity_system);
    transf  = comp_transform_pool_emplace(&entity_system.transform_sys.pool, entity_debug_text);
    visual  = comp_visual_pool_emplace(&entity_system.visual_sys.pool_ortho, entity_debug_text);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    transf->trans       = vec3_create(-500, 200, -1);
    transf->scale       = vec3_create(0.6, 0.6, 0.6);
    transf->synced      = 0;
    visual->texture     = font_texture;
    visual->color       = VEC3_ZERO;
    visual->draw_pass   = 1;
    entity_system.signature[entity_act_button] = SIGNATURE_TRANSFORM | SIGNATURE_VISUAL_ORTHO;
}

static void render()
{
    graphic_clear(clear_color.x, clear_color.y, clear_color.z);

    comp_visual_system_draw(&entity_system.visual_sys, &perspective, &orthographic);

    graphic_render(session);
}

/* GAME EVENTS */
int game_init(struct graphic_session *created_session)
{
    const int PLAYER_AMOUNT     = 2;
    const int DEAL_PER_PLAYER   = 4;

    if (!resources_init(created_session))
        return -1;

    game_state_init(&game_state_mut, PLAYER_AMOUNT);
    deal_cards(&game_state_mut, DEAL_PER_PLAYER);

    entity_system_init(&entity_system);
    entities_init();

    return 0;
}

static char global_game_log_buff[4096];
void game_mouse_event(struct mouse_event event)
{
    vec2 mouse_ndc          = screen_to_ndc(session_info.width, session_info.height, event.mouse_x, event.mouse_y);
    vec3 mouse_camspace_ray = ndc_to_camspace(aspect_ratio, DEG_TO_RAD(FOV_DEG), mouse_ndc, -10);
    vec2 mouse_orthospace   = ndc_to_orthospace(ORTHO_WIDTH, ortho_height, mouse_ndc);
    unsigned char mask      = event.type == MOUSE_DOWN ? HITMASK_MOUSE_DOWN : 
                              event.type == MOUSE_UP   ? HITMASK_MOUSE_UP   : 
                              event.type == MOUSE_HOLD ? HITMASK_MOUSE_HOLD :
                              HITMASK_MOUSE_HOVER;

    struct comp_hitrect *hitrect;

    comp_hitrect_system_update_hitstate(&entity_system.hitrect_sys, &mouse_orthospace, &mouse_camspace_ray, mask);

    hitrect = comp_hitrect_pool_try_get(&entity_system.hitrect_sys.pool, entity_act_button);
    if (hitrect->hitstate) {
        clear_color.x = clear_color.x ? 0 : 1;
        if (!game_state->ended) {
            supersmartAI_act(&game_state_mut);

            log_game_state(global_game_log_buff, sizeof(global_game_log_buff), game_state);
            change_debug_text(global_game_log_buff);

            represent_current_turn();
            end_turn(&game_state_mut);
        }
        hitrect->hitstate = 0;
    }
}
void game_update()
{
    comp_transform_system_sync_matrices(&entity_system.transform_sys);
    if (!session)
        return;
    render();
}
