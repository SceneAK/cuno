#include <GLES2/gl2.h>

const char* vertex_shader_src =
    "attribute vec3 aPosition;\n"
    "attribute vec2 aTexCoord;\n"
    "uniform mat4 uMVP;\n"
    "varying vec2 vTexCoord;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = uMVP * vec4(aPosition, 1.0);\n"
    "   vTexCoord = aTexCoord;\n"
    "}\n";

const char* fragment_shader_src =
    "precision mediump float;\n"
    "uniform bool uUseTexture;\n"
    "uniform sampler2D uTexture;\n"
    "varying vec2 vTexCoord;\n"
    "void main()\n"
    "{\n"
    "   if (uUseTexture) {\n"
    "       gl_FragColor = texture2D(uTexture, vTexCoord);\n"
    "   } else {\n"
    "       gl_FragColor = vec4(0.3, 0.93, 0.93, 1);\n"
    "   }\n"
    "}\n";
