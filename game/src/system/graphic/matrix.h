#ifndef MATRIX_H
#define MATRIX_H

/* Expected Row-Major */
typedef struct { float m[4][4]; } mat4;
typedef struct { float x, y, z; } vec3;
#define VEC4_ZERO { 0, 0, 0, 0 };
#define PI 3.141592653589793

mat4 mat4_scale(vec3 vec);

mat4 mat4_trans(vec3 vec);

mat4 mat4_rotx(float rad);
mat4 mat4_roty(float rad);
mat4 mat4_rotz(float rad);

mat4 mat4_mult(mat4 a, mat4 b) ;

#endif
