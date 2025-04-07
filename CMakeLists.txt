cmake_minimum_required(VERSION 3.10)
project(cuno VERSION 1.0)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/src/lib)

# glue
set(GLUE_DIR "${ANDROID_NDK}/sources/android/native_app_glue")
include_directories(${GLUE_DIR})

set(GLUE_SRC ${GLUE_DIR}/android_native_app_glue.c)
add_library(native_app_glue STATIC ${GLUE_SRC})

add_library(cuno SHARED src/main.c)
target_link_libraries(cuno native_app_glue log android)

