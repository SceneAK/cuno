set(SRC_DIR "${CMAKE_SOURCE_DIR}/src/game")

add_library(game INTERFACE)
target_link_libraries(game INTERFACE engine)
target_include_directories(game INTERFACE ${SRC_DIR})

target_sources(game INTERFACE
    ${SRC_DIR}/logic.c
)
if (NO_GUI)
    target_sources(game INTERFACE 
        ${SRC_DIR}/cli.c
        ${SRC_DIR}/server.c
    )
else()
    target_sources(game INTERFACE 
        ${SRC_DIR}/gui.c
        ${SRC_DIR}/server.c
    )
endif()

