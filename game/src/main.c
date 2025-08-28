#include "system/graphic/graphic.h"
#include "system/graphic/text.h"
#include "system/asset_management.h"
#include "system/logging.h"
#include "coord_util.h"
#include "game_math.h"
#include "object.h"
#include "main.h"
#include "game.h"

#define ASPECT_RATIO ((float)session_info.width / session_info.height)

/* RESOURCES */
static struct graphic_session      *session;
static struct graphic_session_info  session_info;
static struct baked_font            font;
static struct graphic_texture      *font_tex_shr;

static int                          obj_count;
static mat4                         perspective;

static struct game_state            game_state;

/* GLOBAL CONST */
static const float NEAR = 0.2f;
static const float FOV_DEG = 90;

/* OBJECTS */
static struct object cursor;
static struct object card;
static struct object ortho_button;

void init_objects()
{
    int i, dist = 0;
    font_tex_shr = graphic_texture_create(font.width, font.height, font.bitmap);

    const rect2D card_bounds = {
        -1.0f, -1.5f,
         1.0f,  1.5f,
    };
    const float card_verts[] = {
        -1.0f,  1.5f, 0.0f, 0.0f, 0.0f,
        -1.0f, -1.5f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.5f, 0.0f, 0.0f, 0.0f,

        -1.0,   1.5f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.5f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.5f, 0.0f, 0.0f, 0.0f
    };
    card.id = obj_count++;
    card.vertecies      = graphic_vertecies_create(card_verts, 6);
    card.trans          = vec3_create(0, 0, -10);
    card.rot            = VEC3_ZERO;
    card.scale          = vec3_create(2.2, 2.2, 2.2);

    cursor.id = obj_count++;
    cursor.vertecies    = card.vertecies;
    cursor.trans        = VEC3_ZERO;
    cursor.rot          = vec3_create(0, 0, PI/4);
    cursor.scale        = vec3_create(0.08, 0.05, 1);

    ortho_button.id = obj_count++;
    ortho_button.rect_bounds    = card_bounds;
    ortho_button.vertecies      = card.vertecies;
    ortho_button.trans          = VEC3_ZERO;
    ortho_button.rot            = VEC3_ZERO;
    ortho_button.scale          = vec3_create(0.15, 0.1, 1);
    ortho_button.model          = mat4_model_object(&ortho_button);
    ortho_button.model_inv      = mat4_invert(ortho_button.model);
}

int load_resources()
{
    unsigned char   buffer[1<<19];
    asset_handle    handle;

    handle = asset_open(ASSET_PATH_FONT);
    asset_read(buffer, 1<<19, handle);
    font = create_ascii_baked_font(buffer);
    asset_close(handle);

    return 1;
}

int game_init(struct graphic_session *created_session)
{
    const int player_amount = 2;
    session = created_session;
    session_info = graphic_session_info_get(created_session);

    if (!load_resources())
        return -1;

    perspective = mat4_perspective(DEG_TO_RAD(FOV_DEG), ASPECT_RATIO, NEAR);
    init_objects();

    game_state_init(&game_state, player_amount);
    deal_cards(&game_state);

    return 0;
}

static vec3 clear_color = { 1, 1, 1 };
void render()
{
    int i;

    graphic_clear(clear_color.x, clear_color.y, clear_color.z);

    graphic_draw_object(&cursor, perspective);
    graphic_draw_object(&ortho_button, MAT4_IDENTITY);

    graphic_render(session);
}

static int turn_count = 0;
void game_mouse_event(struct mouse_event event)
{
    vec2 mouse_ndc = screen_to_ndc(session_info.width, session_info.height, event.mouse_x, event.mouse_y);
    vec3 mouse_camspace = { 0, 0, 0 };
    int return_code; 

    if (!session)
        return;

    mouse_camspace = ndc_to_camspace(ASPECT_RATIO, DEG_TO_RAD(FOV_DEG), mouse_ndc, -10);

    cursor.trans = mouse_camspace;
    cursor.model = mat4_model_object(&cursor);

    if (event.type == MOUSE_DOWN && ndc_on_ortho_bounds(&ortho_button, mouse_ndc.x, mouse_ndc.y)) {
        LOG(" ");
        LOGF(" == TURN %i == ", ++turn_count);
        return_code = supersmartAI_act(&game_state);
        if (return_code < 0) {
            LOGF("! supersmartAI err: %i", return_code);
            return;
        }else
            LOGF("supersmartAI move: %s", return_code == 1 ? "Draw" : (return_code == 2 ? "Batsu" : "Play"));
        LOG(" ");
        LOG("logging game_state");
        log_game_state(&game_state);
        end_turn(&game_state);
        LOG(" == END TURN == ");
        LOG(" ");
        LOG(" ");
    }
}

void game_update()
{
    if (game_state.ended)
        exit(EXIT_SUCCESS);

    if (!session)
        return;
    render();
}
