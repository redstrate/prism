macro(compile_shader src)
    string(REGEX REPLACE "\\.[^.]*$" "" MYFILE_WITHOUT_EXT ${src})

    set(SHADER_COMPILER_COMMAND "${CMAKE_BINARY_DIR}/bin/Debug/ShaderCompiler")
    if(ENABLE_IOS)
	set(SHADER_COMPILER_COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../build/bin/Debug/ShaderCompiler")
    endif()

    set(EXTRA_PLATFORM_ARG "0")
    if(ENABLE_IOS)
	set(EXTRA_PLATFORM_ARG "1")
    endif()

    add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/${MYFILE_WITHOUT_EXT}.glsl
    COMMAND ${SHADER_COMPILER_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/../../${src} ${CMAKE_BINARY_DIR}/${src} ${EXTRA_PLATFORM_ARG}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../../${src}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../shaders
    )
endmacro()

function(add_shader)
    if(NOT ENABLE_IOS)
        set(options OPTIONAL FAST)
        set(oneValueArgs TARGET)
        set(multiValueArgs SHADERS)
        cmake_parse_arguments(add_shader "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

        foreach(shader ${add_shader_SHADERS})
            string(REGEX REPLACE "\\.[^.]*$" "" MYFILE_WITHOUT_EXT ${shader})

            compile_shader(${shader})

            list(APPEND SPV_FILES ${CMAKE_BINARY_DIR}/${MYFILE_WITHOUT_EXT}.glsl)
        endforeach()

        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

        add_custom_target(BuildShaders DEPENDS ${SPV_FILES} ShaderCompiler)
        add_dependencies(${add_shader_TARGET} BuildShaders)
        
        set(ALL_SHADER_FILES ${SPV_FILES} CACHE INTERNAL "" FORCE)
    endif()
endfunction()
