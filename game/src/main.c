#include <stdio.h>
#include <math.h>
#include "system/logging.h"
#include "system/graphic/graphic.h"
#include "system/graphic/text.h"
#include "system/asset_management.h"
#include "game_math.h"
#include "object.h"
#include "main.h"

/* RESOURCES */
static struct graphic_session *session;
struct graphic_session_info session_info;
#define ASPECT_RATIO ((float)session_info.width / session_info.height)

static struct baked_font font;
static mat4 perspective;

/* GLOBAL CONST */
static const float NEAR = 0.2f;
static const float FOV_DEG = 90;

/* OBJECTS */
static struct object card;
static struct object static_card;
static struct object triangle;
static struct object text[3];

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
    card.id = 0;
    card.vertecies = graphic_vertecies_create(card_verts, 6);
    card.rot = vec3_create(0, 0, 0);
    card.scale = vec3_create(1, 1, 1);
    card.click_bounds = card_bounds;

    static_card.id = 9;
    static_card.vertecies = graphic_vertecies_create(card_verts, 6);
    static_card.trans = vec3_create(0, 0, -10);
    static_card.rot = vec3_create(0, 0, 0);
    static_card.scale = vec3_create(2, 2, 2);
    static_card.model = mat4_model_object(&static_card);
    static_card.model_inv = mat4_invert(static_card.model);
    static_card.click_bounds = card_bounds;

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
    text[1].vertecies = graphic_vertecies_create_text(font, "I could be a martyr, I could be a cause");
    text[1].texture = font_tex;
    text[1].trans = vec3_create(-5,-0.6, -10);
    text[1].scale = vec3_create( 0.012, 0.012, 1);
    text[1].model = mat4_model_object(&text[1]);

    text[2].id = 4;
    text[2].texture = font_tex;
    text[2].trans = vec3_create(-5,-5, -10);
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
    session = created_session;
    session_info = graphic_session_info_get(created_session);

    perspective = mat4_perspective(DEG_TO_RAD(FOV_DEG), ASPECT_RATIO, NEAR);
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

    graphic_clear(clear_color.x, clear_color.y, clear_color.z);

    /* object_oscillate(&card, step_fast, 0); */
    /* card.model_inv = mat4_invert(card.model); */
    graphic_draw_object(&card, perspective);

    graphic_draw_object(&static_card, perspective);

    /* object_oscillate(&triangle, step_slow, 1); */
    /* graphic_draw_object(&triangle, perspective); */

    for (i = 1; i < 3; i++)
        graphic_draw_object(&text[i], perspective);

    graphic_render(session);
}

void game_update()
{
    if (!session)
        return;

    render();
}

/* Assumes no View matrix & camera at origin */
vec3 ndc_to_cameraspace(float aspect_ratio, float fov_y, vec2 ndc, float z_target)
{
    float screen_height, screen_width;
    vec3 world;

    screen_height = z_target * tan(fov_y/2);
    screen_width = screen_height * aspect_ratio;

    world.x = ndc.x * screen_width;
    world.y = ndc.y * screen_height;
    world.z = z_target;
    return world;
}

vec2 screen_to_ndc(float x, float y)
{
    vec2 ndc = {
        2*(x / session_info.width) - 1,
        1 - 2*(y / session_info.height)
    };
    return ndc;
}

void game_mouse_event(struct mouse_event event)
{
    char strbuf[128];
    vec2 mouse_ndc = screen_to_ndc(event.mouse_x, event.mouse_y);
    vec3 mouse_camspace = { 0, 0, 0 };

    if (!session)
        return;

    /* test for static_card */
    mouse_camspace = ndc_to_cameraspace(ASPECT_RATIO, DEG_TO_RAD(FOV_DEG), mouse_ndc, static_card.trans.z);
    card.trans = mouse_camspace;
    card.model = mat4_model_object(&card);

    if (text[2].vertecies)
        graphic_vertecies_destroy(text[2].vertecies);
    snprintf(strbuf, sizeof(strbuf), "cam: (%.2f, %.2f)", mouse_camspace.x, mouse_camspace.y);
    text[2].vertecies = graphic_vertecies_create_text(font, strbuf);

    if (event.type == MOUSE_DOWN && event.type != MOUSE_UP)
        return;
    if (falls_in_click_rect(&static_card, &mouse_camspace, &mouse_camspace)) {
        clear_color.y = clear_color.y == 1 ? 0 : 1;
        LOG("success");
    }
}
