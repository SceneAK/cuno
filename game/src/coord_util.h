#include <math.h>
#include "game_math.h"

vec2 screen_to_ndc(float width, float height, float x, float y)
{
    vec2 ndc = {
        2*(x / width) - 1,
        1 - 2*(y / height)
    };
    return ndc;
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
