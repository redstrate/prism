#include "dofpass.hpp"

#include "renderer.hpp"
#include "gfx.hpp"
#include "gfx_commandbuffer.hpp"
#include "engine.hpp"
#include "asset.hpp"

AssetPtr<Texture> aperture_texture;

DoFPass::DoFPass(GFX* gfx, Renderer* renderer) : renderer(renderer) {
    aperture_texture = assetm->get<Texture>(file::app_domain / "textures/aperture.png");
    
    GFXRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.label = "Depth of Field";
    renderPassInfo.attachments.push_back(GFXPixelFormat::RGBA_32F);
    
    renderpass = gfx->create_render_pass(renderPassInfo);
    
    //const auto extent = renderer->get_render_extent();
    const auto extent = prism::Extent();
    
    GFXShaderConstant width_constant = {};
    width_constant.value = extent.width;
    
    GFXShaderConstant height_constant = {};
    height_constant.index = 1;
    height_constant.value = extent.height;
    
    GFXGraphicsPipelineCreateInfo create_info = {};
    create_info.shaders.vertex_src = file::Path("dof.vert");
    create_info.shaders.vertex_constants = {width_constant, height_constant};
    create_info.shaders.fragment_src = file::Path("dof.frag");
    
    create_info.shader_input.bindings = {
        {0, GFXBindingType::StorageImage},
        {1, GFXBindingType::Texture},
        {3, GFXBindingType::Texture},
        {2, GFXBindingType::PushConstant}
    };

    create_info.shader_input.push_constants = {
        {sizeof(Vector4), 0}
    };
    
    create_info.render_pass = renderpass;
    
    create_info.blending.enable_blending = true;
    create_info.blending.src_rgb = GFXBlendFactor::SrcColor;
    create_info.blending.dst_rgb = GFXBlendFactor::One;
    create_info.blending.src_alpha = GFXBlendFactor::SrcAlpha;
    create_info.blending.dst_alpha = GFXBlendFactor::One;

    pipeline = gfx->create_graphics_pipeline(create_info);
        
    GFXTextureCreateInfo textureInfo = {};
    textureInfo.label = "Normal Field";
    textureInfo.width = extent.width;
    textureInfo.height = extent.height;
    textureInfo.format = GFXPixelFormat::RGBA_32F;
    textureInfo.usage = GFXTextureUsage::Attachment | GFXTextureUsage::Sampled;
    textureInfo.samplingMode = SamplingMode::ClampToEdge;
    
    normal_field = gfx->create_texture(textureInfo);

    textureInfo.label = "Far Field";
    far_field = gfx->create_texture(textureInfo);
            
    GFXFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.render_pass = renderpass;
    framebufferInfo.attachments = { normal_field };
    
    normal_framebuffer = gfx->create_framebuffer(framebufferInfo);
    
    framebufferInfo.attachments = { far_field };
    
    far_framebuffer = gfx->create_framebuffer(framebufferInfo);
}

void DoFPass::render(GFXCommandBuffer* command_buffer, Scene&) {
    //const auto render_extent = renderer->get_render_extent();
    const auto extent = prism::Extent();
    const auto render_extent = prism::Extent();
    
    // render far field
    GFXRenderPassBeginInfo beginInfo = {};
    beginInfo.framebuffer = far_framebuffer;
    beginInfo.render_pass = renderpass;

    command_buffer->set_render_pass(beginInfo);
    
    Viewport viewport = {};
    viewport.width = render_extent.width;
    viewport.height = render_extent.height;
    
    command_buffer->set_viewport(viewport);
    
    command_buffer->set_graphics_pipeline(pipeline);
    
    //command_buffer->bind_texture(renderer->offscreenColorTexture, 0);
    //command_buffer->bind_texture(renderer->offscreenDepthTexture, 1);
    command_buffer->bind_texture(aperture_texture->handle, 3);

    //const auto extent = renderer->get_render_extent();
    
    Vector4 params(render_options.depth_of_field_strength, 0.0, 0.0, 0.0);
    
    command_buffer->set_push_constant(&params, sizeof(Vector4));
    
    command_buffer->draw(0, 4, 0, extent.width * extent.height);
    
    // render normal field
    beginInfo.framebuffer = normal_framebuffer;
    
    command_buffer->set_render_pass(beginInfo);
    
    command_buffer->set_graphics_pipeline(pipeline);
    
    //command_buffer->bind_texture(renderer->offscreenColorTexture, 0);
    //command_buffer->bind_texture(renderer->offscreenDepthTexture, 1);
    command_buffer->bind_texture(aperture_texture->handle, 2);

    params.y = 1;
    
    command_buffer->set_push_constant(&params, sizeof(Vector4));
    
    command_buffer->draw(0, 4, 0, extent.width * extent.height);
}
