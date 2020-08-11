macro(set_engine_properties target)
    target_compile_features(${target} PUBLIC cxx_std_17)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    target_compile_options(${target} PUBLIC
        -Wall
        -Wextra
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-padded
        -Wno-documentation-unknown-command
        -Wno-used-but-marked-unused
        -Wno-system-headers
        -Wconversion
        -Wno-sign-conversion)
    
    if(ENABLE_MACOS)
        target_compile_definitions(${target} PUBLIC PLATFORM_MACOS)
    endif()
    
    if(ENABLE_WINDOWS)
        target_compile_definitions(${target} PUBLIC PLATFORM_WINDOWS)
    endif()

    if(ENABLE_LINUX)
        target_compile_definitions(${target} PUBLIC PLATFORM_LINUX)
    endif()
    
    if(ENABLE_IOS)
        target_compile_definitions(${target} PUBLIC PLATFORM_IOS)
    endif()
    
   if(ENABLE_TVOS)
        target_compile_definitions(${target} PUBLIC PLATFORM_TVOS)
    endif()
endmacro()
