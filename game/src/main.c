#include <stdio.h>
#include "system/graphic/graphic.h"
#include "system/graphic/text.h"
#include "system/asset_management.h"
#include "coord_util.h"
#include "game_math.h"
#include "game_logic.h"
#include "component.h"
#include "utils.h"
#include "main.h"

#define ASPECT_RATIO ((float)session_info.width / session_info.height)

/* RESOURCES */
static struct baked_font font_default;

static struct graphic_session      *session;
static struct graphic_session_info  session_info;
static struct graphic_texture      *font_texture;
static struct graphic_vertecies    *card_vertecies;

const rect2D                        CARD_BOUNDS = { -1.0f, -1.5f,   1.0f,  1.5f };
static mat4                         perspective;

static struct game_state            game_state_mut;
static const struct game_state     *game_state = &game_state_mut;

/* GLOBAL CONST */
static const float NEAR = 0.2f;
static const float FOV_DEG = 90;

/* COMPONENT POOLS */
static struct comp_transform_pool *comp_transform_pool;
static struct comp_visual_pool *comp_visual_pool;

/* OBJECTS */
struct object {
    struct comp_visual     *visual;
    struct comp_transform  *transf;
};
static struct object        ortho_button;
static size_t               objects_len;

struct object_card {
    size_t          card_id;
    struct object   main;
    struct object   text;
};
void object_card_init(struct object_card *obj_card, const struct card *card)
{
    char temp[8];
    const float LINE_HEIGHT = 0.5;

    obj_card->card_id = card->id;

    obj_card->main.visual = comp_visual_pool_request(comp_visual_pool);
    obj_card->main.visual->vertecies    = card_vertecies;
    obj_card->main.visual->texture      = NULL;
    obj_card->main.visual->color        = card_color_to_rgb(card->color);

    obj_card->text.visual = comp_visual_pool_request(comp_visual_pool);
    obj_card->text.visual->texture      = font_texture;
    obj_card->text.transf = comp_transform_pool_request(comp_transform_pool);
    obj_card->text.transf->scale        = vec3_create(0.5f, 0.5f, 0.5f);
    obj_card->text.transf->parent       = obj_card->main.transf;

    switch (card->type) { /* should cache verts */
        case NUMBER:
            snprintf(temp, 8, "[ %i ]", card->num);
            obj_card->text.visual->vertecies = graphic_vertecies_create_text(font_default, LINE_HEIGHT, temp);
            break;
        case REVERSE:
            obj_card->text.transf->trans        = vec3_create(-0.05, 0, 0);
            obj_card->text.visual->vertecies    = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ <=> ]");
            break;
        case SKIP:
            obj_card->text.transf->trans        = vec3_create(-0.03, 0, 0);
            obj_card->text.visual->vertecies    = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ >> ]");
            break;
        case PLUS2:
            obj_card->text.transf->trans        = vec3_create(-0.03, 0, 0);
            obj_card->text.visual->vertecies    = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ +2 ]");
            break;
        case PICK_COLOR:
            obj_card->text.transf->trans        = vec3_create(-0.1, 0, 0);
            obj_card->text.visual->vertecies    = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ PICK_COLOR ]");
            break;
        case PLUS4:
            obj_card->text.transf->trans        = vec3_create(-0.03, 0, 0);
            obj_card->text.visual->vertecies    = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ +4 ]");
            break;
        default:
            obj_card->text.visual->vertecies = graphic_vertecies_create_text(font_default, LINE_HEIGHT, "[ ??? ]");
            break;
    }

    obj_card->main.transf->desynced = 1;
    obj_card->text.transf->desynced = 1;
}
void object_card_cleanup(struct object_card *obj_card)
{
    comp_transform_pool_return(comp_transform_pool, obj_card->main.transf);
    comp_visual_pool_return(comp_visual_pool, obj_card->main.visual);

    comp_transform_pool_return(comp_transform_pool, obj_card->text.transf);
    graphic_vertecies_destroy(obj_card->text.visual->vertecies);
    comp_visual_pool_return(comp_visual_pool, obj_card->text.visual);
}

