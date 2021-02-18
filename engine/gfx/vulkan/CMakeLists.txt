set(HEADERS
    include/gfx_vulkan.hpp
    src/gfx_vulkan_buffer.hpp
    src/gfx_vulkan_pipeline.hpp
    src/gfx_vulkan_texture.hpp
    src/gfx_vulkan_framebuffer.hpp
	src/gfx_vulkan_renderpass.hpp
	src/gfx_vulkan_commandbuffer.hpp)

find_package(Vulkan REQUIRED)

add_library(GFXVulkan STATIC
    src/gfx_vulkan.cpp
    ${HEADERS})

target_link_libraries(GFXVulkan PUBLIC
    GFX
	Vulkan::Vulkan
	Core
	Log)
target_include_directories(GFXVulkan PUBLIC
    include
    PRIVATE
    src)
set_target_properties(GFXVulkan PROPERTIES
    CXX_STANDARD 17)
