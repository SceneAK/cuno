#include "system/logging.h"
#include "system/graphic/graphic.h"

struct object {
    int id;

    vec3 trans, rot, scale;
    mat4 model;
    mat4 model_inv;

    struct graphic_vertecies *vertecies;
    struct graphic_texture *texture;

    rect2D click_bounds;
};

void graphic_draw_object(const struct object *object, mat4 perspective)
{
    if (!object->vertecies)
        return;
    graphic_draw(object->vertecies, object->texture, mat4_mult(perspective, object->model));
}

mat4 mat4_model_object(const struct object *obj)
{
    return mat4_model(obj->trans, obj->rot, obj->scale);
}

/* Keep it stupid. No need for raycast math. */
int falls_in_click_rect(const struct object *obj, const vec3 *point, const vec3 *dir)
{
    vec3 inverted_point = vec3_mult_mat4(obj->model_inv, vec3_create(point->x, point->y, 0));
    vec3 inverted_dir   = { inverted_point.x - obj->model_inv.m[0][3], 
                            inverted_point.y - obj->model_inv.m[1][3], 
                            inverted_point.z - obj->model_inv.m[2][3]};
    vec3 land_point;

    if (inverted_dir.z == 0)
        return 0;
    land_point = vec3_create(inverted_point.x + inverted_dir.x * (-inverted_point.z / inverted_dir.z),
                            inverted_point.y + inverted_dir.y * (-inverted_point.z / inverted_dir.z),
                            0);
    LOGF("point&dir: (%.2f, %.2f)", point->x, point->y);
    LOGF("Inverted point: (%.2f, %.2f) Inverted dir: (%.2f, %.2f)", inverted_point.x, inverted_point.y, inverted_dir.x, inverted_dir.y);
    LOGF("Land Point: (%.2f, %.2f)", land_point.x, land_point.y);
    return ( (obj->click_bounds.x0 <= land_point.x) && (land_point.x <= obj->click_bounds.x1) ) &&
           ( (obj->click_bounds.y0 <= land_point.y) && (land_point.y <= obj->click_bounds.y1) );
}
