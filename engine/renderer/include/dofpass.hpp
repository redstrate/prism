#pragma once

class GFX;
class GFXCommandBuffer;
class GFXFramebuffer;
class GFXPipeline;
class GFXRenderPass;
class GFXTexture;
class Scene;

namespace prism {
    class renderer;
}

class DoFPass {
public:
    DoFPass(GFX* gfx, prism::renderer* renderer);
        
    void render(GFXCommandBuffer* command_buffer, Scene& scene);
    
    GFXTexture* far_field = nullptr;
    GFXTexture* normal_field = nullptr;
    
    GFXFramebuffer* far_framebuffer = nullptr;
    GFXFramebuffer* normal_framebuffer = nullptr;
    
    GFXRenderPass* renderpass = nullptr;
    
private:
    prism::renderer* renderer = nullptr;
    
    GFXPipeline* pipeline = nullptr;
};
