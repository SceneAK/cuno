#include <stdio.h>
#include "system/logging.h"
#include "system/graphic/graphic.h"
#include "system/graphic/text.h"
#include "system/asset_management.h"
#include "game_math.h"
#include "game_logic.h"
#include "component.h"
#include "main.h"

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

/* OBJECTS */
static entity_t act_button;

static entity_t entity_card_create(const struct card *card)
{
    char                   temp[8];
    const float            LINE_HEIGHT = 0.5f,
                           TEXT_Z = 0.5f;
    struct comp_transform *transf;
    struct comp_visual    *visual;
    card_id_t             *card_id;
    entity_t               main,
                           text;

    main    = entity_system_activate_new(&entity_system);
    transf  = comp_transform_pool_emplace(&entity_system.transforms, main);
    visual  = comp_visual_pool_emplace(&entity_system.visuals_persp, main);
    card_id = comp_card_pool_emplace(&entity_system.cards, main);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    transf->scale        = vec3_create(2, 2, 2);
    transf->synced       = 0;
    visual->vertecies    = card_vertecies;
    visual->texture      = NULL;
    visual->color        = card_color_to_rgb(card->color);

    text   = entity_system_activate_new(&entity_system);
    transf = comp_transform_pool_emplace(&entity_system.transforms, text);
    visual = comp_visual_pool_emplace(&entity_system.visuals_persp, text);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    transf->scale        = vec3_create(0.014f, 0.014f, 0.014f);
    visual->texture      = font_texture;
    visual->color        = card->color == BLACK ? VEC3_ONE : VEC3_ZERO;
    visual->draw_pass    = 1;

    entity_system.parent_map[text] = main;
    entity_system.first_child_map[main] = text;

    switch (card->type) { /* should cache verts */
        case NUMBER:
            snprintf(temp, 8, "[ %i ]", card->num);
            transf->trans        = vec3_create(-0.645, 0, TEXT_Z);
            visual->vertecies    = graphic_vertecies_create_text(font_default, LINE_HEIGHT, temp);
            break;
        case REVERSE:
            transf->trans        = vec3_create(-0.645, 0, TEXT_Z);
            visual->vertecies    = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ <=> ]");
            break;
        case SKIP:
            transf->trans        = vec3_create(-0.645, 0, TEXT_Z);
            visual->vertecies    = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ >> ]");
            break;
        case PLUS2:
            transf->trans        = vec3_create(-0.645, 0, TEXT_Z);
            visual->vertecies    = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ +2 ]");
            break;
        case PICK_COLOR:
            transf->trans        = vec3_create(-0.95, 0, TEXT_Z);
            visual->vertecies    = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ PICK ]");
            break;
        case PLUS4:
            transf->trans        = vec3_create(-0.645, 0, TEXT_Z);
            visual->vertecies    = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ +4 ]");
            break;
        default:
            visual->vertecies = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ ??? ]");
            break;
    }
    transf->synced = 0;

    return main;
}
static void entity_card_destroy(entity_t main)
{LOG("DESTROY CARD CALLED");
    int i;
    entity_t text = entity_system.first_child_map[main];
    graphic_vertecies_destroy(comp_visual_pool_try_get(&entity_system.visuals_persp, text)->vertecies);
    entity_system_erase_via_signature(&entity_system, main, 1<<SIGN_TRANSFORM | 1<<SIGN_VISUAL_PERSP | 1<<SIGN_CARD);
    entity_system_erase_via_signature(&entity_system, text, 1<<SIGN_TRANSFORM | 1<<SIGN_VISUAL_PERSP);

    entity_system.parent_map[main]      = ENTITY_INVALID;
    entity_system.first_child_map[main] = ENTITY_INVALID;
    entity_system.parent_map[text]      = ENTITY_INVALID;
    entity_system_deactivate(&entity_system, main);
    entity_system_deactivate(&entity_system, text);
}

static entity_t entity_players[2];

static void entity_player_add_cards(entity_t player, const struct card *cards, int amount)
{
    const float            DIST = 6.0f;
    entity_t               entity_card, 
                           entity_card_last;
    struct comp_transform *transf;
    int                    i;

    for (i = 0; i < amount; i++) {
        entity_card     = entity_card_create(cards + i);
        transf          = comp_transform_pool_try_get(&entity_system.transforms, entity_card);
        transf->trans   = vec3_create(DIST * i, 0, 0);
        transf->rot     = vec3_create(0, -PI/12, 0);
        transf->synced  = 0;

        entity_system.parent_map[entity_card] = player;
        /*if (i == 0)
            entity_system.first_child_map[player] = entity_card;
        else
            entity_system.sibling_map[entity_card_last] = entity_card;

        entity_card_last = entity_card; */
    }
}
static void entity_player_remove_cards(size_t *card_ids, int amount);

