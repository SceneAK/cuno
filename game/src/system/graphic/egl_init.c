#include <EGL/egl.h>
#include <stdlib.h>
#include <GLES/gl.h>
#include "system/graphic/graphic.h"
#include "system/logging.h"

struct graphic_session{
    EGLDisplay  display;
    EGLConfig   config;
    EGLContext  context;
    EGLSurface  surface;
};

static EGLint const required_attr[] = {
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
    EGL_NONE
};

struct graphic_session *graphic_session_create()
{
    struct graphic_session *session = malloc(sizeof(struct graphic_session));
    return session;
}

int graphic_session_setup(struct graphic_session *session, void *native_window_handle)
{
    EGLint num_config;

    session->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (session->display == EGL_NO_DISPLAY)
        return -1;

    if (eglInitialize(session->display, NULL, NULL) != EGL_TRUE)
        return -1;

    if (eglChooseConfig(session->display, required_attr, &session->config, 1, &num_config) != EGL_TRUE)
        return -1;

    session->context = eglCreateContext(session->display, session->config, EGL_NO_CONTEXT, NULL);
    session->surface = eglCreateWindowSurface(session->display, session->config, (NativeWindowType)native_window_handle, NULL);
    if (session->surface == EGL_NO_SURFACE)
        LOG("BAD NATIVE WINDOW");

    eglMakeCurrent(session->display, session->surface, session->surface, session->context);
    return 0;
}

int graphic_session_destroy(struct graphic_session *session)
{
    eglMakeCurrent(session->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (eglTerminate(session->display) == EGL_TRUE) {
        free(session);
        return 0;
    }
    return -1;
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
