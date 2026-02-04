cmake_minimum_required(VERSION 3.19)
project(cuno 
	VERSION 1.0
	LANGUAGES C)
set(C_STANDARD 99)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}/bin CACHE FILEPATH "Runtime Output Dir")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}/lib CACHE FILEPATH "Archive Output Dir")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY 
    ${CMAKE_BINARY_DIR}/lib CACHE FILEPATH "Library Output Dir")

add_subdirectory("src/engine")
add_subdirectory("src/game")

if(ANDROID)
	add_subdirectory("platform/android")
elseif(EMSCRIPTEN)
	add_subdirectory("platform/wasm")
elseif(UNIX AND NO_GUI)
    message(STATUS "UNIX cli patch")
    add_executable(cuno)
    target_link_libraries(cuno game)
else()
	message(STATUS "Platform not implemented")
endif()
