set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
file(GLOB SOURCES "${SRC_DIR}/engine/*.c")

add_library(engine_static STATIC ${SOURCES})
target_include_directories(engine_static PUBLIC ${SRC_DIR})
#set_target_properties(engine_static PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(engine INTERFACE)
target_link_libraries(engine INTERFACE
    engine_static
)

if (ANDROID)
    message(STATUS "Android Engine")
    target_sources(engine_static PRIVATE
        ${SRC_DIR}/engine/system/time/time_posix.c
        ${SRC_DIR}/engine/system/network/network_posix.c
    )
    target_link_libraries(engine_static PRIVATE
        android 
        log 
    )
    if(NO_GUI)
        message(STATUS "No GUI")
        target_sources(engine_static PRIVATE 
            ${SRC_DIR}/engine/system/log/log_stdio.c
            ${SRC_DIR}/engine/system/asset/asset_stdio.c
        )
    else()
        target_sources(engine_static PRIVATE 
            ${SRC_DIR}/engine/entry/main_andr.c
            ${SRC_DIR}/engine/system/log/log_andr.c
            ${SRC_DIR}/engine/system/graphic/egl.c
            ${SRC_DIR}/engine/system/asset/asset_andr.c
        )

        set(GLUE_DIR "${ANDROID_NDK}/sources/android/native_app_glue")
        target_include_directories(engine_static PRIVATE ${GLUE_DIR})
        add_library(android_glue STATIC "${GLUE_DIR}/android_native_app_glue.c")

        find_library(GLES NAMES GLESv2 REQUIRED)
        find_library(EGL NAMES EGL REQUIRED)
        target_link_libraries(engine INTERFACE 
            -Wl,--whole-archive android_glue -Wl,--no-whole-archive
            ${GLES}
            ${EGL}
        )
    endif()
elseif(EMSCRIPTEN)
    message(STATUS "TEST EMSCRIPTEN")
else()
    message(STATUS "DEFAULT POSIX CLI")
    target_sources(engine_static PRIVATE 
        ${SRC_DIR}/engine/system/log/log_stdio.c
        ${SRC_DIR}/engine/system/asset/asset_stdio.c
        ${SRC_DIR}/engine/system/time/time_posix.c
        ${SRC_DIR}/engine/system/network/network_posix.c
    )
    option(NO_GUI, ON)
endif()
