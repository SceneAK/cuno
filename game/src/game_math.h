#ifndef MATH_H
#define MATH_H
#define PI 3.141592653589793
#define DEG_TO_RAD(deg) (deg * PI/180)

typedef struct { float m[4][4]; } mat4;

typedef struct { float x, y; } vec2;
typedef struct { float x, y, z; } vec3;

static const vec3 VEC3_ONE = { 1, 1, 1 };
static const vec3 VEC3_ZERO  = { 0, 0, 0 };

/* VECTORS */
vec2 vec2_create(float x, float y);
vec3 vec3_create(float x, float y, float z);
vec3 vec3_sum(vec3 a, vec3 b);
vec3 vec3_mult(vec3 a, vec3 b);
vec3 vec3_mult_mat4(mat4 mat, vec3 vec, float w);

/* MATRIX - Expected Row-Major */
static const mat4 MAT4_IDENTITY  =  { {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }, } };

mat4 mat4_scale(vec3 vec);
mat4 mat4_trans(vec3 vec);
mat4 mat4_rotx(float rad);
mat4 mat4_roty(float rad);
mat4 mat4_rotz(float rad);
mat4 mat4_mult(mat4 a, mat4 b); /* Chains nicely at the cost of copying mat4s */
mat4 mat4_invert(mat4 mat);
mat4 mat4_trs(vec3 trans, vec3 rot, vec3 scale);
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
