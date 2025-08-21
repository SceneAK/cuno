#include "system/graphic/graphic.h"

struct object {
    int id;

    vec3 trans, rot, scale;
    mat4 model;
    mat4 model_inv;

    struct graphic_vertecies *vertecies;
    struct graphic_texture *texture;

    rect2D rect_bounds;
};

mat4 mat4_model_object(const struct object *obj)
{
    return mat4_model(obj->trans, obj->rot, obj->scale);
}

void graphic_draw_object(const struct object *object, mat4 perspective)
{
    if (!object->vertecies)
        return;
    graphic_draw(object->vertecies, object->texture, mat4_mult(perspective, object->model));
}

int on_rect_bounds(const rect2D *rect_bounds, float local_x, float local_y)
{
    return ( (rect_bounds->x0 <= local_x) && (local_x <= rect_bounds->x1) )
        && ( (rect_bounds->y0 <= local_y) && (local_y <= rect_bounds->y1) );
}

/* Just a test/experimental function. any rotation will probably fuck it up */
int ndc_on_ortho_bounds(const struct object *obj, float ndc_x, float ndc_y)
{
    vec3 local_coords = vec3_mult_mat4(obj->model_inv, vec3_create(ndc_x, ndc_y, 0), 1);
    return on_rect_bounds(&obj->rect_bounds, local_coords.x, local_coords.y);
}

int origin_ray_intersects_bounds(const struct object *obj, const vec3 ray_dir)
{
    vec3 local_origin =  { obj->model_inv.m[0][3], obj->model_inv.m[1][3], obj->model_inv.m[2][3] };
    vec3 local_dir;
    float intersect_x, intersect_y;

    local_dir = vec3_mult_mat4(obj->model_inv, ray_dir, 0);

    if (local_dir.z == 0)
        return 0;
    intersect_x = local_origin.x + local_dir.x * (-local_origin.z / local_dir.z);
    intersect_y = local_origin.y + local_dir.y * (-local_origin.z / local_dir.z);

    return on_rect_bounds(&obj->rect_bounds, intersect_x, intersect_y);
}
