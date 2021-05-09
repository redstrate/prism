# add custom command to output compiled shader
function(add_shader_command)
    cmake_parse_arguments(ARGS "" "FILENAME;OUT" "" ${ARGN})

    cmake_path(REMOVE_EXTENSION ARGS_FILENAME LAST_ONLY OUTPUT_VARIABLE FILENAME_WITHOUT_EXT)

    set(SHADER_COMPILER_COMMAND $<TARGET_FILE:ShaderCompilerTool>)
    set(EXTRA_PLATFORM_ARG "0")

    # if targeting ios, we want to use the host's shader compiler
    if(ENABLE_IOS)
        set(SHADER_COMPILER_COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../build/bin/Debug/ShaderCompilerTool")
        set(EXTRA_PLATFORM_ARG "1")
    endif()

    set(SRC_FILENAME ${CMAKE_CURRENT_SOURCE_DIR}/${ARGS_FILENAME})
    set(OUTPUT_FILENAME ${CMAKE_BINARY_DIR}/${FILENAME_WITHOUT_EXT})

    # we want to remove the .nocompile extension just like the tool does, if it exists
    cmake_path(GET FILENAME_WITHOUT_EXT EXTENSION LAST_ONLY LAST_EXT)

    if(LAST_EXT STREQUAL ".nocompile")
        cmake_path(REPLACE_EXTENSION OUTPUT_FILENAME LAST_ONLY glsl)
    endif()

    add_custom_command(
        OUTPUT ${OUTPUT_FILENAME}
        COMMAND ${SHADER_COMPILER_COMMAND} ${SRC_FILENAME} ${OUTPUT_FILENAME} ${EXTRA_PLATFORM_ARG}
        DEPENDS ${SRC_FILENAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

    # return the actual filename
    set(${ARGS_OUT} ${OUTPUT_FILENAME} PARENT_SCOPE)
endfunction()

# add shaders to target
function(add_shaders)
    if(NOT ENABLE_IOS)
        cmake_parse_arguments(ARGS "" "TARGET" "SHADERS" ${ARGN})

        foreach(SHADER_FILENAME ${ARGS_SHADERS})
            cmake_path(REMOVE_EXTENSION SHADER_FILENAME LAST_ONLY OUTPUT_VARIABLE FILENAME_WITHOUT_EXT)

            add_shader_command(FILENAME ${SHADER_FILENAME} OUT DST_FILENAME)

            list(APPEND SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILENAME})
            list(APPEND DST_FILES ${DST_FILENAME})
        endforeach()

        add_custom_target(BuildShaders DEPENDS ShaderCompilerTool ${DST_FILES})
        add_dependencies(${ARGS_TARGET} BuildShaders)
    endif()
endfunction()

# make the shader directory if it doesnt exist already
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)
