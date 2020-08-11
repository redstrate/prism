set(CMAKE_TOOLCHAIN_FILE "")

find_package(spirv_cross_core REQUIRED)
find_package(spirv_cross_glsl REQUIRED)
find_package(spirv_cross_cpp REQUIRED)
find_package(spirv_cross_msl REQUIRED)
find_package(glslang REQUIRED)

add_executable(ShaderCompiler main.cpp)
target_link_libraries(ShaderCompiler
    PUBLIC
    glslang::glslang
    glslang::SPIRV
    glslang::OSDependent
    glslang::OGLCompiler
    glslang::HLSL
    spirv-cross-core
    spirv-cross-glsl
    spirv-cross-cpp
    spirv-cross-msl
	Log)
set_engine_properties(ShaderCompiler)
set_output_dir(ShaderCompiler)
