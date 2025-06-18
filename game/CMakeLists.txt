add_library(game_core STATIC ${CMAKE_SOURCE_DIR}/game/src/main.c)
target_include_directories(game_core PUBLIC ${CMAKE_SOURCE_DIR}/game/include)
