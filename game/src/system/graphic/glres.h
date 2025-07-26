#include <GLES2/gl2.h>

const char* vertex_shader_src =
    "attribute vec3 aPosition;\n"
    "attribute vec2 aTexCoord;\n"
    "uniform mat4 uMVP;\n"
    "void main() {\n"
    "   \n"
    "   gl_Position = uMVP * vec4(aPosition, 1.0);\n"
    "}\n";

const char* fragment_shader_src =
    "precision mediump float;\n"
    "void main() {\n"
    "   gl_FragColor = vec4(0, 255, 255, 1);\n"
    "}\n";

;
