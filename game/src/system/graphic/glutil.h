#include <GLES2/gl2.h>
#include "system/logging.h"

GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        LOGF("ERR: \nShader failed to compile with the following info: %s\n", log);
    }

    return shader;
}

void construct_3D_quad(GLfloat *target, float x0, float y0, float x1, float y1)
{
    target[0] = x0; target[1] = y0; target[2] = 0.0f;  
     target[3] = x1; target[4] = y0; target[5] = 0.0f;  
     target[6] = x0; target[7] = y1; target[8] = 0.0f;  

    target[9]  = x1; target[10] = x1; target[11] = 0.0f;  
     target[12] = x1; target[13] = y0; target[14] = 0.0f;  
     target[15] = x0; target[16] = y1; target[17] = 0.0f;  
}
