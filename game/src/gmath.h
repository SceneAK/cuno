#ifndef MATH_H
#define MATH_H

#define PI 3.141592653589793
#define DEG_TO_RAD(deg) (deg * PI/180)


typedef struct { float m[4][4]; } mat4;

typedef struct { float x, y; } vec2;
typedef struct { float x, y, z; } vec3;

static const vec3 VEC3_ONE  = { 1, 1, 1 };
static const vec3 VEC3_ZERO = { 0, 0, 0 };

static const mat4 MAT4_IDENTITY  =  { {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }, } };


struct transform {
    vec3    trans, rot, scale;
};
static const struct transform TRANSFORM_ZERO = {0};

static struct transform a, b;

/* VECTORS */
static vec2 vec2_create(float x, float y)
{ 
    vec2 val = {x, y};
    return val;
}
static vec3 vec3_create(float x, float y, float z)
{ 
    vec3 val = {x, y, z};
    return val;
}
static vec3 vec3_all(float f)
{ 
    vec3 val = {f, f, f};
    return val;
}
static int vec3_equal(vec3 a, vec3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
static vec3 vec3_sum(vec3 a, vec3 b)
{
    vec3 sum = {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
    return sum;
}
static vec3 vec3_mult(vec3 a, vec3 b)
{
    vec3 product = {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z
    };
    return product;
}
static vec3 vec3_mult_f(vec3 a, float f)
{
    vec3 product = {
        a.x * f,
        a.y * f,
        a.z * f
    };
    return product;
}
static vec3 vec3_div(vec3 num, vec3 den)
{
    vec3 product = {
        num.x / den.x,
        num.y / den.y,
        num.z / den.z
    };
    return product;
}
static vec3 vec3_div_f(vec3 num, float f)
{
    vec3 product = {
        num.x / f,
        num.y / f,
        num.z / f
    };
    return product;
}
static void vec3_mult_ptr(vec3 *a, vec3 b)
{
    a->x *= b.x;
    a->y *= b.y;
    a->z *= b.z;
}
static vec3 vec3_mult_mat4(mat4 mat, vec3 vec, float w)
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

/* TRANSFORM */
struct transform transform_delta(struct transform transform, struct transform target);
struct transform transform_from_mat4(mat4 mat);
struct transform transform_relative(const struct transform *parent, const struct transform *subject);

/* MATRIX - Expected Row-Major */
mat4 mat4_scale(vec3 vec);
mat4 mat4_trans(vec3 vec);
mat4 mat4_rotx(float rad);
mat4 mat4_roty(float rad);
mat4 mat4_rotz(float rad);
mat4 mat4_mult(mat4 a, mat4 b); /* Chains nicely at the cost of copying mat4s */
mat4 mat4_invert(mat4 mat);
mat4 mat4_trs(struct transform transf);
mat4 mat4_perspective(float fov_y, float aspect, float near);
mat4 mat4_orthographic(float width, float height, float far);

/* RECT2D */
typedef struct {
    float x0; float y0; 
    float x1; float y1;
} rect2D;
static const rect2D RECT2D_ZERO = {0, 0, 0, 0};

/* UTILS */
vec2 screen_to_ndc(float screen_width, float screen_height, float x, float y);
vec3 ndc_to_camspace(float aspect_ratio, float fov_y, vec2 ndc, float z_slice);
vec2 ndc_to_orthospace(float ortho_width, float ortho_height, vec2 ndc);
int point_lands_on_rect(const rect2D *rect, const mat4 *rect_trs_inv, vec3 point);
int origin_ray_intersects_rect(const rect2D *rect, const mat4 *rect_trs_inv, const vec3 origin_ray_dir);

#endif
