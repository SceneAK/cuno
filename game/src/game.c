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
const float                             CARD_SELECT_RAISE = 5.0f;
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

static char                             global_game_log_buff[4096];
static card_id_t                        global_card_ids[ENTITY_MAX] = {(card_id_t)-1};

/******* COMPONENT SYSTEMS *******/
static struct entity_record             entity_record;
static struct comp_system_family       *sys_fam;
static struct comp_system_transform    *sys_transf;
static struct comp_system_visual       *sys_visual;
static struct comp_system_hitrect      *sys_hitrect;
static struct comp_system_interpolator *sys_interp;

/******* TRACKED ENTITIES *******/
static entity_t                         entity_draw_button;
static struct entity_list               entity_discards;
static struct entity_list               entity_selects;
static entity_t                         entity_players[2];
static entity_t                         entity_debug_text;

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

static void select_entity_card(entity_t entity_card)
{
    struct transform target;

    if (!entity_selects.len)
        game_state_copy(&game_state_alt, game_state);

    if (!game_state_can_act_play(&game_state_alt, global_card_ids[entity_card])) {
        return;
    }

    game_state_act_play(&game_state_alt, global_card_ids[entity_card]);
    entity_list_append(&entity_selects, &entity_card, 1);

    target = comp_system_transform_get(sys_transf, entity_card)->data;
    target.trans.y += CARD_SELECT_RAISE;
    comp_system_interpolator_start(sys_interp, entity_card, target);
}
static void deselect_entity_card(size_t index)
{
    struct transform    target;
    entity_t            entity_card;
    size_t              i;

    entity_card = entity_selects.elements[index];

    game_state_copy(&game_state_alt, game_state);
    for (i = 0; i < entity_selects.len; i++) {
        if (i == index)
            continue;

        if (!game_state_can_act_play(&game_state_alt, global_card_ids[entity_selects.elements[i]]))
            break;
    }
    if (i == entity_selects.len) {
        entity_list_remove_sft(&entity_selects, &index, 1);
        target = comp_system_transform_get(sys_transf, entity_card)->data;
        target.trans.y -= CARD_SELECT_RAISE;
        comp_system_interpolator_start(sys_interp, entity_card, target);
    }

    game_state_copy(&game_state_alt, game_state);
    for (i = 0; i < entity_selects.len; i++)
        game_state_act_play(&game_state_alt, global_card_ids[entity_selects.elements[i]]);
}
static void toggle_select_entity_card(entity_t entity_card)
{
    size_t i;

    for (i = 0; i < entity_selects.len; i++) 
        if (entity_card == entity_selects.elements[i]) break;

    comp_system_interpolator_finish(sys_interp, entity_card);
    if (i == entity_selects.len) {
        select_entity_card(entity_card);
    } else {
        deselect_entity_card(i);
    }
}
static void entity_selects_play()
{
    int i;

    /* or just game_state_copy the alt  */
    for (i = 0; i < entity_selects.len; i++)
        game_state_act_play(&game_state_mut, global_card_ids[entity_selects.elements[i]]);
}
static void entity_selects_reset()
{
    int i;
    for (i = 0; i < entity_selects.len; i++)
        deselect_entity_card(i);
    entity_list_clear(&entity_selects);
}
void arrange_discards()
{
    const float LAYER_DIST = 0.5f;
    struct transform target = DISCARD_PILE_TRANSFORM;
    int i;

    LOG("Arrange Discards");
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
    hitrect->type           = RECT_CAMSPACE;
    hitrect->hitmask        = HITMASK_MOUSE_DOWN;
    visual->vertecies       = card_vertecies;
    visual->texture         = NULL;
    visual->color           = card_color_to_rgb(card->color);
    interp->opt.ease_in     = 0.1f;
    interp->opt.ease_out    = 0.2f;
    global_card_ids[main]   = card->id;

    text                    = entity_record_activate(&entity_record);
    transf                  = comp_system_transform_emplace(sys_transf, text);
    visual                  = comp_system_visual_emplace(sys_visual, text, PROJ_PERSP);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    transf->data.scale      = vec3_create(0.014f, 0.014f, 0.014f);
    switch (card->type) { /* should cache verts */
        case NUMBER:
            snprintf(temp, 8, "[ %i ]", card->num);
            transf->data.trans  = vec3_create(-CHAR_WIDTH*2.5, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_default, LINE_HEIGHT, temp);
            break;
        case REVERSE:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*3.5, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ <-> ]");
            break;
        case SKIP:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*3, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ >> ]");
            break;
        case PLUS2:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*3, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ +2 ]");
            break;
        case PICK_COLOR:
            transf->data.trans  = vec3_create(-CHAR_WIDTH*4, 0, TEXT_Z);
            visual->vertecies   = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ PICK ]");
            break;
        case PLUS4:
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
    visual->color        = card->color == BLACK ? VEC3_ONE : VEC3_ZERO;
    visual->draw_pass    = 1;
    comp_system_family_adopt(sys_fam, main, text);

    return main;
}
static void entity_card_destroy(entity_t main)
{
    entity_t text = comp_system_family_view(sys_fam).first_child_map[main];
    LOGF("Destroying entity card %d with child %d", main, text);

    graphic_vertecies_destroy(comp_system_visual_get(sys_visual, text)->vertecies);

    cleanup_recorded_component_flags(main);
    cleanup_recorded_component_flags(text);

    entity_record_deactivate(&entity_record, main);
    entity_record_deactivate(&entity_record, text);
}
static void free_old_discards()
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

void entity_card_discard(entity_t card)
{
    entity_list_append(&entity_discards, &card, 1);
    comp_system_hitrect_get(sys_hitrect, card)->active = 1;
}

