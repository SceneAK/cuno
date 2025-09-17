#include <GLES2/gl2.h>

static const char* VERTEX_SHADER_SRC =
    "attribute vec3 aPosition;\n"
    "attribute vec2 aTexCoord;\n"
    "uniform mat4 uMVP;\n"
    "varying vec2 vTexCoord;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = uMVP * vec4(aPosition, 1.0);\n"
    "   vTexCoord = aTexCoord;\n"
    "}\n";

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define FRAG_MODE_COLOR 0
#define FRAG_MODE_TEXTURE 1
#define FRAG_MODE_TEXTURE_MASK 2

static const char* FRAGMENT_SHADER_SRC =
    "precision mediump float;\n"
    "uniform int uFragMode;\n"
    "uniform vec3 uColorSolid;\n"
    "uniform sampler2D uTexture;\n"
    "varying vec2 vTexCoord;\n"
    "void main()\n"
    "{\n"
    "   if (uFragMode == "STR(FRAG_MODE_TEXTURE)")\n"
    "       gl_FragColor = texture2D(uTexture, vTexCoord);\n"
    "   else if (uFragMode ==  "STR(FRAG_MODE_TEXTURE_MASK)")\n"
    "       gl_FragColor = vec4(uColorSolid.x, uColorSolid.y, uColorSolid.z, texture2D(uTexture, vTexCoord).a);\n"
    "   else \n"
    "       gl_FragColor = vec4(uColorSolid.x, uColorSolid.y, uColorSolid.z, 1);\n"
    "   \n"
    "}\n";
