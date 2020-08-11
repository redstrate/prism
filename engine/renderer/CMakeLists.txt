set(SRC
    include/renderer.hpp
    include/font.hpp
    include/pass.hpp
    include/gaussianhelper.hpp
    include/shadowpass.hpp
    include/imguipass.hpp
    include/smaapass.hpp
    include/scenecapture.hpp
    include/shadercompiler.hpp
    include/DirStackIncluder.h
    include/dofpass.hpp

    src/renderer.cpp
    src/gaussianhelper.cpp
    src/shadowpass.cpp
    src/imguipass.cpp
    src/smaapass.cpp
    src/scenecapture.cpp
    src/shadercompiler.cpp
    src/dofpass.cpp)
    
if(NOT ENABLE_IOS AND NOT ENABLE_TVOS)
    find_package(spirv_cross_core REQUIRED)
    find_package(spirv_cross_glsl REQUIRED)
    find_package(spirv_cross_cpp REQUIRED)
    find_package(spirv_cross_msl REQUIRED)
    find_package(glslang REQUIRED)

    set(CROSS_LIBS
        glslang::glslang
        glslang::SPIRV
        glslang::OSDependent
        glslang::OGLCompiler
        glslang::HLSL
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-cpp
        spirv-cross-msl
    )
else()
    set(CROSS_LIBS
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-cpp
        spirv-cross-msl
        glslang
        SPIRV
    )
endif()

add_library(Renderer STATIC ${SRC})
target_link_libraries(Renderer
    PUBLIC
    GFX
    Math
    PRIVATE
    stb
    Math
    Utility
    Core
    imgui
    SMAA::SMAA
    ${CROSS_LIBS})
target_include_directories(Renderer PUBLIC include)
set_target_properties(Renderer PROPERTIES CXX_STANDARD 17)
target_compile_features(Renderer PUBLIC cxx_std_17)
set_target_properties(Renderer PROPERTIES CXX_EXTENSIONS OFF)

include(../../cmake/BuildShaders.cmake)

add_shader(TARGET Renderer
    SHADERS
    shaders/mesh.vert.nocompile.glsl
    shaders/post.vert.glsl
    shaders/post.frag.glsl
    shaders/text.vert.glsl
    shaders/text.frag.glsl
    shaders/ui.vert.glsl
    shaders/ui.frag.glsl
    shaders/imgui.vert.glsl
    shaders/imgui.frag.glsl
    shaders/debug.vert.glsl
    shaders/debug.frag.glsl
    shaders/gaussian.vert.glsl
    shaders/gaussian.frag.glsl
    shaders/shadow.vert.nocompile.glsl
    shaders/shadow.frag.glsl
    shaders/color.vert.glsl
    shaders/color.frag.glsl
    shaders/omnishadow.frag.glsl
    shaders/edge.vert.glsl
    shaders/edge.frag.glsl
    shaders/blend.vert.glsl
    shaders/blend.frag.glsl
    shaders/sky.vert.glsl
    shaders/sky.frag.glsl
    shaders/billboard.vert.glsl
    shaders/billboard.frag.glsl
    shaders/scenecapture.vert.nocompile.glsl
    shaders/irradiance.vert.glsl
    shaders/irradiance.frag.glsl
    shaders/filter.vert.glsl
    shaders/filter.frag.glsl
    shaders/brdf.vert.glsl
    shaders/brdf.frag.glsl
    shaders/rendering.nocompile.glsl
    shaders/common.nocompile.glsl
    shaders/dof.vert.glsl
    shaders/dof.frag.glsl)
