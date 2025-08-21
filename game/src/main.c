#include <stdlib.h>
#include "system/graphic/graphic.h"
#include "system/graphic/text.h"
#include "system/asset_management.h"
#include "game_math.h"
#include "coord_util.h"
#include "object.h"
#include "main.h"

#define ASPECT_RATIO ((float)session_info.width / session_info.height)

enum card_type { 
    NUMBER,
    REVERSE,
    SKIP,
    PICK_COLOR,
    PLUS2,
    PLUS4,
    SWAP
};
struct card {
    enum card_type type;
    enum {
        RED,
        GREEN,
        BLUE,
        YELLOW
    } color;
    unsigned short num;
};
struct player {
    short        id;
    char        *name;
    struct card *cards;
    size_t       card_len;
};
struct game_state {
    struct player      *players;
    unsigned short      player_len;
};

/* RESOURCES */
static struct graphic_session      *session;
static struct graphic_session_info  session_info;
static struct baked_font            font;

static int                          obj_count;
static mat4                         perspective;

static struct game_state            game_state;

/* GLOBAL CONST */
static const float NEAR = 0.2f;
static const float FOV_DEG = 90;

/* OBJECTS */
static struct object cursor;
static struct object card;

void init_objects()
{
    struct graphic_texture *font_tex;
    font_tex = graphic_texture_create(font.width, font.height, font.bitmap);

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
    card.rect_bounds    = card_bounds;

    cursor.id = obj_count++;
    cursor.vertecies    = card.vertecies;
    cursor.trans        = VEC3_ZERO;
    cursor.rot          = vec3_create(0, 0, PI/4);
    cursor.scale        = vec3_create(0.08, 0.05, 1);
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

void populate_rand_cards(struct card *cards, int amount)
{
    const float RATIO = RAND_MAX / 110;
    int       i, random;

    srand(1);
    for (i = 0; i < amount; i++) {
        random = rand();
        cards[i].color = random % 4;

        if ( random <= RATIO * 76 ) {
            cards[i].type = NUMBER;
            cards[i].num = random % 10;
        }
        else if ( random <= RATIO * 84 )
            cards[i].type = REVERSE;

        else if ( random <= RATIO * 92 )
            cards[i].type = SKIP;

        else if ( random <= RATIO * 100 )
            cards[i].type = PLUS2;

        else if ( random <= RATIO * 104 )
            cards[i].type = PICK_COLOR;

        else if ( random <= RATIO * 108 )
            cards[i].type = PLUS4;

        else if ( random <= RATIO * 110 )
            cards[i].type = SWAP;
    }
}

void game_state_init()
{
    const int deal_per_player = 7;
    int i;

    game_state.player_len = 3;
    game_state.players = malloc( game_state.player_len*sizeof(struct player) );
    
    for (i = 0; i < game_state.player_len; i++) {
        game_state.players[i].id = i;
        game_state.players[i].cards = malloc( 7 * sizeof(struct card) );
        populate_rand_cards(game_state.players[i].cards, deal_per_player);
    }
}
int game_init(struct graphic_session *created_session)
{
    session = created_session;
    session_info = graphic_session_info_get(created_session);

    if (!load_resources())
        return -1;

    perspective = mat4_perspective(DEG_TO_RAD(FOV_DEG), ASPECT_RATIO, NEAR);
    init_objects();

    game_state_init();

    return 1;
}

static vec3 clear_color = { 1, 1, 1 };
void render()
{
    int i;

    graphic_clear(clear_color.x, clear_color.y, clear_color.z);

    graphic_draw_object(&cursor, perspective);
    graphic_draw_object(&card, perspective);

    graphic_render(session);
}

void game_mouse_event(struct mouse_event event)
{
    vec2 mouse_ndc = screen_to_ndc(session_info.width, session_info.height, event.mouse_x, event.mouse_y);
    vec3 mouse_camspace = { 0, 0, 0 };

    if (!session)
        return;

    mouse_camspace = ndc_to_camspace(ASPECT_RATIO, DEG_TO_RAD(FOV_DEG), mouse_ndc, -10);

    cursor.trans = mouse_camspace;
    cursor.model = mat4_model_object(&cursor);
}

void game_update()
{
    if (!session)
        return;

    render();
}
