cmake_minimum_required(VERSION 3.10)
project(cuno VERSION 1.0)
set(C_STANDARD 90)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory("game")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	message(STATUS "Android Build")
	add_subdirectory("platform/android")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Emscriptn")
	message(STATUS "WASM Build")
	add_subdirectory("platform/wasm")
else()
	message(STATUS "Termux Test Build not implemented")
endif()
