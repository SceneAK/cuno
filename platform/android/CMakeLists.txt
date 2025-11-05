if (NO_GUI)
    add_executable(cuno
        ${CMAKE_SOURCE_DIR}/game/src/cli.c

        ${CMAKE_SOURCE_DIR}/game/src/system/asset_stdio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/log_andr.c
    )
    target_link_libraries(cuno 
        base 
        log 
    )
else()
    # Copy app to build directory
    set(OUTPUT_APP_DIR "${CMAKE_BINARY_DIR}/app")
    add_custom_target(APP_TEMPLATE_COPY
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_APP_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/app
                ${OUTPUT_APP_DIR}
        COMMENT "Copying app to build directory"
    )
    add_custom_target(outputdir DEPENDS APP_TEMPLATE_COPY)

    # Cuno Shared Library
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_APP_DIR}/lib/${ANDROID_ABI})

    add_library(cuno SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main_andr.c
        ${CMAKE_SOURCE_DIR}/game/src/gui.c

        ${CMAKE_SOURCE_DIR}/game/src/component.c
        ${CMAKE_SOURCE_DIR}/game/src/system/graphic/egl.c
        ${CMAKE_SOURCE_DIR}/game/src/system/graphic/text.c

        ${CMAKE_CURRENT_SOURCE_DIR}/src/asset_andr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/log_andr.c
    )

    find_library(GLES NAMES GLESv2 REQUIRED)
    find_library(EGL NAMES EGL REQUIRED)
    add_dependencies(cuno outputdir)

    # Android Glue Object Compile
    set(GLUE_DIR "${ANDROID_NDK}/sources/android/native_app_glue")
    target_include_directories(cuno PRIVATE ${GLUE_DIR})
    add_library(android_glue OBJECT "${GLUE_DIR}/android_native_app_glue.c")

    target_link_libraries(cuno 
        base 
        android 
        log 
        android_glue
        ${GLES}
        ${EGL}
    )

    # copy asan, call to aapt2, apksign, zipalign
    include(apk.cmake)
endif()
