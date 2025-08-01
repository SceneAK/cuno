#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <stdlib.h>
#include <string.h>
#include "system/graphic/graphic.h"
#include "system/graphic/matrix.h"
#include "system/graphic/glutil.h"
#include "system/graphic/glres.h"
#include "system/logging.h"

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
    free(session);
    return NULL;
}

void on_graphic_ready();
int graphic_session_reset_window(struct graphic_session *session, void *native_window_handle)
{
    eglMakeCurrent(session->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (session->surface != NULL)
        eglDestroySurface(session->display, session->surface);

    if (session->context == NULL)
        session->context = eglCreateContext(session->display, session->config, EGL_NO_CONTEXT, ctx_required_attr);

    session->surface = eglCreateWindowSurface(session->display, session->config, (NativeWindowType)native_window_handle, NULL);
    if (eglMakeCurrent(session->display, session->surface, session->surface, session->context) == EGL_FALSE) 
        LOG("ERR: Failed to make context current");

    if (session->surface == EGL_NO_SURFACE)
        return -1;

    on_graphic_ready();
    return 0;
}

int graphic_session_destroy(struct graphic_session *session)
{
    eglMakeCurrent(session->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(session->display, session->surface);
    eglDestroyContext(session->display, session->context);
    eglTerminate(session->display);
    return 0;
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
void create_program()
{
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);

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
    glUseProgram(default_program);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
void on_graphic_ready()
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

struct graphic_draw_ctx {
    GLuint                  glvbo;
    size_t                  vert_count;
    struct graphic_texture *texture;
};
struct graphic_draw_ctx *graphic_draw_ctx_create(const float *verts, size_t vert_count, struct graphic_texture *texture)
{
    struct graphic_draw_ctx *ctx = malloc(sizeof(struct graphic_draw_ctx));
    if (!ctx)
        return NULL;

    ctx->vert_count = vert_count;
    ctx->texture = texture;

    glGenBuffers(1, &ctx->glvbo);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->glvbo);
    glBufferData(GL_ARRAY_BUFFER, VERT_SIZE*(vert_count), verts, GL_STATIC_DRAW);

    return ctx;
}
void graphic_draw_ctx_destroy(struct graphic_draw_ctx *ctx)
{
    if (ctx->texture)
        graphic_texture_destroy(ctx->texture);

    glDeleteBuffers(1, &ctx->glvbo);
    free(ctx);
}

static GLuint bound_vbo = -1;
static GLuint bound_tex2D = -1;
void graphic_draw(struct graphic_draw_ctx *ctx, mat4 mvp)
{
    if (bound_vbo != ctx->glvbo) {
        glBindBuffer(GL_ARRAY_BUFFER, ctx->glvbo);
        bound_vbo = ctx->glvbo;
    
        glVertexAttribPointer(default_aPos, 3, GL_FLOAT, GL_FALSE, VERT_SIZE, (void*)0);
        glEnableVertexAttribArray(default_aPos);
    
        glVertexAttribPointer(default_aTexCoord, 2, GL_FLOAT, GL_FALSE, VERT_SIZE, (void*)(3*sizeof(GLfloat)));
        glEnableVertexAttribArray(default_aTexCoord);
    }

    if (ctx->texture) {
        if (bound_tex2D != ctx->texture->gltex) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ctx->texture->gltex);
            bound_tex2D = ctx->texture->gltex;
        }
        glUniform1i(default_uTexture, 0);
    }
    glUniform1i(default_uUseTexture, ctx->texture != NULL);

    glUniformMatrix4fv(default_uMVP, 1, GL_TRUE, mvp.m[0]);

    glDrawArrays(GL_TRIANGLES, 0, ctx->vert_count);
}
