#include "system/logging.h"
#include "system/graphic/graphic.h"
#include "system/graphic/matrix.h"
#include "system/graphic/text.h"
#include "system/asset_management.h"
#include "main.h"
#include <stdio.h>
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

struct object {
    int id;
    struct graphic_vertecies *vertecies;
    struct graphic_texture *texture;
    vec3 trans, rot, scale;
    mat4 model_cached;
    int dirty_model;
};

int graphic_draw_object(struct object *object)
{
    if (object->dirty_model) {
        object->model_cached = mat4_model(object->trans, object->rot, object->scale);
        object->dirty_model = 0;
    }
    if (object->vertecies)
        graphic_draw(object->vertecies, object->texture, object->model_cached);

    return 0;
}

static struct graphic_session  *session;

static struct baked_font font;

static struct object card;
static struct object triangle;
static struct object text[3];

void init_objects()
{
    struct graphic_texture *font_tex;

    font_tex = graphic_texture_create(font.width, font.height, font.bitmap);

    card.id = 0;
    card.vertecies     = graphic_vertecies_create(card_verts, 6);
    card.scale = vec3_create(2, 2, 2);

    triangle.id = 1;
    triangle.vertecies = graphic_vertecies_create(triangle_verts, 3);
    triangle.scale = vec3_create(2, 2, 2);

    text[0].id = 2;
    text[0].vertecies = graphic_vertecies_create_text(font, "Watch me take my steps");
    text[0].texture = font_tex;
    text[0].trans = vec3_create(-0.8, 0, 0);
    text[0].scale = vec3_create( 0.0045, 0.0045, 1);
    text[0].dirty_model = 1;

    text[1].id = 3;
    text[1].vertecies = graphic_vertecies_create_text(font, "The journey may be short and sweet");
    text[1].texture = font_tex;
    text[1].trans = vec3_create(-0.8,-0.1, 0);
    text[1].scale = vec3_create( 0.0025, 0.0025, 1);
    text[1].dirty_model = 1;

    text[2].id = 4;
    text[2].texture = font_tex;
    text[2].trans = vec3_create(-0.9,-0.8, 0);
    text[2].scale = vec3_create( 0.0025, 0.0025, 1);
    text[2].dirty_model = 1;
}
int game_init(struct graphic_session *created)
{
    session = created;

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
        object->trans.y = sin(step/4)/1.5;
    else
        object->trans.x = sin(step/3)/1.5;

    object->dirty_model = 1;
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
    object_oscillate(&triangle, step_slow, 1);

    graphic_clear(clear_color.x, clear_color.y, clear_color.z);

    graphic_draw_object(&card);
    graphic_draw_object(&triangle);
    for (i = 0; i < 3; i++)
        graphic_draw_object(&text[i]);

    graphic_render(session);
}

void game_update()
{
    if (session)
        render();
}

void game_mouse_event(struct mouse_event event)
{
    char strbuf[32];

    if (event.state == MOUSE_UP)
        clear_color.y = clear_color.y == 1 ? 0 : 1;
    else if (event.state == MOUSE_DOWN)
        clear_color.z = clear_color.z == 1 ? 0 : 1;

    if (text[2].vertecies)
        graphic_vertecies_destroy(text[2].vertecies);
    snprintf(strbuf, sizeof(strbuf), "x: %.2f, y: %.2f", event.mouse_x, event.mouse_y);
    text[2].vertecies = graphic_vertecies_create_text(font, strbuf);
}
