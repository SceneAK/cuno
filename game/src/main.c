#include <stdio.h>
#include "system/logging.h"
#include "system/graphic/graphic.h"
#include "system/graphic/text.h"
#include "system/asset_management.h"
#include "game_math.h"
#include "game_logic.h"
#include "component.h"
#include "utils.h"
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

/* RESOURCES - COMPONENT POOLS */
static struct transform_pool   transform_pool;
static struct visual_pool      visual_pool_ortho, visual_pool_persp;

/* OBJECTS */
struct object {
    struct visual     *visual;
    struct transform  *transf;
};
struct object_interactable {
    struct visual     *visual;
    struct transform  *transf;
    struct hitrect     hitrect;
};
static struct object_interactable interactable;
struct object_card {
    size_t          card_id;
    struct object   main;
    struct object   text;
};
static void object_card_init(struct object_card *obj_card, const struct card *card)
{
    char temp[8];
    const float LINE_HEIGHT = 0.5f;

    obj_card->card_id = card->id;

    obj_card->main.transf = transform_pool_request(&transform_pool);
    obj_card->main.visual = visual_pool_request(&visual_pool_persp);
    transform_set_default(obj_card->main.transf);
    obj_card->main.visual->vertecies    = card_vertecies;
    obj_card->main.visual->texture      = NULL;
    obj_card->main.visual->color        = card_color_to_rgb(card->color);

    obj_card->text.visual = visual_pool_request(&visual_pool_persp);
    obj_card->text.transf = transform_pool_request(&transform_pool);
    obj_card->text.visual->texture      = font_texture;
    obj_card->text.transf->scale        = vec3_create(0.5f, 0.5f, 0.5f);
    obj_card->text.transf->parent       = obj_card->main.transf;

    switch (card->type) { /* should cache verts */
        case NUMBER:
            snprintf(temp, 8, "[ %i ]", card->num);
            obj_card->text.visual->vertecies    = graphic_vertecies_create_text(font_default, LINE_HEIGHT, temp);
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

    obj_card->main.transf->synced = 0;
    obj_card->text.transf->synced = 0;
}
static void object_card_cleanup(struct object_card *obj_card)
{
    transform_pool_return(&transform_pool, obj_card->main.transf);
    visual_pool_return(&visual_pool_persp, obj_card->main.visual);

    transform_pool_return(&transform_pool, obj_card->text.transf);
    graphic_vertecies_destroy(obj_card->text.visual->vertecies);
    visual_pool_return(&visual_pool_persp, obj_card->text.visual);
}

DEFINE_ARRAY_LIST_WRAPPER(static, struct object_card, object_card_list)
struct object_player {
    struct transform       *transf;
    struct object_card_list objcard_list;
    float                   objcard_dist;
};
static struct object_player object_players[2];

static void object_player_add_card(struct object_player *pv, struct card *cards, int amount)
{
    struct object_card *appended;
    struct transform   *main_transf;
    int i;

    pv->objcard_dist = 0.3f;

    appended = object_card_list_append_empty(&pv->objcard_list, amount);
    for (i = 0; i < amount; i++) {
        /* main_transf          = object_card_init(appended + i, cards + i); */
        main_transf->parent     = pv->transf;
        main_transf->synced     = 0;
    }
}
static void object_player_remove_card(size_t *card_ids, int amount);

static int resources_init(struct graphic_session *created_session)
{
    unsigned char       buffer[1<<19];
    asset_handle        handle;

    session = created_session;
    session_info = graphic_session_info_get(created_session);
    
    /* ortho_height mirrors device height, aspect ratio also mirrors device's */
    aspect_ratio = ((float)session_info.width / session_info.height);
    ortho_height = session_info.height;

    handle = asset_open(ASSET_PATH_FONT);
    asset_read(buffer, 1<<19, handle);
    font_default = create_ascii_baked_font(buffer);
    asset_close(handle);

    font_texture    = graphic_texture_create(font_default.width, font_default.height, font_default.bitmap);
    card_vertecies  = graphic_vertecies_create(CARD_VERTS_RAW, 6);

    perspective  = mat4_perspective(DEG_TO_RAD(FOV_DEG), aspect_ratio, NEAR);
    orthographic = mat4_orthographic(ORTHO_WIDTH, ortho_height, FAR); 

    return 1;
}

static void objects_init()
{
/*  object_players[0].transf = transform_pool_request(&transform_pool);
    transform_set_default(object_players[0].transf);

    object_players[0].transf->trans     = vec3_create(0, 7, -10);  
    object_players[0].transf->rot       = vec3_create(-PI/8, 0, 0); 
    object_players[0].transf->synced    = 0;


    object_players[1].transf = transform_pool_request(&transform_pool);
    transform_set_default(object_players[1].transf);

    object_players[1].transf->trans     = vec3_create(0, -7, -10); 
    object_players[1].transf->rot       = vec3_create(PI/8, 0, 0); 
    object_players[1].transf->synced    = 0; */


    interactable.transf  = transform_pool_request(&transform_pool);
    interactable.visual  = visual_pool_request(&visual_pool_ortho);
    transform_set_default(interactable.transf);
    visual_set_default(interactable.visual);
    hitrect_set_default(&interactable.hitrect);

    interactable.transf->trans          = vec3_create(-80, 80, -1);
    interactable.transf->rot            = vec3_create(0, 0, PI/16);
    interactable.transf->scale          = vec3_create(100, 100, 1);
    interactable.transf->synced         = 0;

    interactable.visual->vertecies      = card_vertecies;
    interactable.visual->color          = vec3_create(0, 0.5, 1);
    interactable.visual->model          = &interactable.transf->matrix;

    interactable.hitrect.transf         = interactable.transf;
    interactable.hitrect.rect           = CARD_BOUNDS;
    interactable.hitrect.rect_type      = RECT_ORTHOSPACE;
}

static void render()
{
    graphic_clear(clear_color.x, clear_color.y, clear_color.z);

    visual_pool_draw(&visual_pool_persp, &perspective);
    visual_pool_draw(&visual_pool_ortho, &orthographic);

    graphic_render(session);
}

/* GAME EVENTS */
int game_init(struct graphic_session *created_session)
{
    const int PLAYER_AMOUNT     = 2;
    const int DEAL_PER_PLAYER   = 4;
    const int COMP_INITIAL      = 16;

    if (!resources_init(created_session))
        return -1;

    game_state_init(&game_state_mut, PLAYER_AMOUNT);
    deal_cards(&game_state_mut, DEAL_PER_PLAYER);

    visual_pool_init(&visual_pool_persp, COMP_INITIAL);
    visual_pool_init(&visual_pool_ortho, COMP_INITIAL);
    transform_pool_init(&transform_pool, COMP_INITIAL);

    objects_init();

    return 0;
}

void game_mouse_event(struct mouse_event event)
{
    vec2 mouse_ndc          = screen_to_ndc(session_info.width, session_info.height, event.mouse_x, event.mouse_y);
    vec3 mouse_camspace_ray = ndc_to_camspace(aspect_ratio, DEG_TO_RAD(FOV_DEG), mouse_ndc, -10);
    vec2 mouse_orthospace   = ndc_to_orthospace(ORTHO_WIDTH, ortho_height, mouse_ndc);

    if (event.type == MOUSE_DOWN && hitrect_check_hit(&interactable.hitrect, &mouse_orthospace, &mouse_camspace_ray))
        clear_color.x = clear_color.x ? 0 : 1;
}
void game_update()
{
    transform_pool_sync_matrix(&transform_pool);

    if (!session)
        return;
    render();
}
