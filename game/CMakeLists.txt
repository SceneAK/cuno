set(SRC_DIR "${CMAKE_SOURCE_DIR}/game/src")
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/game/assets")

add_library(game STATIC
    ${SRC_DIR}/game.c
    ${SRC_DIR}/game_math.c
    ${SRC_DIR}/game_logic.c
    ${SRC_DIR}/component.c
    ${SRC_DIR}/utils.c
    ${SRC_DIR}/system/graphic/text.c
)

target_include_directories(game PUBLIC ${SRC_DIR})
