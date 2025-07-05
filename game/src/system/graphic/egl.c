#include <EGL/egl.h>
#include <stdlib.h>
#include <GLES2/gl2.h>
#include "system/graphic/graphic.h"
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

int graphic_session_switch_window(struct graphic_session *session, void *native_window_handle)
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

const char* vertex_shader_src =
    "attribute vec4 aPosition;\n"
    "void main() {\n"
    "    gl_Position = aPosition;\n"
    "}\n";

const char* fragment_shader_src =
    "precision mediump float;\n"
    "void main() {\n"
    "    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";

GLfloat triangle[] = {
     0.0f,  0.5f,
    -0.5f, -0.5f,
     0.5f, -0.5f
};

GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        LOGF("Shader compile error: %s\n", log);
    }

    return shader;
}

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
static GLuint vbo;
void glinit()
{
    program = create_program();
    pos_attrib = glGetAttribLocation(program, "aPosition");
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
}
void graphic_draw(struct graphic_session *session)
{
    if (!program)
        glinit();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(program);
    
    glEnableVertexAttribArray(pos_attrib);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    glDisableVertexAttribArray(pos_attrib);
    glFlush();
    eglSwapBuffers(session->display, session->surface);
}

static float red = 0;
static float green = 1;
static float blue = 0.5;
void graphic_gay_test(struct graphic_session *session)
{
    red += 0.05;
    green += 0.03;
    blue += 0.015;
    if (red >= 1.0) red = 0;
    if (green >= 1.0) green = 0;
    if (blue >= 1.0) blue = 0;

    glClearColor(red, green, blue, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
    eglSwapBuffers(session->display, session->surface);
}
