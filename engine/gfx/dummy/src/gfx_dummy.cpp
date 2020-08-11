#include "gfx_dummy.hpp"

bool GFXDummy::initialize() {
	return true;
}

void GFXDummy::initializeView(void* native_handle, uint32_t width, uint32_t height) {
       
}

GFXBuffer* GFXDummy::createBuffer(void* data, GFXSize size, GFXBufferUsage usage) {
    return nullptr;
}

void GFXDummy::copyBuffer(GFXBuffer* buffer, void* data, GFXSize offset, GFXSize size) {

}

GFXTexture* GFXDummy::createTexture(uint32_t width, uint32_t height, GFXPixelFormat format, GFXStorageMode storageMode, GFXTextureUsage usage) {
    return nullptr;
}

void GFXDummy::copyTexture(GFXTexture* texture, void* data, GFXSize size) {

}


GFXFramebuffer* GFXDummy::createFramebuffer(GFXFramebufferCreateInfo& info) {
    return nullptr;
}

GFXRenderPass* GFXDummy::createRenderPass(GFXRenderPassCreateInfo& info) {
    return nullptr;
}

GFXPipeline* GFXDummy::createPipeline(GFXPipelineCreateInfo& info) {
    return nullptr;
}

void GFXDummy::render(GFXCommandBuffer* command_buffer) {

}

const char* GFXDummy::getName() {
    return "None";
}
