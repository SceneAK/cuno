#include "system/logging.h"
#include "system/graphic/graphic.h"
#include "system/graphic/matrix.h"
#include "system/graphic/text.h"
#include "system/asset_management.h"
#include "main.h"
#include "object.h"
#include <stdio.h>
#include <math.h>

static struct graphic_session *session;
struct graphic_session_info session_info;

static struct baked_font font;
static mat4 perspective;
static const float NEAR = 0.2f;

static struct object card;
static struct object triangle;
static struct object text[3];

void init_objects()
{
    struct graphic_texture *font_tex;

    font_tex = graphic_texture_create(font.width, font.height, font.bitmap);

    const box3D card_bounds = {
        -1.0f, -1.5f, 0.0f,
         1.0f,  1.5f, 0.0f,
    };
    const float card_verts[] = {
        -1.0f,  1.5f, 0.0f, 0.0f, 0.0f,
        -1.0f, -1.5f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.5f, 0.0f, 0.0f, 0.0f,
    
        -1.0,   1.5f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.5f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.5f, 0.0f, 0.0f, 0.0f
    };
    card.id = 0;
    card.vertecies = graphic_vertecies_create(card_verts, 6);
    card.scale = vec3_create(2, 2, 2);
    card.box_bounds = card_bounds;

    const float triangle_verts[] = {
        -1.f,  1.5f, 0.0f, 0.0f, 0.0f,
        -1.f, -1.5f, 0.0f, 0.0f, 0.0f,
         1.f, -1.5f, 0.0f, 0.0f, 0.0f
    };
    triangle.id = 1;
    triangle.vertecies = graphic_vertecies_create(triangle_verts, 3);
    triangle.scale = vec3_create(2, 2, 1);

    text[0].id = 2;
    text[0].vertecies = graphic_vertecies_create_text(font, "Konoha's state of the world");
    text[0].texture = font_tex;
    text[0].trans = vec3_create(-5, 0, -10);
    text[0].scale = vec3_create( 0.02, 0.02, 1);
    text[0].model = mat4_model_object(&text[0]);

    text[1].id = 3;
    text[1].vertecies = graphic_vertecies_create_text(font, "We're invited to a lifelong party!");
    text[1].texture = font_tex;
    text[1].trans = vec3_create(-5,-0.6, -10);
    text[1].scale = vec3_create( 0.012, 0.012, 1);
    text[1].model = mat4_model_object(&text[1]);

    text[2].id = 4;
    text[2].texture = font_tex;
    text[2].trans = vec3_create(-5,-2, -10);
    text[2].scale = vec3_create( 0.01, 0.01, 1);
    text[2].model = mat4_model_object(&text[2]);
}

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
int game_init(struct graphic_session *created_session)
{
    float aspect_ratio;

    session = created_session;
    session_info = graphic_session_info_get(created_session);
    aspect_ratio = (float)session_info.width / session_info.height;

    perspective = mat4_perspective(DEG_TO_RAD(90), aspect_ratio, NEAR);
    if (!load_ascii_font(&font)) {
        LOG("ERR: Failed to load font");
        return -1;
    } 
    init_objects();
    return 0;
}

void object_oscillate(struct object *object, float step, int osc_y)
{
    object->rot.x = step;
    object->rot.y = step/2;
    object->rot.z = step/8;
    if (osc_y)
        object->trans.y = sin(step/4)*7;
    else
        object->trans.x = sin(step/3)*4;

    object->trans.z = -10 - sin(step/6);
    object->model = mat4_model_object(object);
    object->model = mat4_model_object(object);
}

static float step_fast;
static float step_slow;
static vec3 clear_color = { 1, 1, 1 };
void render()
{
    int i;

    step_fast += 0.1f;
    step_slow += 0.03f;

    object_oscillate(&card, step_fast, 0);
    card.model_inv = mat4_invert(card.model);

    object_oscillate(&triangle, step_slow, 1);

    graphic_clear(clear_color.x, clear_color.y, clear_color.z);

    graphic_draw_object(&card, perspective);
    graphic_draw_object(&triangle, perspective);
    for (i = 0; i < 3; i++)
        graphic_draw_object(&text[i], perspective);

    graphic_render(session);
}

void game_update()
{
    if (session)
        render();
}

void game_mouse_event(struct mouse_event event)
{
    char strbuf[64];

    if (text[2].vertecies)
        graphic_vertecies_destroy(text[2].vertecies);
    snprintf(strbuf, sizeof(strbuf), "It's, rude to overstay. (%.2f, %.2f)", event.mouse_x, event.mouse_y);
    text[2].vertecies = graphic_vertecies_create_text(font, strbuf);
}
