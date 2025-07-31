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
    *font = create_ascii_baked_font(ttf);
    asset_close(ttf);
    return 1;
}

static struct graphic_session  *session;

static struct baked_font font;
static struct graphic_texture *font_tex;

static struct graphic_draw_ctx *ctx_card;
static struct graphic_draw_ctx *ctx_triangle;
static struct graphic_draw_ctx *ctx_text = NULL;

int init(struct graphic_session *created)
{
    LOG("INIT");
    session = created;

    ctx_card = graphic_draw_ctx_create(card_verts, 6, NULL);
    ctx_triangle = graphic_draw_ctx_create(triangle_verts, 3, NULL);

    if (!load_ascii_font(&font)) {
        LOG("ERR: Failed to loat font");
        return -1;
    } 

    font_tex = graphic_texture_create(font.width, font.height, font.bitmap);
    ctx_text = graphic_draw_ctx_create_text(font, font_tex, "The Yag Noah");

    return 0;
}

mat4 oscillate_model(float step)
{
    mat4 model;
    vec3 pos = { 0, sin(step/3)/3, sin(step/6)/4 };
    vec3 scale = { 2.0f, 2.0f, 2.0f };

    model = mat4_scale(scale);
    model = mat4_mult(model, mat4_trans(pos));
    model = mat4_mult(model, mat4_rotz(step));
    model = mat4_mult(model, mat4_roty(step/3));
    model = mat4_mult(model, mat4_rotx(step/9));
    return model;
}

static float step_card;
static float step_triangle;
void render()
{
    vec3 text_scale = {1.0/100000, 1.0/100000, 1};

    mat4 model_card = oscillate_model(step_card);
    mat4 model_triangle = oscillate_model(step_triangle);
    mat4 model_text = mat4_scale(text_scale);

    graphic_clear(1, 1, 1);

    graphic_draw(ctx_card, model_card);
    graphic_draw(ctx_triangle, model_triangle);
    if (ctx_text)
        graphic_draw(ctx_text, model_text);

    graphic_render(session);

    step_card += 0.1f;
    step_triangle += 0.03f;
}

void update()
{
    /* I expect to have control over position, textures, model, etc */
    if (session)
        render();
}
