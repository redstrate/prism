#pragma once

#include "common.hpp"
#include "render_options.hpp"

class GFXTexture;
class GFXFramebuffer;
class GFXBuffer;

class RenderTarget {
public:
    prism::Extent extent;
    
    prism::Extent get_render_extent() const {        
        return {static_cast<uint32_t>(std::max(int(extent.width * render_options.render_scale), 1)),
            static_cast<uint32_t>(std::max(int(extent.height * render_options.render_scale), 1))};
    }
    
    // offscreen
    GFXTexture* offscreenColorTexture = nullptr;
    GFXTexture* offscreenDepthTexture = nullptr;
    
    GFXFramebuffer* offscreenFramebuffer = nullptr;
    
    // mesh
    GFXBuffer* sceneBuffer = nullptr;
    
    // smaa
    GFXTexture* edge_texture = nullptr;
    GFXTexture* blend_texture = nullptr;

    GFXFramebuffer* edge_framebuffer = nullptr;
    GFXFramebuffer* blend_framebuffer = nullptr;
};
