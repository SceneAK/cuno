set(SRC_DIR "${CMAKE_SOURCE_DIR}/game/src")
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/game/assets")

add_library(base STATIC
    ${SRC_DIR}/gmath.c
    ${SRC_DIR}/logic.c
    ${SRC_DIR}/utils.c
)

target_include_directories(base PUBLIC ${SRC_DIR})
