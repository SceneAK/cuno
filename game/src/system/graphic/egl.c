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

static EGLint const required_attr[] = {
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
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

    if (eglChooseConfig(session->display, required_attr, &session->config, 1, &num_config) != EGL_TRUE)
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

    session->context = eglCreateContext(session->display, session->config, EGL_NO_CONTEXT, NULL);
    session->surface = eglCreateWindowSurface(session->display, session->config, (NativeWindowType)native_window_handle, NULL);
    eglMakeCurrent(session->display, session->surface, session->surface, session->context);

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
