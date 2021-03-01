include(../../cmake/AddPlatformExecutable.cmake)

add_platform(
        SRC ${CMAKE_CURRENT_SOURCE_DIR}/file.cpp
        MAIN_FILE
        main.cpp.in
        LINK_LIBRARIES
        SDL2
        Core
        GFXVulkan
)

function(add_platform_commands target)
    if(NOT SKIP_DATA)
        add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:${target}>/data)
    endif()

    add_custom_command(TARGET ${target} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target}>/shaders)

    add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/shaders $<TARGET_FILE_DIR:${target}>/shaders)
endfunction()