DEFINE_ARRAY_LIST(struct object_card, object_card_list)
struct player_visual {
    int                     player_index;
    struct comp_transform  *transf;
    struct object_card_list objcard_list;
};
static struct player_visual player_visuals[2];

const float CARD_DIST = 0.3f;
void player_visual_add_card(struct player_visual *pv, struct card *cards, int amount)
{
    struct object_card *appended;
    struct comp_transform *main_transf;
    int i;

    object_card_list_append_empty(&pv->objcard_list, amount);
    appended = pv->objcard_list.elements + (pv->objcard_list.len - amount);
    for (i = 0; i < amount; i++) {
        /* main_transf             = object_card_init(appended + i, cards + i); */
        main_transf->parent     = pv->transf;
        main_transf->desynced   = 1;
    }
}
void player_visual_remove_card(size_t *card_ids, int amount)
{

}

int resources_init()
{
    unsigned char       buffer[1<<19];
    asset_handle        handle;

    const float card_verts_raw[] = {
        -1.0f,  1.5f, 0.0f, 0.0f, 0.0f,
        -1.0f, -1.5f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.5f, 0.0f, 0.0f, 0.0f,

        -1.0,   1.5f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.5f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.5f, 0.0f, 0.0f, 0.0f
    };

    handle = asset_open(ASSET_PATH_FONT);
    asset_read(buffer, 1<<19, handle);
    font_default = create_ascii_baked_font(buffer);
    asset_close(handle);

    font_texture = graphic_texture_create(font_default.width, font_default.height, font_default.bitmap);
    card_vertecies = graphic_vertecies_create(card_verts_raw, 6);

    perspective = mat4_perspective(DEG_TO_RAD(FOV_DEG), ASPECT_RATIO, NEAR);

    return 1;
}

void objects_init()
{
    int i, j;

    player_visuals[0].transf->trans      = vec3_create(0, 7, -10);  
    player_visuals[0].transf->rot        = vec3_create(-PI/8, 0, 0); 
    player_visuals[0].transf->desynced   = 1;

    player_visuals[1].transf->trans      = vec3_create(0, -7, -10); 
    player_visuals[1].transf->rot        = vec3_create(PI/8, 0, 0); 
    player_visuals[1].transf->desynced   = 1;

    ortho_button.transf->trans           = VEC3_ZERO;
    ortho_button.transf->rot             = VEC3_ZERO;
    ortho_button.transf->scale           = vec3_create(0.2, 0.2, 1);
    ortho_button.transf->desynced        = 1;
}

int game_init(struct graphic_session *created_session)
{
    const int PLAYER_AMOUNT     = 2;
    const int DEAL_PER_PLAYER   = 4;

    session = created_session;
    session_info = graphic_session_info_get(created_session);

    game_state_init(&game_state_mut, PLAYER_AMOUNT);
    deal_cards(&game_state_mut, DEAL_PER_PLAYER);

    if (!resources_init())
        return -1;
    objects_init();

    return 0;
}

static vec3 clear_color = { 1, 1, 1 };
void render()
{
    int i;

    graphic_clear(clear_color.x, clear_color.y, clear_color.z);

    comp_visual_pool_draw(comp_visual_pool, &perspective);

    graphic_render(session);
}

void game_mouse_event(struct mouse_event event)
{
    vec2 mouse_ndc = screen_to_ndc(session_info.width, session_info.height, event.mouse_x, event.mouse_y);
    vec3 mouse_camspace = { 0, 0, 0 };

    if (!session)
        return;

    mouse_camspace = ndc_to_camspace(ASPECT_RATIO, DEG_TO_RAD(FOV_DEG), mouse_ndc, -10);
}

void game_update()
{
    if (!session)
        return;
    render();
}
