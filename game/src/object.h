#include "system/graphic/graphic.h"
#include "system/logging.h"

typedef struct {
    float x0, y0, z0,
          x1, y1, z1;
} box3D;

struct object {
    int id;

    vec3 trans, rot, scale;
    mat4 model;
    mat4 model_inv;

    struct graphic_vertecies *vertecies;
    struct graphic_texture *texture;

    box3D box_bounds;
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

/* No XNOR shenanigans, so ensure bounds xyz0 < xyz1. */
int in_object_bounds(const struct object *obj, vec3 point)
{LOGF("INPUT=(%f, %f, %f)", point.x, point.y, point.z);
    box3D bounds = obj->box_bounds; 
    vec3 local_point = vec3_mult_mat4(obj->model_inv, point);
    LOGF("INVERTED=(%f, %f, %f)", local_point.x, local_point.y, local_point.z);
    return ( (bounds.x0 <= local_point.x) && (local_point.x <= bounds.x1) ) &&
           ( (bounds.y0 <= local_point.y) && (local_point.y <= bounds.y1) ) &&
           ( (bounds.z0 <= local_point.z) && (local_point.z <= bounds.z1) );
}
