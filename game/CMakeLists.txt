set(SRC_DIR "${CMAKE_SOURCE_DIR}/game/src")

add_library(game STATIC
    ${SRC_DIR}/main.c
    ${SRC_DIR}/system/graphic/egl.c
)
find_library(GLES NAMES GLESv2 REQUIRED)
find_library(EGL NAMES EGL REQUIRED)
target_link_libraries(game PRIVATE 
    ${GLES}
    ${EGL}
)
target_include_directories(game PUBLIC ${SRC_DIR})
