#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "system/graphic/graphic.h"
#include "system/graphic/glutil.h"
#include "system/graphic/glres.h"
#include "system/logging.h"
#include "game_math.h"

struct graphic_session {
    EGLDisplay  display;
    EGLConfig   config;
    EGLContext  context;
    EGLSurface  surface;
};

static const EGLint dpy_required_attr[] = {
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
    EGL_NONE
};

static const EGLint ctx_required_attr[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

static const char* egl_err_to_str(EGLint err)
{
    switch(err) {
        case EGL_SUCCESS: return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
        case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
        case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
        case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
        default: return "UNKNOWN";
    }
}

struct graphic_session *graphic_session_create()
{
    EGLint num_config;
    struct graphic_session *session = malloc(sizeof(struct graphic_session));

    if (!session)
        return NULL;

    session->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (session->display == EGL_NO_DISPLAY)
        goto err;

    if (eglInitialize(session->display, NULL, NULL) != EGL_TRUE)
        goto err;

    if (eglChooseConfig(session->display, dpy_required_attr, &session->config, 1, &num_config) != EGL_TRUE)
        goto err;
    
    return session;

err:
    LOG(egl_err_to_str(eglGetError()));
    free(session);
    return NULL;
}

int graphic_session_destroy(struct graphic_session *session)
{
    eglMakeCurrent(session->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(session->display, session->surface);
    eglDestroyContext(session->display, session->context);
    eglTerminate(session->display);
    return 0;
}

static void on_graphic_ready();
int graphic_session_reset_window(struct graphic_session *session, void *native_window_handle)
{
    eglMakeCurrent(session->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (session->surface != NULL)
        eglDestroySurface(session->display, session->surface);
    session->surface = eglCreateWindowSurface(session->display, session->config, (NativeWindowType)native_window_handle, NULL);

    eglBindAPI(EGL_OPENGL_ES_API);
    if (session->context == NULL)
        session->context = eglCreateContext(session->display, session->config, EGL_NO_CONTEXT, ctx_required_attr);

    if (eglMakeCurrent(session->display, session->surface, session->surface, session->context) == EGL_FALSE)  {
        LOG("ERR: Failed to make context current");
        return -1;
    }

    on_graphic_ready();
    return 0;
}

struct graphic_session_info graphic_session_info_get(struct graphic_session *session)
{
    struct graphic_session_info info;
    eglQuerySurface(session->display, session->surface, EGL_WIDTH, &info.width);
    eglQuerySurface(session->display, session->surface, EGL_HEIGHT, &info.height); 
    return info;
}


/* END OF EGL. START OF GL */

void graphic_clear(float r, float g, float b)
{
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void graphic_render(struct graphic_session *session)
{
    glFlush();
    eglSwapBuffers(session->display, session->surface);
}

static GLuint default_program;
static GLuint default_aPos;
static GLuint default_aTexCoord;
static GLuint default_uMVP;
static GLuint default_uUseTexture;
static GLuint default_uTexture;
static GLuint default_uColorSolid;
static void create_program()
{
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, VERTEX_SHADER_SRC);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SRC);

    GLuint default_program = glCreateProgram();
    glAttachShader(default_program, vertex_shader);
    glAttachShader(default_program, fragment_shader);
    glLinkProgram(default_program);

    GLint linked;
    glGetProgramiv(default_program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[512];
        glGetProgramInfoLog(default_program, sizeof(log), NULL, log);
        LOGF("Shader compile error: %s\n", log);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    default_aPos        = glGetAttribLocation(default_program, "aPosition");
    default_aTexCoord   = glGetAttribLocation(default_program, "aTexCoord");
    default_uMVP        = glGetUniformLocation(default_program, "uMVP");
    default_uUseTexture = glGetUniformLocation(default_program, "uUseTexture");
    default_uTexture    = glGetUniformLocation(default_program, "uTexture");
    default_uColorSolid    = glGetUniformLocation(default_program, "uColorSolid");
    glUseProgram(default_program);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
static void on_graphic_ready()
{
    create_program();
}

struct graphic_texture {
    GLuint  gltex;
    float   width, height;
};
struct graphic_texture *graphic_texture_create(float width, float height, const unsigned char *bitmap)
{
    struct graphic_texture *texture = malloc(sizeof(struct graphic_texture));
    if (!texture)
        return NULL;

    texture->width = width;
    texture->height = height;
    glGenTextures(1, &texture->gltex);
    glBindTexture(GL_TEXTURE_2D, texture->gltex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texture;
}
void graphic_texture_destroy(struct graphic_texture *texture)
{
    glDeleteTextures(1, &texture->gltex);
    free(texture);
}

struct graphic_vertecies {
    GLuint                  glvbo;
    size_t                  vert_count;
};
struct graphic_vertecies *graphic_vertecies_create(const float *verts, size_t vert_count)
{
    struct graphic_vertecies *vertecies = malloc(sizeof(struct graphic_vertecies));
    if (!vertecies)
        return NULL;

    vertecies->vert_count = vert_count;

    glGenBuffers(1, &vertecies->glvbo);
    glBindBuffer(GL_ARRAY_BUFFER, vertecies->glvbo);
    glBufferData(GL_ARRAY_BUFFER, VERT_SIZE*(vert_count), verts, GL_STATIC_DRAW);

    return vertecies;
}
void graphic_vertecies_destroy(struct graphic_vertecies *vertecies)
{
    glDeleteBuffers(1, &vertecies->glvbo);
    free(vertecies);
}

static GLuint bound_vbo = -1;
static GLuint bound_tex2D = -1;
void graphic_draw(struct graphic_vertecies *vertecies, struct graphic_texture *texture, mat4 mvp, vec3 color)
{
    if (!vertecies)
        return;

    if (bound_vbo != vertecies->glvbo) {
        glBindBuffer(GL_ARRAY_BUFFER, vertecies->glvbo);
        bound_vbo = vertecies->glvbo;
    
        glVertexAttribPointer(default_aPos, 3, GL_FLOAT, GL_FALSE, VERT_SIZE, (void*)0);
        glEnableVertexAttribArray(default_aPos);
    
        glVertexAttribPointer(default_aTexCoord, 2, GL_FLOAT, GL_FALSE, VERT_SIZE, (void*)(3*sizeof(GLfloat)));
        glEnableVertexAttribArray(default_aTexCoord);
    }

    if (texture) {
        if (bound_tex2D != texture->gltex) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture->gltex);
            bound_tex2D = texture->gltex;
        }
        glUniform1i(default_uTexture, 0);
    }

    glUniform1i(default_uUseTexture, texture != NULL);
    glUniform3f(default_uColorSolid, color.x, color.y, color.z);
    glUniformMatrix4fv(default_uMVP, 1, GL_TRUE, mvp.m[0]);

    glDrawArrays(GL_TRIANGLES, 0, vertecies->vert_count);
}

/* Utils */
/* Assumes 30 elements allocated */
void graphic_construct_3D_quad(float *verts, rect2D dimension, rect2D tex) 
{
    verts[0]   = dimension.x0; verts[1]  = dimension.y0; verts[2]  = 0.0f; verts[3]  = tex.x0; verts[4]  = tex.y0;
     verts[5]  = dimension.x1; verts[6]  = dimension.y0; verts[7]  = 0.0f; verts[8]  = tex.x1; verts[9]  = tex.y0;
     verts[10] = dimension.x0; verts[11] = dimension.y1; verts[12] = 0.0f; verts[13] = tex.x0; verts[14] = tex.y1;

    verts[15]  = dimension.x1; verts[16] = dimension.y1; verts[17] = 0.0f; verts[18] = tex.x1; verts[19] = tex.y1;
     verts[20] = dimension.x1; verts[21] = dimension.y0; verts[22] = 0.0f; verts[23] = tex.x1; verts[24] = tex.y0;
     verts[25] = dimension.x0; verts[26] = dimension.y1; verts[27] = 0.0f; verts[28] = tex.x0; verts[29] = tex.y1;
}

