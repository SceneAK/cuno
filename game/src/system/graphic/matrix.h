#ifndef MATRIX_H
#define MATRIX_H
#define PI 3.141592653589793

/* VECTORS */
typedef struct { float x, y, z; } vec3;
vec3 vec3_create(float x, float y, float z);

/* MATRIX */
/* Expected Row-Major */
typedef struct { float m[4][4]; } mat4;

mat4 mat4_identity();

mat4 mat4_scale(vec3 vec);

mat4 mat4_trans(vec3 vec);

mat4 mat4_rotx(float rad);
mat4 mat4_roty(float rad);
mat4 mat4_rotz(float rad);

mat4 mat4_mult(mat4 a, mat4 b);


mat4 mat4_model(vec3 trans, vec3 rot, vec3 scale);

#endif
