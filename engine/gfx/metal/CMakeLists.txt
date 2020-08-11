set(HEADERS
    include/gfx_metal.hpp
    src/gfx_metal_buffer.hpp
    src/gfx_metal_pipeline.hpp
    src/gfx_metal_texture.hpp
    src/gfx_metal_framebuffer.hpp
    src/gfx_metal_renderpass.hpp
    src/gfx_metal_sampler.hpp)

add_library(GFXMetal STATIC
    src/gfx_metal.mm
    ${HEADERS})

set_target_properties(GFXMetal PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION OBJC
    PUBLIC_HEADER "${HEADERS}"
)
target_link_libraries(GFXMetal PUBLIC
    GFX
    Core
    Log
    "-framework Metal")
target_include_directories(GFXMetal PUBLIC
    include
    PRIVATE
    src)
set_engine_properties(GFXMetal)
