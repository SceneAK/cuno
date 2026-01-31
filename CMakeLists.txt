cmake_minimum_required(VERSION 3.19)
project(cuno 
	VERSION 1.0
	LANGUAGES C)
set(C_STANDARD 90)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory("src/engine")
add_subdirectory("src/game")

if(ANDROID)
	add_subdirectory("platform/android")
elseif(EMSCRIPTEN)
	add_subdirectory("platform/wasm")
else()
	message(STATUS "Platform not implemented")
endif()
