message(STATUS "Preparing to package APK...")

if (NO_GUI)
    add_executable(cuno)
    target_link_libraries(cuno game)
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

    add_library(cuno SHARED)
    target_link_libraries(cuno game)
    add_dependencies(cuno outputdir)

    # copy asan, call to aapt2, apksign, zipalign
    include(apk.cmake)
endif()
