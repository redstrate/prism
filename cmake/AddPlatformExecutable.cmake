function(set_output_dir target)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        XCODE_GENERATE_SCHEME ON)
endfunction()

function(add_platform_executable)
    cmake_parse_arguments(add_platform_executable
        ""
        "TARGET;APP_CLASS;APP_INCLUDE;APP_NAME;SKIP_DATA"
        "SRC"
        ${ARGN})

    set(APP_CLASS ${add_platform_executable_APP_CLASS} CACHE INTERNAL "")
    set(GAME_SRC ${add_platform_executable_SRC} CACHE INTERNAL "")
    set(APP_INCLUDE ${add_platform_executable_APP_INCLUDE} CACHE INTERNAL "")
    set(APP_NAME ${add_platform_executable_APP_NAME} CACHE INTERNAL "")

    string(REGEX MATCH "^(.*)\\.[^.]*$" dummy ${PLATFORM_MAIN_FILE_NAME})
    set(MYFILE_WITHOUT_EXT ${CMAKE_MATCH_1})

    configure_file(${PLATFORM_MAIN_FILE_PATH} ${CMAKE_BINARY_DIR}/${add_platform_executable_TARGET}${MYFILE_WITHOUT_EXT})

    if(COMMAND setup_target)
        setup_target(${add_platform_executable_TARGET})
    endif()

    add_executable(${add_platform_executable_TARGET}
        ${PLATFORM_SOURCES}
        ${add_platform_executable_SRC}
        ${CMAKE_BINARY_DIR}/${add_platform_executable_TARGET}${MYFILE_WITHOUT_EXT})

    if("${PLATFORM_EXECUTABLE_PROPERTIES}" STREQUAL "")
    else()
        set_target_properties(${add_platform_executable_TARGET}
            PROPERTIES
            ${PLATFORM_EXECUTABLE_PROPERTIES}
            )
    endif()

    target_link_libraries(${add_platform_executable_TARGET}
        PRIVATE
        ${PLATFORM_LINK_LIBRARIES}
        )

    if("${add_platform_executable_APP_NAME}")
        set_target_properties(${add_platform_executable_TARGET} PROPERTIES XCODE_ATTRIBUTE_EXECUTABLE_NAME ${add_platform_executable_APP_NAME})
        set_target_properties(${add_platform_executable_TARGET} PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME ${add_platform_executable_APP_NAME})
    endif()

    if(COMMAND add_platform_commands)
        add_platform_commands(${add_platform_executable_TARGET})
    endif()

    set_output_dir(${add_platform_executable_TARGET})
endfunction()

function(add_platform)
    cmake_parse_arguments(add_platform
        "$"
        "MAIN_FILE;DISPLAY_NAME"
        "EXECUTABLE_PROPERTIES;LINK_LIBRARIES;COMPILE_OPTIONS;SRC"
        ${ARGN})

    set(PLATFORM_MAIN_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${add_platform_MAIN_FILE} CACHE INTERNAL "")
    set(PLATFORM_MAIN_FILE_NAME ${add_platform_MAIN_FILE} CACHE INTERNAL "")
    set(PLATFORM_LINK_LIBRARIES ${add_platform_LINK_LIBRARIES} CACHE INTERNAL "")
    set(PLATFORM_EXECUTABLE_PROPERTIES ${add_platform_EXECUTABLE_PROPERTIES} CACHE INTERNAL "")
    set(PLATFORM_SOURCES ${add_platform_SRC} CACHE INTERNAL "")
endfunction()
