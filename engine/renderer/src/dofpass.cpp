#include "dofpass.hpp"

#include "renderer.hpp"
#include "gfx.hpp"
#include "gfx_commandbuffer.hpp"
#include "engine.hpp"
#include "asset.hpp"

AssetPtr<Texture> aperture_texture;

DoFPass::DoFPass(GFX* gfx, Renderer* renderer) : renderer(renderer) {
    aperture_texture = assetm->get<Texture>(file::app_domain / "textures/aperture.png");
    
    const auto extent = renderer->get_render_extent();
    
    GFXShaderConstant width_constant = {};
    width_constant.value = extent.width;
    
    GFXShaderConstant height_constant = {};
    height_constant.index = 1;
    height_constant.value = extent.height;
    
    GFXGraphicsPipelineCreateInfo create_info = {};
    create_info.shaders.vertex_path = "dof.vert";
    create_info.shaders.vertex_constants = {width_constant, height_constant};
    create_info.shaders.fragment_path = "dof.frag";
    create_info.render_pass = renderer->viewportRenderPass;
    create_info.blending.enable_blending = true;
    create_info.blending.src_rgb = GFXBlendFactor::SrcAlpha;
    create_info.blending.dst_rgb = GFXBlendFactor::OneMinusSrcAlpha;
    create_info.blending.src_alpha = GFXBlendFactor::SrcAlpha;
    create_info.blending.dst_alpha = GFXBlendFactor::OneMinusSrcAlpha;

    pipeline = gfx->create_graphics_pipeline(create_info);
}

void DoFPass::render(GFXCommandBuffer* command_buffer, Scene&) {
    command_buffer->set_graphics_pipeline(pipeline);
    
    command_buffer->bind_texture(renderer->offscreenColorTexture, 0);
    command_buffer->bind_texture(renderer->offscreenDepthTexture, 1);
    command_buffer->bind_texture(aperture_texture->handle, 2);

    const auto extent = renderer->get_render_extent();
    
    command_buffer->draw(0, 4, 0, extent.width * extent.height);
}
