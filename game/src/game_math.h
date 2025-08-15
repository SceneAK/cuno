#ifndef MATH_H
#define MATH_H
#define PI 3.141592653589793
#define DEG_TO_RAD(deg) (deg * PI/180)

typedef struct { float m[4][4]; } mat4;

typedef struct { float x, y; } vec2;
typedef struct { float x, y, z; } vec3;

/* VECTORS */
vec2 vec2_create(float x, float y);
vec3 vec3_create(float x, float y, float z);

vec3 vec3_mult_mat4(mat4 mat, vec3 vec, float w);

/* MATRIX */
/* Expected Row-Major */
mat4 mat4_identity();

mat4 mat4_scale(vec3 vec);

mat4 mat4_trans(vec3 vec);

mat4 mat4_rotx(float rad);
mat4 mat4_roty(float rad);
mat4 mat4_rotz(float rad);

mat4 mat4_mult(mat4 a, mat4 b); /* Chains nicely at the cost of copying mat4s */

mat4 mat4_invert(mat4 mat);

mat4 mat4_model(vec3 trans, vec3 rot, vec3 scale);

#endif
