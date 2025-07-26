#include "system/logging.h"
#include "system/graphic/graphic.h"
#include "system/graphic/matrix.h"
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
static struct graphic_session *session;
static struct graphic_draw_ctx *card_ctx;
static struct graphic_draw_ctx *triangle_ctx;
int init(struct graphic_session *created)
{
    LOG("INIT");
    session = created;
    card_ctx = graphic_draw_ctx_create(card_verts, 6);
    triangle_ctx = graphic_draw_ctx_create(triangle_verts, 3);
    return 0;
}

mat4 oscillate_model(float step)
{
    mat4 model;
    vec3 pos = {0, sin(step/3)/3, sin(step/6)/4};
    vec3 scale = { 2.0f, 2.0f, 2.0f };

    model = mat4_scale(scale);
    model = mat4_mult(model, mat4_trans(pos));
    model = mat4_mult(model, mat4_rotz(step));
    model = mat4_mult(model, mat4_roty(step/3));
    model = mat4_mult(model, mat4_rotx(step/9));
    return model;
}

static float step1;
static float step2;
void render()
{
    mat4 model1 = oscillate_model(step1);
    mat4 model2 = oscillate_model(step2);
    graphic_clear(1, 1, 1);

    graphic_draw(card_ctx, model1);
    graphic_draw(triangle_ctx, model2);

    step1 += 0.1f;
    step2 += 0.03f;

    graphic_render(session);
}

void update()
{
    /* I expect to have control over position, textures, model, etc */
    if (session)
        render();
}
