set(CMAKE_FOLDER "GFX Backends")

add_library(GFX INTERFACE)
target_include_directories(GFX INTERFACE public)
target_link_libraries(GFX INTERFACE Utility ShaderCompiler)

add_custom_target(GFXInterface SOURCES
    public/gfx.hpp
    public/gfx_buffer.hpp
    public/gfx_pipeline.hpp
    public/gfx_commandbuffer.hpp
    public/gfx_texture.hpp
    public/gfx_framebuffer.hpp
    public/gfx_renderpass.hpp
    public/gfx_object.hpp
    public/gfx_sampler.hpp)
set_target_properties(GFXInterface PROPERTIES CMAKE_FOLDER "GFX")

if(ENABLE_METAL)
    add_subdirectory(metal)
endif()

if(ENABLE_VULKAN)
   add_subdirectory(vulkan)
endif()
