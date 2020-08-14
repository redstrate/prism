include(../../cmake/AddPlatformExecutable.cmake)

add_platform(
    MAIN_FILE
    main.cpp.in
    SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/file.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/windows.cpp
    EXECUTABLE_PROPERTIES
    LINK_LIBRARIES
    Core
    GFXVulkan
)
add_custom_target(PlatformWindows_IDE SOURCES
    main.cpp.in)

function(add_platform_commands target)
    set_target_properties(${target} PROPERTIES
    LINK_FLAGS /SUBSYSTEM:WINDOWS)

	add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/shaders $<TARGET_FILE_DIR:${target}>/shaders)
    add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:${target}>/data)
endfunction()
