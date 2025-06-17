add_library(src STATIC main.c)
set_target_properties(src PROPERTIES 
    C_STANDARD 90
    C_EXTENSIONS OFF
)
target_include_directories(src PUBLIC ${CMAKE_SOURCE_DIR}/include)
