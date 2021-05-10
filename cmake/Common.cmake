macro(set_engine_properties target)
    target_compile_features(${target} PUBLIC cxx_std_17)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)

    if(ENABLE_MACOS OR ENABLE_LINUX)
        target_compile_options(${target} PUBLIC
            -fpermissive) # ew but required for now TODO: remove and test!
    endif()
    
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
