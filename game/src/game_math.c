#include <math.h>
#include "system/logging.h"
#include "game_math.h"

/* VECTORS */
vec2 vec2_create(float x, float y)
{ 
    vec2 val = {x, y};
    return val;
}

vec3 vec3_create(float x, float y, float z)
{ 
    vec3 val = {x, y, z};
    return val;
}

vec3 vec3_mult_mat4(mat4 mat, vec3 vec, float w)
{
    vec3 result;
    int col;

    col = 0;
    result.x = (mat.m[col][0] * vec.x) + (mat.m[col][1] * vec.y) + (mat.m[col][2] * vec.z) + (mat.m[col][3] * w);
    col = 1;
    result.y = (mat.m[col][0] * vec.x) + (mat.m[col][1] * vec.y) + (mat.m[col][2] * vec.z) + (mat.m[col][3] * w);
    col = 2;
    result.z = (mat.m[col][0] * vec.x) + (mat.m[col][1] * vec.y) + (mat.m[col][2] * vec.z) + (mat.m[col][3] * w);

    return result;
}

/* MATRICES */

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

mat4 mat4_mult(mat4 a, mat4 b) 
{
    mat4 result;
    int col, row, k;

    for (row = 0; row < 4; ++row) {
        for (col = 0; col < 4; ++col) {
            result.m[row][col] = 0.0f;
            for (k = 0; k < 4; ++k) {
                result.m[row][col] += a.m[row][k] * b.m[k][col];
            }
        }
    }
    return result;
}

mat4 mat4_invert(mat4 mat)
{
    mat4 result = MAT4_IDENTITY;
    int pivot_row, pivot_col;
    int row, col;
    float temp, pivot, factor;

    for (pivot_col = 0; pivot_col < 4; pivot_col++) {
        pivot_row = pivot_col;
        for (row = pivot_col+1; row < 4; row++) {
            if (fabsf(mat.m[row][pivot_col]) > fabsf(mat.m[pivot_row][pivot_col]))
                pivot_row = row;
        }

        if (pivot_row != pivot_col) {
            for (col = 0; col < 4; col++) {
                temp = mat.m[pivot_col][col];
                mat.m[pivot_col][col] = mat.m[pivot_row][col];
                mat.m[pivot_row][col] = temp;
            }
            for (col = 0; col < 4; col++) {
                temp = result.m[pivot_col][col];
                result.m[pivot_col][col] = result.m[pivot_row][col];
                result.m[pivot_row][col] = temp;
            }
            pivot_row = pivot_col;
        }

        pivot = mat.m[pivot_row][pivot_col];
        if (pivot == 0) {
            LOG("mat4 not invertible");
            return MAT4_IDENTITY;
        }

        for (col = 0; col < 4; col++)
            mat.m[pivot_row][col] /= pivot;
        for (col = 0; col < 4; col++)
            result.m[pivot_row][col] /= pivot;
        
        for (row = 0; row < 4; row++ ) {
            if (row == pivot_row)
                continue;

            factor = mat.m[row][pivot_col];
            for (col = 0; col < 4; col++)
                mat.m[row][col] -= factor * mat.m[pivot_row][col];
            for (col = 0; col < 4; col++)
                result.m[row][col] -= factor * result.m[pivot_row][col];
        }
    }

    return result;
}

/* MATRIX UTILS */

mat4 mat4_model(vec3 trans, vec3 rot, vec3 scale)
{
    mat4 model = mat4_scale(scale);

    if (rot.x)
        model = mat4_mult(mat4_rotx(rot.x), model);
    if (rot.y)
        model = mat4_mult(mat4_roty(rot.y), model);
    if (rot.z)
        model = mat4_mult(mat4_rotz(rot.z), model);

    if (trans.x || trans.y || trans.z)
        model = mat4_mult(mat4_trans(trans), model);

    return model;
}
