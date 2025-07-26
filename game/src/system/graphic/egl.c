#include <EGL/egl.h>
#include <stdlib.h>
#include <GLES2/gl2.h>
#include "system/graphic/graphic.h"
#include "system/graphic/matrix.h"
#include "system/graphic/glutil.h"
#include "system/graphic/glres.h"
#include "system/logging.h"

struct graphic_session{
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

    if(!session)
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
    default_aPos = glGetAttribLocation(default_program, "aPosition");
    default_aTexCoord = glGetAttribLocation(default_program, "aTexCoord");
    default_uMVP = glGetUniformLocation(default_program, "uMVP");
    glUseProgram(default_program);
}
void on_graphic_ready()
{
    create_program();
}

struct graphic_font_ctx {
    GLuint gl_tex;
    float width, height;
    unsigned int cp_len;
    struct graphic_codepoint *cps;
};
struct graphic_font_ctx *graphic_font_ctx_create(float width, float height, char *bitmap,
                                                        unsigned int cp_len, struct graphic_codepoint *cps)
{
    struct graphic_font_ctx *ctx = malloc(sizeof(struct graphic_font_ctx));
    if(!ctx)
        return NULL;

    ctx->width = width;
    ctx->height = height;
    glGenTextures(1, &ctx->gl_tex);
    glBindTexture(GL_TEXTURE_2D, ctx->gl_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    ctx->cp_len = cp_len;
    ctx->cps = cps;

    return ctx;
}

struct graphic_text_ctx {
    GLint vert_vbo;
};
struct graphic_text_ctx *graphic_text_ctx_create(struct graphic_font_ctx *font_ctx, const char *text)
{
    struct graphic_text_ctx *ctx = malloc(sizeof(struct graphic_text_ctx));    
    struct graphic_codepoint cp;
    float cp_width, cp_height, x = 0;

    GLfloat verts[18];

    for (const char *c = text; *c; c++) {
        cp = font_ctx->cps[*c - 32];
        cp_width = cp.x1 - cp.x0;
        cp_height = cp.y1 - cp.y0;

        construct_3D_quad(verts, x, 0, x + cp_width, cp_height);
        
        /* Texture coordinates
         *
         * add tex coords to verts
         * add it to the vbo
         * 
         * */

        x += cp.xadv;
    }
    return ctx;
}

struct graphic_draw_ctx {
    GLuint vbo;
    size_t vert_count;
};
struct graphic_draw_ctx *graphic_draw_ctx_create(const float *verts, size_t vert_count)
{
    struct graphic_draw_ctx *ctx = malloc(sizeof(struct graphic_draw_ctx));
    if(!ctx)
        return NULL;

    ctx->vert_count = vert_count;

    glGenBuffers(1, &ctx->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * (5*vert_count), verts, GL_STATIC_DRAW);

    return ctx;
}

void graphic_draw(struct graphic_draw_ctx *ctx, mat4 mvp)
{
    glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo);

    glVertexAttribPointer(default_aPos, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(default_aPos);

    glVertexAttribPointer(default_aTexCoord, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));
    glEnableVertexAttribArray(default_aTexCoord);

    glUniformMatrix4fv(default_uMVP, 1, GL_TRUE, mvp.m[0]);
    glDrawArrays(GL_TRIANGLES, 0, ctx->vert_count);
}
