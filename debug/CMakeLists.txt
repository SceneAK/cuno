add_executable(cuno
    ${CMAKE_SOURCE_DIR}/game/src/system/graphic/nographic.c
    ${CMAKE_SOURCE_DIR}/game/src/system/noasset.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main_headless.c
)
target_compile_definitions(game PUBLIC DEBUG LOG_TO_STDOUT)
target_link_libraries(cuno game m)
