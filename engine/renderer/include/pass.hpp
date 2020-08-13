#pragma once

#include "common.hpp"

class GFXCommandBuffer;
class Scene;

enum class PassTextureType {
    SelectionSobel
};

class GFXTexture;

class Pass {
public:
    virtual ~Pass() {}
    
    virtual void initialize() {}
    
    virtual void resize([[maybe_unused]] const prism::Extent extent) {}

	virtual void render_scene([[maybe_unused]] Scene& scene,
                             [[maybe_unused]] GFXCommandBuffer* commandBuffer) {}
    virtual void render_post([[maybe_unused]] GFXCommandBuffer* commandBuffer,
                            [[maybe_unused]] int index) {}
    
    virtual GFXTexture* get_requested_texture([[maybe_unused]] PassTextureType type) { return nullptr; }
};
