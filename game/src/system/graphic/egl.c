#include <EGL/egl.h>
#include <stdlib.h>
#include <GLES2/gl2.h>
#include "system/graphic/graphic.h"
#include "system/graphic/matrix.h"
#include "system/graphic/glutil.h"
#include "system/logging.h"

struct graphic_session{
    EGLDisplay  display;
    EGLConfig   config;
    EGLContext  context;
    EGLSurface  surface;
};

static EGLint const dpy_required_attr[] = {
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
    EGL_NONE
};

static EGLint const ctx_required_attr[] = {
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

int graphic_session_destroy(struct graphic_session *session)
{
    eglMakeCurrent(session->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(session->display, session->surface);
    eglDestroyContext(session->display, session->context);
    eglTerminate(session->display);
    return 0;
}

const char* vertex_shader_src =
    "attribute vec4 aPosition;\n"
    "uniform mat4 uModel;\n"
    "void main() {\n"
    "   \n"
    "   gl_Position = uModel * aPosition;\n"
    "}\n";
const char* fragment_shader_src =
    "precision mediump float;\n"
    "uniform vec2 uResolution;\n"
    "void main() {\n"
    "   float g_add = (gl_FragCoord.x * 2.0)/uResolution.x;\n"
    "   float b_add = (gl_FragCoord.y * 2.0)/uResolution.y;\n"
    "   gl_FragColor = vec4(cos(g_add), g_add, b_add, 1.0);\n"
    "}\n";
GLfloat vertecies[] = {
    -0.1f,  0.15f, 0.0f,
    -0.1f, -0.15f, 0.0f,
     0.1f, -0.15f, 0.0f,

    -0.1f,  0.15f, 0.0f,
     0.1f,  0.15f, 0.0f,
     0.1f, -0.15f, 0.0f,
};

GLuint create_program() {
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[512];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        LOGF("Shader compile error: %s\n", log);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

static GLuint program;
static GLuint pos_attrib;
static GLuint model_uni;
static GLuint resolution_uni;
static GLuint vbo;

void graphic_draw_init(struct graphic_session *session)
{
    program = create_program();
    pos_attrib = glGetAttribLocation(program, "aPosition");
    model_uni = glGetUniformLocation(program, "uModel");
    resolution_uni = glGetUniformLocation(program, "uResolution");
    glUseProgram(program);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertecies), vertecies, GL_STATIC_DRAW);

    glEnableVertexAttribArray(pos_attrib);
    glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

    EGLint width, height;
    eglQuerySurface(session->display, session->surface, EGL_WIDTH, &width);
    eglQuerySurface(session->display, session->surface, EGL_HEIGHT, &height);
    glUniform2f(resolution_uni, width, height);
}

int graphic_session_reset_window(struct graphic_session *session, void *native_window_handle)
{
    eglMakeCurrent(session->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (session->surface != NULL)
        eglDestroySurface(session->display, session->surface);

    if (session->context != NULL)
        eglDestroyContext(session->display, session->context);

    session->context = eglCreateContext(session->display, session->config, EGL_NO_CONTEXT, ctx_required_attr);
    session->surface = eglCreateWindowSurface(session->display, session->config, (NativeWindowType)native_window_handle, NULL);
    if (eglMakeCurrent(session->display, session->surface, session->surface, session->context) == EGL_FALSE) 
        LOG("ERR: Failed to make context current");

    if (session->surface == EGL_NO_SURFACE)
        return -1;
    graphic_draw_init(session);
    return 0;
}

void graphic_draw_default(struct graphic_session *session, mat4 model)
{
    glUniformMatrix4fv(model_uni, 1, GL_TRUE, model.m[0]);

    glClearColor(1.0f, 1.0f, 1.0f, 0.98f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glFlush();
    eglSwapBuffers(session->display, session->surface);
}
