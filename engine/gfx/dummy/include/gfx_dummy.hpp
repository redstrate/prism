#pragma once

#include "gfx.hpp"

class GFXDummy : public GFX {
public:
    bool initialize() override;
	void initializeView(void* native_handle, uint32_t width, uint32_t height) override;

    // buffer operations
    GFXBuffer* createBuffer(void* data, GFXSize size, GFXBufferUsage usage) override;
    void copyBuffer(GFXBuffer* buffer, void* data, GFXSize offset, GFXSize size) override;
    
    // texture operations
    GFXTexture* createTexture(uint32_t width, uint32_t height, GFXPixelFormat format, GFXStorageMode storageMode, GFXTextureUsage usage) override;
    void copyTexture(GFXTexture* texture, void* data, GFXSize size) override;
    
    // framebuffer operations
    GFXFramebuffer* createFramebuffer(GFXFramebufferCreateInfo& info) override;
    
    // render pass operations
    GFXRenderPass* createRenderPass(GFXRenderPassCreateInfo& info) override;
    
    // pipeline operations
    GFXPipeline* createPipeline(GFXPipelineCreateInfo& info) override;
    
    void render(GFXCommandBuffer* command_buffer) override;
    
    const char* getName() override;
};
