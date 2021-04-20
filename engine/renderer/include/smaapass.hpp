#pragma once

#include "common.hpp"

class Scene;
class GFX;
class GFXCommandBuffer;
class GFXFramebuffer;
class GFXPipeline;
class GFXRenderPass;
class GFXTexture;
class RenderTarget;

namespace prism {
    class renderer;
}

class SMAAPass {
public:
    SMAAPass(GFX* gfx, prism::renderer* renderer);
    
    void create_render_target_resources(RenderTarget& target);
    
    void render(GFXCommandBuffer* command_buffer, RenderTarget& target);

private:
    void create_textures();
    void create_render_pass();
    void create_pipelines();

    prism::renderer* renderer = nullptr;
    
    GFXTexture* area_image = nullptr;
    GFXTexture* search_image = nullptr;
    
    GFXRenderPass* render_pass = nullptr;
    
    GFXPipeline* edge_pipeline = nullptr;
    GFXPipeline* blend_pipeline = nullptr;
};
