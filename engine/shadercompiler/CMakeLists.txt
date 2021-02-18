set(SRC
    include/shadercompiler.hpp

    src/shadercompiler.cpp
    src/includer.hpp
    src/defaultresources.hpp)

add_library(ShaderCompiler STATIC ${SRC})
target_link_libraries(ShaderCompiler
    PUBLIC
    Platform
    PRIVATE
    Utility
    Log
    ${CROSS_LIBS})
target_include_directories(ShaderCompiler PUBLIC include PRIVATE src)
set_engine_properties(ShaderCompiler)
