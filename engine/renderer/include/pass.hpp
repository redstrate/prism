#pragma once

#include "common.hpp"

class GFXCommandBuffer;
class Scene;

enum class PassTextureType {
    SelectionSobel
};

class GFXTexture;
class RenderTarget;

class Pass {
public:
    virtual ~Pass() {}
    
    virtual void initialize() {}
    
    virtual void create_render_target_resources([[maybe_used]] RenderTarget& target) {}

	virtual void render_scene([[maybe_unused]] Scene& scene,
                             [[maybe_unused]] GFXCommandBuffer* commandBuffer) {}
    virtual void render_post([[maybe_unused]] GFXCommandBuffer* commandBuffer,
                             [[maybe_unused]] RenderTarget& target,
                             [[maybe_unused]] int index) {}
    
    virtual GFXTexture* get_requested_texture([[maybe_unused]] PassTextureType type) { return nullptr; }
};
