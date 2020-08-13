#pragma once

#include "common.hpp"

class Scene;
class GFX;
class GFXCommandBuffer;
class GFXFramebuffer;
class GFXPipeline;
class GFXRenderPass;
class GFXTexture;
class Renderer;

class SMAAPass {
public:
    SMAAPass(GFX* gfx, Renderer* renderer);
    
    void render(GFXCommandBuffer* command_buffer);
    
    GFXTexture* edge_texture = nullptr;
    GFXTexture* blend_texture = nullptr;

private:
    void create_textures();
    void create_render_pass();
    void create_pipelines();
    void create_offscreen_resources();
    
    prism::Extent extent;
    Renderer* renderer = nullptr;
    
    GFXTexture* area_image = nullptr;
    GFXTexture* search_image = nullptr;
    
    GFXRenderPass* render_pass = nullptr;
    
    GFXPipeline* edge_pipeline = nullptr;
    GFXPipeline* blend_pipeline = nullptr;
    
    GFXFramebuffer* edge_framebuffer = nullptr;
    
    GFXFramebuffer* blend_framebuffer = nullptr;
};
