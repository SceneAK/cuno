#include "matrix.h"
#include <math.h>

mat4 mat4_scale(vec3 vec)
{
    mat4 result = { {
        { vec.x, 0, 0, 0 },
        { 0, vec.y, 0, 0 },
        { 0, 0, vec.z, 0 },
        { 0, 0, 0, 1 },
    } };
    return result;
}

mat4 mat4_trans(vec3 vec)
{
    mat4 result = { {
        { 1, 0, 0, vec.x },
        { 0, 1, 0, vec.y },
        { 0, 0, 1, vec.z },
        { 0, 0, 0, 1 },
    } };
    return result;
}

mat4 mat4_rotx(float rad)
{
    float cos_v, sin_v;
    cos_v = cos(rad);
    sin_v = sin(rad);

    mat4 result = {{
        { 1, 0, 0, 0 },
        { 0, cos_v, -sin_v, 0 },
        { 0, sin_v, cos_v, 0 },
        { 0, 0, 0, 1 }
    }};
    return result;
}

mat4 mat4_roty(float rad)
{
    float cos_v, sin_v;
    cos_v = cos(rad);
    sin_v = sin(rad);

    mat4 result = {{
        { cos_v, 0, -sin_v, 0 },
        { 0, 1, 0, 0 },
        { sin_v, 0, cos_v, 0 },
        { 0, 0, 0, 1 }
    }};
    return result;
}

mat4 mat4_rotz(float rad)
{
    float cos_v, sin_v;
    cos_v = cos(rad);
    sin_v = sin(rad);

    mat4 result = {{
        { cos_v,-sin_v, 0, 0 },
        { sin_v, cos_v, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }
    }};
    return result;
}

/* Row-Major */
mat4 mat4_mult(mat4 a, mat4 b) 
{
    mat4 result;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result.m[row][col] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result.m[row][col] += a.m[row][k] * b.m[k][col];
            }
        }
    }
    return result;
}
