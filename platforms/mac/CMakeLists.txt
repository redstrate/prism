include(../../cmake/AddPlatformExecutable.cmake)

add_platform(
    MAIN_FILE
    main.mm.in
    SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/file.mm
    EXECUTABLE_PROPERTIES
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    LINK_LIBRARIES
    Core
    GFXMetal
    "-framework Cocoa"
    "-framework QuartzCore"
    "-framework GameController"
    COMPILE_OPTIONS
    -std=c++17
)

function(add_platform_commands target)
	target_link_options(${target} BEFORE PRIVATE -pagezero_size 10000 -image_base 100000000
)
	if(NOT SKIP_DATA)
		add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:${target}>/../Resources/data)
	endif()

	add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/shaders $<TARGET_FILE_DIR:${target}>/../Resources/shaders)
endfunction()
