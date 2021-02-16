#include "smaapass.hpp"

#include "gfx_commandbuffer.hpp"
#include "renderer.hpp"
#include "engine.hpp"
#include "gfx.hpp"
#include "assertions.hpp"

#include <AreaTex.h>
#include <SearchTex.h>

SMAAPass::SMAAPass(GFX* gfx, Renderer* renderer) : renderer(renderer), extent(renderer->get_render_extent()) {
    Expects(gfx != nullptr);
    Expects(renderer != nullptr);
    
    create_textures();
    create_render_pass();
    create_pipelines();
    create_offscreen_resources();
}

void SMAAPass::render(GFXCommandBuffer* command_buffer) {
    GFXRenderPassBeginInfo beginInfo = {};
    beginInfo.clear_color.a = 0.0f;
    beginInfo.render_area.extent = extent;
    beginInfo.render_pass = render_pass;
    
    struct PushConstant {
        Vector4 viewport;
        Matrix4x4 correction_matrix;
    } pc;
    
    pc.viewport = Vector4(1.0f / static_cast<float>(extent.width), 1.0f / static_cast<float>(extent.height), extent.width, extent.height);

    // edge
    {
        beginInfo.framebuffer = edge_framebuffer;
        command_buffer->set_render_pass(beginInfo);
        
        Viewport viewport = {};
        viewport.width = extent.width;
        viewport.height = extent.height;
        
        command_buffer->set_viewport(viewport);
        
        command_buffer->set_graphics_pipeline(edge_pipeline);
        command_buffer->set_push_constant(&pc, sizeof(PushConstant));

        command_buffer->bind_texture(renderer->offscreenColorTexture, 0); // color
        command_buffer->bind_texture(renderer->offscreenDepthTexture, 1); // depth

        command_buffer->draw(0, 3, 0, 1);
    }

    // blend
    {
        beginInfo.framebuffer = blend_framebuffer;
        command_buffer->set_render_pass(beginInfo);

        command_buffer->set_graphics_pipeline(blend_pipeline);
        command_buffer->set_push_constant(&pc, sizeof(PushConstant));

        command_buffer->bind_texture(edge_texture, 0);
        command_buffer->bind_texture(area_image, 1);
        command_buffer->bind_texture(search_image, 3);

        command_buffer->draw(0, 3, 0, 1);
    }
}

void SMAAPass::create_textures() {
    auto gfx = engine->get_gfx();
    
    //load area image
    GFXTextureCreateInfo areaInfo = {};
    areaInfo.label = "SMAA Area";
    areaInfo.width = AREATEX_WIDTH;
    areaInfo.height = AREATEX_HEIGHT;
    areaInfo.format = GFXPixelFormat::R8G8_UNORM;
    areaInfo.usage = GFXTextureUsage::Sampled;
    areaInfo.samplingMode = SamplingMode::ClampToEdge;
    
    area_image = gfx->create_texture(areaInfo);
    
    gfx->copy_texture(area_image, (void*)areaTexBytes, AREATEX_SIZE);
    
    // load search image
    GFXTextureCreateInfo searchInfo = {};
    searchInfo.label = "SMAA Search";
    searchInfo.width = SEARCHTEX_WIDTH;
    searchInfo.height = SEARCHTEX_HEIGHT;
    searchInfo.format = GFXPixelFormat::R8_UNORM;
    searchInfo.usage = GFXTextureUsage::Sampled;
    searchInfo.samplingMode = SamplingMode::ClampToEdge;
    
    search_image = gfx->create_texture(searchInfo);
    
    gfx->copy_texture(search_image, (void*)searchTexBytes, SEARCHTEX_SIZE);
}

void SMAAPass::create_render_pass() {
    GFXRenderPassCreateInfo createInfo = {};
    createInfo.label = "SMAA";
    createInfo.attachments = {GFXPixelFormat::R16G16B16A16_SFLOAT};
    createInfo.will_use_in_shader = true;
    
    render_pass = engine->get_gfx()->create_render_pass(createInfo);
}

void SMAAPass::create_pipelines() {
    auto gfx = engine->get_gfx();

    GFXGraphicsPipelineCreateInfo createInfo = {};
    createInfo.label = "SMAA Edge";
    createInfo.shaders.vertex_path = "edge.vert";
    createInfo.shaders.fragment_path = "edge.frag";
    
    createInfo.render_pass = render_pass;
    
    createInfo.shader_input.push_constants = {
        {sizeof(Vector4) + sizeof(Matrix4x4), 0}
    };
    
    createInfo.shader_input.bindings = {
        {0, GFXBindingType::Texture},
        {1, GFXBindingType::Texture},
        {2, GFXBindingType::PushConstant}
    };
    
    edge_pipeline = gfx->create_graphics_pipeline(createInfo);
    
    createInfo.label = "SMAA Blend";
    createInfo.shaders.vertex_path = "blend.vert";
    createInfo.shaders.fragment_path = "blend.frag";
    createInfo.shader_input.bindings.push_back({3, GFXBindingType::Texture});
    
    blend_pipeline = gfx->create_graphics_pipeline(createInfo);
}

void SMAAPass::create_offscreen_resources() {
    auto gfx = engine->get_gfx();

    GFXTextureCreateInfo textureInfo = {};
    textureInfo.label = "SMAA Edge";
    textureInfo.width = extent.width;
    textureInfo.height = extent.height;
    textureInfo.format = GFXPixelFormat::R16G16B16A16_SFLOAT;
    textureInfo.usage = GFXTextureUsage::Attachment | GFXTextureUsage::Sampled;
    
    edge_texture = gfx->create_texture(textureInfo);

    textureInfo.label = "SMAA Blend";
    blend_texture = gfx->create_texture(textureInfo);
    
    GFXFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.attachments = {edge_texture};
    framebufferInfo.render_pass = render_pass;
    
    edge_framebuffer = gfx->create_framebuffer(framebufferInfo);
    
    framebufferInfo.attachments = {blend_texture};
    
    blend_framebuffer = gfx->create_framebuffer(framebufferInfo);
}