static int resources_init(struct graphic_session *created_session)
{
    unsigned char       buffer[1<<19];
    asset_handle        handle;
    int i;

    session = created_session;
    session_info = graphic_session_info_get(created_session);
    
    /* ortho_height mirrors device height, aspect ratio also mirrors device's */
    aspect_ratio = ((float)session_info.width / session_info.height);
    ortho_height = session_info.height;

    handle = asset_open(ASSET_PATH_FONT);
    asset_read(buffer, 1<<19, handle);
    font_default = create_ascii_baked_font(buffer);
    asset_close(handle);

    font_texture    = graphic_texture_create(font_default.width, font_default.height, font_default.bitmap, 1);
    card_vertecies  = graphic_vertecies_create(CARD_VERTS_RAW, 6);

    perspective  = mat4_perspective(DEG_TO_RAD(FOV_DEG), aspect_ratio, NEAR);
    orthographic = mat4_orthographic(ORTHO_WIDTH, ortho_height, FAR); 

    return 1;
}

static void components_init()
{
    const int COMP_INITIAL      = 32;

    comp_visual_pool_init(&entity_system.visuals_persp, COMP_INITIAL);
    comp_visual_pool_init(&entity_system.visuals_ortho, COMP_INITIAL);
    comp_transform_pool_init(&entity_system.transforms, COMP_INITIAL);
    comp_hitrect_pool_init(&entity_system.hitrects, COMP_INITIAL);
    comp_card_pool_init(&entity_system.cards, COMP_INITIAL);
}
static void objects_init()
{
    struct comp_transform *transf;
    struct comp_visual    *visual;
    struct comp_hitrect   *hitrect;

    entity_players[0] = entity_system_activate_new(&entity_system);
    transf = comp_transform_pool_emplace(&entity_system.transforms, entity_players[0]);
    comp_transform_set_default(transf);
    transf->trans     = vec3_create(-4.5f, -6, -10);
    transf->rot       = vec3_create(-PI/8, 0, 0);
    transf->scale     = vec3_create(0.4, 0.4, 0.4); 
    transf->synced    = 0;
    entity_player_add_cards(entity_players[0], game_state->players[0].hand.elements, game_state->players[0].hand.len);


    entity_players[1] = entity_system_activate_new(&entity_system);
    transf = comp_transform_pool_emplace(&entity_system.transforms, entity_players[1]);
    comp_transform_set_default(transf);
    transf->trans     = vec3_create(-4.5f, 6, -10);
    transf->rot       = vec3_create(PI/8, 0, 0); 
    transf->scale     = vec3_create(0.3, 0.3, 0.3);
    transf->synced    = 0;

    act_button = entity_system_activate_new(&entity_system);
    transf  = comp_transform_pool_emplace(&entity_system.transforms, act_button);
    visual  = comp_visual_pool_emplace(&entity_system.visuals_ortho, act_button);
    hitrect = comp_hitrect_pool_emplace(&entity_system.hitrects, act_button);
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
    hitrect->hitmask    = HITMASK_MOUSE_DOWN;
}

static void render()
{
    graphic_clear(clear_color.x, clear_color.y, clear_color.z);

    comp_visual_transform_pool_draw(&entity_system.visuals_persp, &entity_system.transforms, &perspective);
    comp_visual_transform_pool_draw(&entity_system.visuals_ortho, &entity_system.transforms, &orthographic);

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
    components_init();
    objects_init();

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

    struct comp_hitrect *hitrect;

    comp_hitrect_transform_pool_update_hitstate(&entity_system.hitrects, &entity_system.transforms, &mouse_orthospace, &mouse_camspace_ray, mask);

    hitrect = comp_hitrect_pool_try_get(&entity_system.hitrects, act_button);
    if (hitrect->hitstate) {
        entity_card_destroy(entity_system.cards.dense[0]);
        hitrect->hitstate = 0;
    }

}
void game_update()
{
    comp_transform_pool_sync_matrices(&entity_system.transforms, entity_system.parent_map);
    if (!session)
        return;
    render();
}

