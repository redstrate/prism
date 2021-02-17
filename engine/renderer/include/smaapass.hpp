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
class RenderTarget;

class SMAAPass {
public:
    SMAAPass(GFX* gfx, Renderer* renderer);
    
    void create_render_target_resources(RenderTarget& target);
    
    void render(GFXCommandBuffer* command_buffer, RenderTarget& target);

private:
    void create_textures();
    void create_render_pass();
    void create_pipelines();
    
    Renderer* renderer = nullptr;
    
    GFXTexture* area_image = nullptr;
    GFXTexture* search_image = nullptr;
    
    GFXRenderPass* render_pass = nullptr;
    
    GFXPipeline* edge_pipeline = nullptr;
    GFXPipeline* blend_pipeline = nullptr;
};
