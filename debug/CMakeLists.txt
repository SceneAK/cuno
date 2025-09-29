add_executable(cuno
    ${CMAKE_SOURCE_DIR}/game/src/system/graphic/nographic.c
    ${CMAKE_SOURCE_DIR}/game/src/system/noasset.c
    ${CMAKE_CURRENT_SOURCE_DIR}/main_headless.c
)

target_link_libraries(cuno game)
