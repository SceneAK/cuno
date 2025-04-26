# Copy app template to build directory
set(OUTPUT_APP_DIR "${CMAKE_BINARY_DIR}/app")
set(COPY_STAMP "${OUTPUT_APP_DIR}/.cp-stmp")
add_custom_command(
    OUTPUT  ${COPY_STAMP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_APP_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/app_template
            ${OUTPUT_APP_DIR}
	    COMMAND ${CMAKE_COMMAND} -E touch ${COPY_STAMP}
    COMMENT "Copying app template to build directory"
)
add_custom_target(outputdir DEPENDS ${COPY_STAMP})

# Cuno Shared Library
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_APP_DIR}/lib/${ANDROID_ABI})
add_library(cuno SHARED ${CMAKE_CURRENT_SOURCE_DIR}/src/cunoandr.c) 
add_dependencies(cuno outputdir)

# Android Glue Object Compile
set(GLUE_DIR "${ANDROID_NDK}/sources/android/native_app_glue")
target_include_directories(cuno PUBLIC ${GLUE_DIR})
add_library(android_glue OBJECT "${GLUE_DIR}/android_native_app_glue.c")
target_link_libraries(cuno src android log android_glue)

# call to aapt2, apksign, zipalign
include(apk.cmake)
