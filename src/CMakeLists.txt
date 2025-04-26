add_library(src STATIC main.c)
target_include_directories(src PUBLIC ${CMAKE_SOURCE_DIR}/include)
