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
static struct object cursor;
static struct object card;

static struct object ortho_test;
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
    card.trans = vec3_create(0, 0, -10);
    card.scale = vec3_create(2.2, 2.2, 2.2);
    card.rect_bounds = card_bounds;

    cursor.id = 1;
    cursor.vertecies = card.vertecies;
    cursor.rot = vec3_create(0, 0, PI/4);
    cursor.scale = vec3_create(0.08, 0.05, 1);
    cursor.rect_bounds = card_bounds;

    text[0].id = 2;
    text[0].vertecies = graphic_vertecies_create_text(font, "Dandadan dandadan dandadan dandadan dandadan");
    text[0].texture = font_tex;
    text[0].trans = vec3_create(-5, 0, -10);
    text[0].scale = vec3_create( 0.02, 0.02, 1);
    text[0].model = mat4_model_object(&text[0]);

    text[1].id = 3;
    text[1].vertecies = graphic_vertecies_create_text(font, "Neuro-sama rules the world!");
    text[1].texture = font_tex;
    text[1].trans = vec3_create(-5, 7, -10);
    text[1].scale = vec3_create( 0.02, 0.02, 1);
    text[1].model = mat4_model_object(&text[1]);

    text[2].id = 4;
    text[2].texture = font_tex;
    text[2].trans = vec3_create(-5,-5, -10);
    text[2].scale = vec3_create( 0.01, 0.01, 1);
    text[2].model = mat4_model_object(&text[2]);

    ortho_test.id = 6; /* In ndc */
    ortho_test.vertecies = card.vertecies;
    ortho_test.trans = vec3_create(0.78, -0.78, 0);
    ortho_test.rot = vec3_create(0, 0, PI/2);
    ortho_test.scale = vec3_create(0.1, 0.1, 1);
    ortho_test.model = mat4_model_object(&ortho_test);
    ortho_test.model_inv = mat4_invert(ortho_test.model);
    ortho_test.rect_bounds = card_bounds;

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
    object->model_inv = mat4_invert(object->model);
}

static float step;
static vec3 clear_color = { 1, 1, 1 };
static int oscillate_orientation = 1;
void render()
{
    int i;

    step += oscillate_orientation ? 0.13f : 0.05f;

    graphic_clear(clear_color.x, clear_color.y, clear_color.z);

    graphic_draw_object(&cursor, perspective);

    graphic_draw_object(&ortho_test, mat4_identity());

    object_oscillate(&card, step, oscillate_orientation);
    graphic_draw_object(&card, perspective);

    for (i = 1; i < 3; i++)
        graphic_draw_object(&text[i], perspective);

    graphic_render(session);
}

/* Assumes no View matrix & camera at origin */
vec3 ndc_to_camspace(float aspect_ratio, float fov_y, vec2 ndc, float z_target)
{
    float screen_height, screen_width;
    vec3 world;

    screen_height = -z_target * tan(fov_y/2);
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
    const float some_random_ahh_z = -5;

    if (!session)
        return;

    mouse_camspace = ndc_to_camspace(ASPECT_RATIO, DEG_TO_RAD(FOV_DEG), mouse_ndc, some_random_ahh_z);
    cursor.trans = mouse_camspace;
    cursor.model = mat4_model_object(&cursor);

    if (text[2].vertecies)
        graphic_vertecies_destroy(text[2].vertecies);
    snprintf(strbuf, sizeof(strbuf), "camspace: (%.2f, %.2f)", mouse_camspace.x, mouse_camspace.y);
    text[2].vertecies = graphic_vertecies_create_text(font, strbuf);

    /* experiment */
    if (event.type != MOUSE_DOWN)
        return;
    if (ndc_on_ortho_bounds(&ortho_test, mouse_ndc.x, mouse_ndc.y)) {
        clear_color.z = clear_color.z ? 0 : 1;
        oscillate_orientation = oscillate_orientation ? 0 : 1;
    } 
}

void game_update()
{
    if (!session)
        return;

    if (origin_ray_intersects_bounds(&card, cursor.trans)) {
        clear_color.y = 0;
        step -= 0.075f;
    } else {
        clear_color.y = 1;
    }

    render();
}
