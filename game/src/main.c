#include "system/logging.h"
#include "system/graphic/graphic.h"
#include "system/graphic/matrix.h"
#include "system/graphic/text.h"
#include "system/asset_management.h"
#include "main.h"
#include <math.h>

const float card_verts[] = {
    -0.1f,  0.15f, 0.0f, 0.0f, 0.0f,
    -0.1f, -0.15f, 0.0f, 0.0f, 0.0f,
     0.1f, -0.15f, 0.0f, 0.0f, 0.0f,

    -0.1f,  0.15f, 0.0f, 0.0f, 0.0f,
     0.1f,  0.15f, 0.0f, 0.0f, 0.0f,
     0.1f, -0.15f, 0.0f, 0.0f, 0.0f
};
const float triangle_verts[] = {
    -0.1f,  0.15f, 0.0f, 0.0f, 0.0f,
    -0.1f, -0.15f, 0.0f, 0.0f, 0.0f,
     0.1f, -0.15f, 0.0f, 0.0f, 0.0f
};

int load_ascii_font(struct baked_font *font)
{ 
    unsigned char               ttf_buffer[1<<19];
    asset_handle                ttf;

    ttf = asset_open(ASSET_PATH_FONT);
    asset_read(ttf_buffer, 1<<19, ttf);
    *font = create_ascii_baked_font(ttf_buffer);
    asset_close(ttf);
    return 1;
}

static struct graphic_session  *session;

static struct baked_font font;
static struct graphic_texture *font_tex;

static struct graphic_draw_ctx *ctx_card;
static struct graphic_draw_ctx *ctx_triangle;
static struct graphic_draw_ctx *ctx_text1 = NULL;
static struct graphic_draw_ctx *ctx_text2 = NULL;
static struct graphic_draw_ctx *ctx_text3 = NULL;

int game_init(struct graphic_session *created)
{
    LOG("GAME INIT");
    session = created;

    ctx_card = graphic_draw_ctx_create(card_verts, 6, NULL);
    ctx_triangle = graphic_draw_ctx_create(triangle_verts, 3, NULL);

    if (!load_ascii_font(&font)) {
        LOG("ERR: Failed to loat font");
        return -1;
    } 

    font_tex = graphic_texture_create(font.width, font.height, font.bitmap);
    ctx_text1 = graphic_draw_ctx_create_text(font, font_tex, "Watch me take my steps");
    ctx_text2 = graphic_draw_ctx_create_text(font, font_tex, "La lu lu la... la lu lu la, hunn");
    ctx_text3 = graphic_draw_ctx_create_text(font, font_tex, "coords here");

    return 0;
}

mat4 oscillate_model(float step, int osc_y)
{
    mat4 model;
    vec3 pos = { 0, 0, 0};
    vec3 scale = { 2.0f, 2.0f, 2.0f };

    if (osc_y)
        pos.y = sin(step/4)/1.5;
    else
        pos.x = sin(step/3)/1.5;

    model = mat4_mult(mat4_rotx(step), mat4_scale(scale));
    model = mat4_mult(mat4_roty(step/2), model);
    model = mat4_mult(mat4_rotz(step/8), model);
    model = mat4_mult(mat4_trans(pos), model);
    return model;
}

static float step_card;
static float step_triangle;
static vec3 clear_color = {1, 1, 1};
void render()
{
    vec3 text1_scale = { 0.0045, 0.0045, 1};
    vec3 text2_scale = { 0.0025, 0.0025, 1};
    vec3 text1_trans = {-0.8, 0, 0};
    vec3 text2_trans = {-0.8,-0.1, 0};
    vec3 text3_trans = {-0.9,-0.4, 0};

    mat4 model_card = oscillate_model(step_card, 1);
    mat4 model_triangle = oscillate_model(step_triangle, 0);
    mat4 model_text1 = mat4_mult(mat4_trans(text1_trans), mat4_scale(text1_scale)); 
    mat4 model_text2 = mat4_mult(mat4_trans(text2_trans), mat4_scale(text2_scale));
    mat4 model_text3 = mat4_mult(mat4_trans(text3_trans), mat4_scale(text2_scale));

    graphic_clear(clear_color.x, clear_color.y, clear_color.z);

    graphic_draw(ctx_card, model_card);
    graphic_draw(ctx_triangle, model_triangle);
    graphic_draw(ctx_text1, model_text1);
    graphic_draw(ctx_text2, model_text2);
    graphic_draw(ctx_text3, model_text3);

    graphic_render(session);

    step_card += 0.1f;
    step_triangle += 0.03f;
}

void game_update()
{
    if (session)
        render();
}

void game_mouse_event(struct mouse_event event)
{
    if (event.state == MOUSE_UP)
        clear_color.y = clear_color.y == 1 ? 0 : 1;
    if (event.state == MOUSE_DOWN)
        clear_color.z = clear_color.z == 1 ? 0 : 1;

    LOG("MEVENT:");
    if (event.state == MOUSE_DOWN)
        LOG("MOUSE DOWN");
    if (event.state == MOUSE_UP)
        LOG("MOUSE UP");
    if (event.state == MOUSE_HOLD)
        LOG("MOUSE HELD");
    if (event.state == MOUSE_HOVER)
        LOG("MOUSE HOVER");
}
