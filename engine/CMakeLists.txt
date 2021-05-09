set(CMAKE_FOLDER "Engine")

add_subdirectory(core)
add_subdirectory(renderer)
add_subdirectory(utility)
add_subdirectory(gfx)
add_subdirectory(math)
add_subdirectory(log)
add_subdirectory(asset)
add_subdirectory(shadercompiler)
add_subdirectory(platform)

if(NOT ENABLE_IOS AND NOT ENABLE_TVOS)
    add_subdirectory(audio)
    add_subdirectory(tests)
endif()

include(../cmake/BuildShaders.cmake)

add_shaders(TARGET Renderer
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
        shaders/dof.frag.glsl
        shaders/histogram.comp.glsl
        shaders/histogram-average.comp.glsl)
