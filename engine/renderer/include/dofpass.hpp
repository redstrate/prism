#pragma once

class GFX;
class GFXCommandBuffer;
class GFXFramebuffer;
class GFXPipeline;
class GFXRenderPass;
class GFXTexture;
class Scene;
class Renderer;

class DoFPass {
public:
    DoFPass(GFX* gfx, Renderer* renderer);
        
    void render(GFXCommandBuffer* command_buffer, Scene& scene);
    
private:
    Renderer* renderer = nullptr;
    
    GFXPipeline* pipeline = nullptr;
};