static void represent_current_turn()
{/* This relies on entity_player and game_state's player to mirror */
    const struct player            *active_player = game_state->players + game_state->active_player_index;
    entity_t                        entity_player = entity_players[game_state->active_player_index], 
                                    current,
                                    next,
                                    top_card;
    size_t                          entity_card_len;
    card_id_t                       card_id;
    struct comp_system_family_view  family_view;
    int i;

    family_view = comp_system_family_view(sys_fam);

    if (game_state->act == ACT_PLAY) {
        current = family_view.first_child_map[entity_player];
        while (!entity_is_invalid(current)) {
            next = family_view.sibling_map[current];

            card_id = global_card_ids[current];
            if (array_list_index_of(id, &active_player->hand, card_id) == -1) {
                entity_player_remove_card(current);
                if (card_id == game_state->top_card.id)
                    top_card = current;
                else
                    entity_card_discard(current);
            }

            current = next;
        }
        if (top_card != ENTITY_INVALID)
            entity_card_discard(top_card);
        else 
            LOG("Could not find final discard");

        arrange_discards();
    } else if (game_state->act == ACT_DRAW) {
        free_old_discards();

        entity_card_len = comp_system_family_count_children(sys_fam, entity_player);
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
    struct comp_transform   *transf;
    struct comp_visual      *visual;
    struct comp_hitrect     *hitrect;

    entity_t temp;

    entity_players[0]   = entity_player_create(game_state->players + 0);
    transf              = comp_system_transform_get(sys_transf, entity_players[0]);
    transf->data.trans  = vec3_create(-4.8f, -5.5, -10);
    transf->data.rot    = vec3_create(-PI/8, 0, 0);
    transf->data.scale  = vec3_create(0.4, 0.4, 0.4); 
    transf->synced      = 0;

    entity_players[1]   = entity_player_create(game_state->players + 1);
    transf              = comp_system_transform_get(sys_transf, entity_players[1]);
    transf->data.trans  = vec3_create(-4.8f, 5.5, -10);
    transf->data.rot    = vec3_create(PI/8, 0, 0); 
    transf->data.scale  = vec3_create(0.3, 0.3, 0.3);
    transf->synced      = 0;

    entity_list_init(&entity_discards, 1);
    temp = entity_card_create(&game_state->top_card);
    entity_list_append(&entity_discards, &temp, 1);
    comp_system_interpolator_change(sys_interp, temp, DISCARD_PILE_TRANSFORM);

    entity_list_init(&entity_selects, 1);

    entity_draw_button  = entity_record_activate(&entity_record);
    transf              = comp_system_transform_emplace(sys_transf, entity_draw_button);
    visual              = comp_system_visual_emplace(sys_visual, entity_draw_button, PROJ_ORTHO);
    hitrect             = comp_system_hitrect_emplace(sys_hitrect, entity_draw_button);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    comp_hitrect_set_default(hitrect);
    transf->data.trans  = vec3_create(500, -900, -1);
    transf->data.rot    = vec3_create(0, 0, 0);
    transf->data.scale  = vec3_create(80, 40, 1);
    transf->synced      = 0;
    visual->vertecies   = card_vertecies;
    visual->color       = vec3_create(1, 0.321568627, 0.760784314);
    hitrect->rect       = CARD_BOUNDS;
    hitrect->type       = RECT_ORTHOSPACE;
    hitrect->hitmask    = HITMASK_MOUSE_DOWN;

    entity_debug_text   = entity_record_activate(&entity_record);
    transf              = comp_system_transform_emplace(sys_transf, entity_debug_text);
    visual              = comp_system_visual_emplace(sys_visual, entity_debug_text, PROJ_ORTHO);
    comp_transform_set_default(transf);
    comp_visual_set_default(visual);
    transf->data.trans  = vec3_create(-500, 200, -1);
    transf->data.scale  = vec3_create(0.6, 0.6, 0.6);
    transf->synced      = 0;
    visual->texture     = font_texture;
    visual->color       = VEC3_ZERO;
    visual->draw_pass   = 1;
}

static void render()
{
    graphic_clear(clear_color.x, clear_color.y, clear_color.z);
    comp_system_visual_draw(sys_visual, &perspective, &orthographic);
    graphic_render(session);
}

static void handle_hitrect_state_update()
{
    struct comp_system_family_view  view;
    entity_t                        top_card,
                                    head;

    if (game_state->active_player_index != 0)
        return;

    view    = comp_system_family_view(sys_fam);
    head    = view.first_child_map[entity_players[0]];
    for (;!entity_is_invalid(head); head = view.sibling_map[head]) {
        if (!comp_system_hitrect_check_clear(sys_hitrect, head))
            continue;
        toggle_select_entity_card(head);
    }

    top_card = entity_discards.elements[entity_discards.len-1];
    if (comp_system_hitrect_check_clear(sys_hitrect, top_card)) {
        entity_selects_play();
        entity_selects_reset();
        represent_current_turn();
        game_state_end_turn(&game_state_mut);
    }

    if (comp_system_hitrect_check_clear(sys_hitrect, entity_draw_button)) {
        game_state_act_draw(&game_state_mut);
        entity_selects_reset();
        represent_current_turn();
        game_state_end_turn(&game_state_mut);
    }
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
        represent_current_turn();
        entity_selects_reset();
        game_state_end_turn(&game_state_mut);
        auto_act_time = -1;
    }

}

/***** GAME EVENTS *****/
int game_init(struct graphic_session *created_session)
{
    const int PLAYER_AMOUNT     = 2;
    const int DEAL_PER_PLAYER   = 4;
    const unsigned int seed = get_monotonic_time();
    
    LOG("GAME_INIT");

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
    int i;

    comp_system_hitrect_update(sys_hitrect, &mouse_orthospace, &mouse_camspace_ray, mask);
    handle_hitrect_state_update();
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
