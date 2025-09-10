set(SRC_DIR "${CMAKE_SOURCE_DIR}/game/src")
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/game/assets")

add_library(game STATIC
    ${SRC_DIR}/main.c
    ${SRC_DIR}/game_math.c
    ${SRC_DIR}/game_logic.c
    ${SRC_DIR}/component.c
    ${SRC_DIR}/system/graphic/egl.c
    ${SRC_DIR}/system/graphic/text.c
#   ${SRC_DIR}/external/stb_truetype.h
)

find_library(GLES NAMES GLESv2 REQUIRED)
find_library(EGL NAMES EGL REQUIRED)
target_link_libraries(game PRIVATE 
    ${GLES}
    ${EGL}
)
target_include_directories(game PUBLIC ${SRC_DIR})
