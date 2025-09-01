#include "system/graphic/graphic.h"
#include "system/graphic/text.h"
#include "system/asset_management.h"
#include "coord_util.h"
#include "game_math.h"
#include "game_logic.h"
#include "object.h"
#include "main.h"

#define ASPECT_RATIO ((float)session_info.width / session_info.height)

/* RESOURCES */
static struct graphic_session      *session;
static struct graphic_session_info  session_info;
static struct graphic_texture      *font_texture;
static struct graphic_vertecies    *card_vertecies;

const rect2D                        CARD_BOUNDS = { -1.0f, -1.5f,   1.0f,  1.5f };
static int                          obj_count;
static mat4                         perspective;

static struct game_state            game_state;

/* GLOBAL CONST */
static const float NEAR = 0.2f;
static const float FOV_DEG = 90;

/* OBJECTS */
struct player_visual {
    int                     player_index;
    struct comp_transform   transform;
    struct object          *cards;
    size_t                  cards_len;
};
static struct player_visual player_visuals[2];
void draw_new_card_object(struct player_visual *player_visual, struct card *card)
{

}

int resources_init()
{
    unsigned char       buffer[1<<19];
    asset_handle        handle;
    struct baked_font   font;

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
    font = create_ascii_baked_font(buffer);
    asset_close(handle);

    font_texture = graphic_texture_create(font.width, font.height, font.bitmap);
    card_vertecies = graphic_vertecies_create(card_verts_raw, 6);

    perspective = mat4_perspective(DEG_TO_RAD(FOV_DEG), ASPECT_RATIO, NEAR);

    return 1;
}

void objects_init()
{
    player_visuals[0].transform.trans   = vec3_create(0, 7, -10);  
    player_visuals[0].transform.rot     = vec3_create(-PI/8, 0, 0); 
    player_visuals[0].transform.scale   = VEC3_ONE;
    player_visuals[0].transform.model   = mat4_model_transform(&player_visuals[0].transform);

    player_visuals[1].transform.trans   = vec3_create(0, -7, -10); 
    player_visuals[1].transform.rot     = vec3_create(PI/8, 0, 0); 
    player_visuals[1].transform.scale   = VEC3_ONE;
    player_visuals[1].transform.model   = mat4_model_transform(&player_visuals[1].transform);
}

int game_init(struct graphic_session *created_session)
{
    const int player_amount = 2;
    session = created_session;
    session_info = graphic_session_info_get(created_session);

    game_state_init(&game_state, player_amount);
    deal_cards(&game_state);

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

static int turn_count = 0;
void game_update()
{
    if (game_state.ended || turn_count > 10)
        exit(EXIT_SUCCESS);

    LOG("");
    LOG("");
    LOGF(" == TURN %i ==", turn_count++);
    LOGF("SUPERSMARTAI returns: %i", supersmartAI_act(&game_state));
    log_game_state(&game_state);
    end_turn(&game_state);
    LOG("");
    LOG("");

    if (!session)
        return;
    render();
}
