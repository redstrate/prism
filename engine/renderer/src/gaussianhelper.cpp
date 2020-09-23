#include "gaussianhelper.hpp"

#include "gfx_commandbuffer.hpp"
#include "gfx.hpp"

GaussianHelper::GaussianHelper(GFX* gfx, const prism::Extent extent) : extent(extent) {
    // render pass
    GFXRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.label = "Gaussian";
    renderPassInfo.attachments.push_back(GFXPixelFormat::RGBA_32F);

    renderPass = gfx->create_render_pass(renderPassInfo);

    // pipeline
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "Gaussian";
    pipelineInfo.shaders.vertex_path = "gaussian.vert";
    pipelineInfo.shaders.fragment_path = "gaussian.frag";

    pipelineInfo.rasterization.primitive_type = GFXPrimitiveType::TriangleStrip;

    pipelineInfo.shader_input.bindings = {
        {0, GFXBindingType::Texture},
        {1, GFXBindingType::PushConstant}
    };

    pipelineInfo.shader_input.push_constants = {
        {4, 0}
    };

    pipelineInfo.render_pass = renderPass;

    pipeline = gfx->create_graphics_pipeline(pipelineInfo);

    // resources
    GFXTextureCreateInfo textureInfo = {};
    textureInfo.width = extent.width;
    textureInfo.height = extent.height;
    textureInfo.format = GFXPixelFormat::RGBA_32F;
    textureInfo.usage = GFXTextureUsage::Attachment | GFXTextureUsage::Sampled;
    textureInfo.samplingMode = SamplingMode::ClampToEdge;

    texA = gfx->create_texture(textureInfo);
    texB = gfx->create_texture(textureInfo);

    GFXFramebufferCreateInfo info;
    info.attachments = {texA};
    info.render_pass = renderPass;

    fboA = gfx->create_framebuffer(info);

    info.attachments = {texB};

    fboB = gfx->create_framebuffer(info);
}

GFXTexture* GaussianHelper::render(GFXCommandBuffer* commandBuffer, GFXTexture* source) {
    bool horizontal = true, first_iteration = true;
    int amount = 10;
    for(int i = 0; i < amount; i++) {
        GFXRenderPassBeginInfo info = {};
        info.framebuffer = horizontal ? fboA : fboB;
        info.render_pass = renderPass;
        info.render_area.extent = extent;

        commandBuffer->set_render_pass(info);

        commandBuffer->set_graphics_pipeline(pipeline);

        commandBuffer->memory_barrier();

        int h = static_cast<int>(horizontal);
        commandBuffer->set_push_constant(&h, sizeof(int));
        commandBuffer->bind_texture(first_iteration ? source : (horizontal ? texB : texA), 0);

        commandBuffer->draw(0, 4, 0, 1);

        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }

    return (horizontal ? texB : texA);
}
