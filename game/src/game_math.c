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

vec3 vec3_add(vec3 a, vec3 b)
{
    vec3 sum = {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
    return sum;
}

vec3 vec3_mult(vec3 a, vec3 b)
{
    vec3 product = {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z
    };
    return product;
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
mat4 mat4_trs(vec3 trans, vec3 rot, vec3 scale)
{
    mat4 trs = mat4_scale(scale);

    if (rot.x)
        trs = mat4_mult(mat4_rotx(rot.x), trs);
    if (rot.y)
        trs = mat4_mult(mat4_roty(rot.y), trs);
    if (rot.z)
        trs = mat4_mult(mat4_rotz(rot.z), trs);

    if (trans.x || trans.y || trans.z)
        trs = mat4_mult(mat4_trans(trans), trs);

    return trs;
}
mat4 mat4_perspective(float fov_y, float aspect, float near)
{
    float f = 1/tan(fov_y/2);
    mat4 result = {{
        { f/aspect, 0, 0, 0 },
        { 0, f, 0, 0 },
        { 0, 0, -1, -2 * near },
        { 0, 0, -1, 0 }
    }};
    return result;
}
mat4 mat4_orthographic(float width, float height, float far)
{
    mat4 result = {{
        { 2/width, 0, 0, 0 },
        { 0, 2/height, 0, 0 },
        { 0, 0, -2/far, -1 },
        { 0, 0, 0, 1 }
    }};
    return result;
}

/* UTILS - COORDS */
vec2 screen_to_ndc(float width, float height, float x, float y)
{
    vec2 ndc = {
        2*(x / width) - 1,
        1 - 2*(y / height)
    };
    return ndc;
}

vec3 ndc_to_camspace(float aspect_ratio, float fov_y, vec2 ndc, float z_slice)
{
    float slice_half_height, slice_half_width;
    vec3 camspace;

    slice_half_height = -z_slice * tan(fov_y/2);
    slice_half_width  = slice_half_height * aspect_ratio;

    camspace.x = ndc.x * slice_half_width;
    camspace.y = ndc.y * slice_half_height;
    camspace.z = z_slice;
    return camspace;
}

vec2 ndc_to_orthospace(float ortho_width, float ortho_height, vec2 ndc)
{
    vec2 ortho = {
        ndc.x * ortho_width/2,
        ndc.y * ortho_height/2
    };
    return ortho;
}

/* UTILS - RECT BOUNDS */
static int on_rect(const rect2D *rect, float local_x, float local_y)
{
    return ( (rect->x0 <= local_x) && (local_x <= rect->x1) )
        && ( (rect->y0 <= local_y) && (local_y <= rect->y1) );
}
int point_lands_on_rect(const rect2D *rect, const mat4 *rect_inv, vec3 point)
{
    vec3 local_coords = vec3_mult_mat4(*rect_inv, point, 1);
    return on_rect(rect, local_coords.x, local_coords.y);
}
int origin_ray_intersects_rect(const rect2D *rect, const mat4 *rect_inv, const vec3 origin_ray)
{
    vec3 local_origin =  { rect_inv->m[0][3], rect_inv->m[1][3], rect_inv->m[2][3] };
    vec3 local_dir;
    float intersect_x, intersect_y;

    local_dir = vec3_mult_mat4(*rect_inv, origin_ray, 0);

    if (local_dir.z == 0)
        return 0;
    intersect_x = local_origin.x + local_dir.x * (-local_origin.z / local_dir.z);
    intersect_y = local_origin.y + local_dir.y * (-local_origin.z / local_dir.z);

    return on_rect(rect, intersect_x, intersect_y);
}
