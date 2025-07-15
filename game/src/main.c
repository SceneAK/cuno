#include "system/logging.h"
#include "system/graphic/graphic.h"
#include "system/graphic/matrix.h"
#include "main.h"
#include <math.h>

static struct graphic_session *session;
int init(struct graphic_session *created)
{
    session = created;
    return 0;
}

static mat4 model;
static float step = 0.0f;
static vec3 scale = { 2.0f, 2.0f, 2.0f };
void draw()
{
    vec3 pos = {0, sin(step/3)/3, sin(step/6)/4};
    step += 0.1f;
    model = mat4_scale(scale);
    model = mat4_mult(model, mat4_trans(pos));
    model = mat4_mult(model, mat4_rotz(step));
    model = mat4_mult(model, mat4_roty(step/3));
    model = mat4_mult(model, mat4_rotx(step/9));
    graphic_draw_default(session, model);
}

void update()
{
    if (session)
        draw();
}
