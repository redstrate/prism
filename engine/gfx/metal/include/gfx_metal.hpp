#pragma once

#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>

#include "gfx.hpp"

class GFXMetal : public GFX {
public:
    bool is_supported() override;
    GFXContext required_context() override { return GFXContext::Metal; }
    ShaderLanguage accepted_shader_language() override { return ShaderLanguage::MSL; }
    const char* get_name() override;
    
    bool supports_feature(const GFXFeature feature) override;
    
    bool initialize(const GFXCreateInfo& createInfo) override;

    void initialize_view(void* native_handle, const int identifier, const uint32_t width, const uint32_t height) override;
    void remove_view(const int identifier) override;

    // buffer operations
    GFXBuffer* create_buffer(void* data, const GFXSize size, const bool dynamicData, const GFXBufferUsage usage) override;
    void copy_buffer(GFXBuffer* buffer, void* data, const GFXSize offset, const GFXSize size) override;
    
    void* get_buffer_contents(GFXBuffer* buffer) override;
    
    // texture operations
    GFXTexture* create_texture(const GFXTextureCreateInfo& info) override;
    void copy_texture(GFXTexture* texture, void* data, GFXSize size) override;
    void copy_texture(GFXTexture* from, GFXTexture* to) override;
    void copy_texture(GFXTexture* from, GFXBuffer* to) override;
    
    // sampler opeations
    GFXSampler* create_sampler(const GFXSamplerCreateInfo& info) override;

    // framebuffer operations
    GFXFramebuffer* create_framebuffer(const GFXFramebufferCreateInfo& info) override;
    
    // render pass operations
    GFXRenderPass* create_render_pass(const GFXRenderPassCreateInfo& info) override;
    
    // pipeline operations
    GFXPipeline* create_graphics_pipeline(const GFXGraphicsPipelineCreateInfo& info) override;
    GFXPipeline* create_compute_pipeline(const GFXComputePipelineCreateInfo& info) override;

    GFXCommandBuffer* acquire_command_buffer() override;
    
    void submit(GFXCommandBuffer* command_buffer, const int window = -1) override;
        
private:
    struct NativeMTLView {
        int identifier = -1;
        CAMetalLayer* layer = nullptr;
    };
    
    std::vector<NativeMTLView*> nativeViews;
    
    NativeMTLView* getNativeView(int identifier) {
        for(auto& view : nativeViews) {
            if(view->identifier == identifier)
                return view;
        }
        
        return nullptr;
    }
    
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> command_queue = nil;
};
