include(../../cmake/AddPlatformExecutable.cmake)

add_platform(
        SRC ${CMAKE_CURRENT_SOURCE_DIR}/file.cpp
        MAIN_FILE
        main.cpp.in
        LINK_LIBRARIES
        SDL2::Main
        Core
        GFXVulkan
)

function(add_platform_commands target)
    if(NOT SKIP_DATA)
        add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:${target}>/data)
    endif()

    # we HAVE to create this dummy target to convince CMake to properly copy over shader files.
    # before you ask, we have used POST_BUILD before but that only runs if the TARGET specified is built.
    # when you change a shader source file on disk, BuildShaders is triggered but that doesn't retrigger your actual
    # app target to be rebuilt, so the shaders are never copied correctly. With this (dumb) system, we ensure they
    # always are. WHY CMAKE WHY
    set(DUMMY_NAME ${target}-CopyShaders)

    add_custom_target(${DUMMY_NAME} ALL DEPENDS ${CMAKE_BINARY_DIR}/dummy)

    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/dummy
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target}>/shaders
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/shaders $<TARGET_FILE_DIR:${target}>/shaders
            )

    add_dependencies(${target} ${DUMMY_NAME})
endfunction()
