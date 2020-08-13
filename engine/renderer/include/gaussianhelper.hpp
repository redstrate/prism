#pragma once

#include "common.hpp"

class GFX;
class GFXCommandBuffer;
class GFXFramebuffer;
class GFXPipeline;
class GFXRenderPass;
class GFXTexture;

class GaussianHelper {
public:
    GaussianHelper(GFX* gfx, const prism::Extent extent);
    
    GFXTexture* render(GFXCommandBuffer* commandBuffer,GFXTexture* source);
    
    GFXPipeline* pipeline = nullptr;
    
    GFXRenderPass* renderPass = nullptr;
    
    GFXTexture* texA = nullptr, *texB = nullptr;
    GFXFramebuffer* fboA = nullptr, *fboB = nullptr;
    
private:
    prism::Extent extent;
};
