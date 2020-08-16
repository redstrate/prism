#include "shadowpass.hpp"

#include "gfx_commandbuffer.hpp"
#include "scene.hpp"
#include "gfx.hpp"
#include "log.hpp"
#include "engine.hpp"
#include "materialcompiler.hpp"
#include "assertions.hpp"
#include "frustum.hpp"

struct PushConstant {
    Matrix4x4 mvp, model;
    Vector3 lightPos;
};

const std::array<Matrix4x4, 6> shadowTransforms = {
    transform::look_at(Vector3(0), Vector3(1.0, 0.0, 0.0), Vector3(0.0, -1.0, 0.0)),
    transform::look_at(Vector3(0), Vector3(-1.0, 0.0, 0.0), Vector3(0.0, -1.0, 0.0)),
    transform::look_at(Vector3(0), Vector3( 0.0, 1.0, 0.0), Vector3(0.0, 0.0,1.0)),
    transform::look_at(Vector3(0), Vector3( 0.0, -1.0, 0.0), Vector3(0.0, 0.0,-1.0)),
    transform::look_at(Vector3(0), Vector3( 0.0, 0.0, 1.0), Vector3(0.0, -1.0, 0.0)),
    transform::look_at(Vector3(0), Vector3( 0.0, 0.0, -1.0), Vector3(0.0, -1.0, 0.0))
};

ShadowPass::ShadowPass(GFX* gfx) {
    Expects(gfx != nullptr);
    Expects(render_options.shadow_resolution > 0);
    
    create_render_passes();
    create_pipelines();
    create_offscreen_resources();
    
    GFXSamplerCreateInfo sampler_info = {};
    sampler_info.samplingMode = SamplingMode::ClampToBorder;
    sampler_info.border_color = GFXBorderColor::OpaqueWhite;
    
    shadow_sampler = gfx->create_sampler(sampler_info);
    pcf_sampler = gfx->create_sampler(sampler_info); // unused atm
}

void ShadowPass::create_scene_resources(Scene& scene) {
    auto gfx = engine->get_gfx();
    
    // sun light
    {
        GFXTextureCreateInfo textureInfo = {};
        textureInfo.width = render_options.shadow_resolution;
        textureInfo.height = render_options.shadow_resolution;
        textureInfo.format = GFXPixelFormat::DEPTH_32F;
        textureInfo.usage = GFXTextureUsage::Sampled | GFXTextureUsage::Attachment;

        scene.depthTexture = gfx->create_texture(textureInfo);
        
        GFXFramebufferCreateInfo info;
        info.attachments = {scene.depthTexture};
        info.render_pass = render_pass;
        
        scene.framebuffer = gfx->create_framebuffer(info);
    }
    
    // point lights
    if(gfx->supports_feature(GFXFeature::CubemapArray)) {
        GFXTextureCreateInfo cubeTextureInfo = {};
        cubeTextureInfo.type = GFXTextureType::CubemapArray;
        cubeTextureInfo.width = render_options.shadow_resolution;
        cubeTextureInfo.height = render_options.shadow_resolution;
        cubeTextureInfo.format = GFXPixelFormat::R_32F;
        cubeTextureInfo.usage = GFXTextureUsage::Sampled;
        cubeTextureInfo.samplingMode = SamplingMode::ClampToEdge;
        cubeTextureInfo.array_length = max_point_shadows;
        
        scene.pointLightArray = gfx->create_texture(cubeTextureInfo);
    }
    
    // spot lights
    {
        GFXTextureCreateInfo spotTextureInfo = {};
        spotTextureInfo.type = GFXTextureType::Array2D;
        spotTextureInfo.width = render_options.shadow_resolution;
        spotTextureInfo.height = render_options.shadow_resolution;
        spotTextureInfo.format = GFXPixelFormat::DEPTH_32F;
        spotTextureInfo.usage = GFXTextureUsage::Sampled | GFXTextureUsage::Attachment;
        spotTextureInfo.samplingMode = SamplingMode::ClampToEdge;
        spotTextureInfo.array_length = max_spot_shadows;
        
        scene.spotLightArray = gfx->create_texture(spotTextureInfo);
    }
}

void ShadowPass::render(GFXCommandBuffer* command_buffer, Scene& scene) {
    last_spot_light = 0;
    last_point_light = 0;
    
    auto lights = scene.get_all<Light>();
    for(auto [obj, light] : lights) {
        switch(light.type) {
            case Light::Type::Sun:
                render_sun(command_buffer, scene, obj, light);
                break;
            case Light::Type::Spot:
                render_spot(command_buffer, scene, obj, light);
                break;
            case Light::Type::Point:
                render_point(command_buffer, scene, obj, light);
                break;
        }
    }
}

void ShadowPass::render_meshes(GFXCommandBuffer* command_buffer, Scene& scene, const Matrix4x4 light_matrix, const Matrix4x4 model, const Vector3 light_position, const Light::Type type, const CameraFrustum& frustum) {
    for(auto [obj, mesh] : scene.get_all<Renderable>()) {
        if(!mesh.mesh)
            continue;
        
        command_buffer->set_vertex_buffer(mesh.mesh->position_buffer, 0, position_buffer_index);
        
        command_buffer->set_index_buffer(mesh.mesh->index_buffer, IndexType::UINT32);
        
        PushConstant pc;
        pc.mvp = light_matrix * model * scene.get<Transform>(obj).model;
        pc.model = scene.get<Transform>(obj).model;
        pc.lightPos = light_position;
        
        if(mesh.mesh->bones.empty()) {
            switch(type) {
                case Light::Type::Sun:
                    command_buffer->set_pipeline(static_sun_pipeline);
                    break;
                case Light::Type::Spot:
                    command_buffer->set_pipeline(static_spot_pipeline);
                    break;
                case Light::Type::Point:
                    command_buffer->set_pipeline(static_point_pipeline);
                    break;
            }
            
            command_buffer->set_push_constant(&pc, sizeof(PushConstant));
            command_buffer->set_depth_bias(1.25f, 0.00f, 1.75f);

            for (auto& part : mesh.mesh->parts) {
                if(render_options.enable_frustum_culling && !test_aabb_frustum(frustum, get_aabb_for_part(scene.get<Transform>(obj), part))) {
                    console::debug(System::Renderer, "Shadow Culled {}!", scene.get(obj).name);
                    continue;
                }
                
                command_buffer->draw_indexed(part.index_count, part.index_offset, part.vertex_offset);
            }
        } else {
            switch(type) {
                case Light::Type::Sun:
                    command_buffer->set_pipeline(skinned_sun_pipeline);
                    break;
                case Light::Type::Spot:
                    command_buffer->set_pipeline(skinned_spot_pipeline);
                    break;
                case Light::Type::Point:
                    command_buffer->set_pipeline(skinned_point_pipeline);
                    break;
            }
            
            command_buffer->set_push_constant(&pc, sizeof(PushConstant));
            
            command_buffer->set_vertex_buffer(mesh.mesh->bone_buffer, 0, bone_buffer_index);
            command_buffer->set_depth_bias(1.25f, 0.00f, 1.75f);

            for (auto& part : mesh.mesh->parts) {
                if(render_options.enable_frustum_culling && !test_aabb_frustum(frustum, get_aabb_for_part(scene.get<Transform>(obj), part))) {
                    console::debug(System::Renderer, "Shadow Culled {}!", scene.get(obj).name);
                    continue;
                }
                
                command_buffer->bind_shader_buffer(part.bone_batrix_buffer, 0, 1, sizeof(Matrix4x4) * 128);
                command_buffer->draw_indexed(part.index_count, part.index_offset, part.vertex_offset);
            }
        }
    }
}

void ShadowPass::render_sun(GFXCommandBuffer* command_buffer, Scene& scene, Object light_object, Light& light) {
    if(scene.sun_light_dirty || light.use_dynamic_shadows) {
        GFXRenderPassBeginInfo info = {};
        info.framebuffer = scene.framebuffer;
        info.render_pass = render_pass;
        info.render_area.extent = {static_cast<uint32_t>(render_options.shadow_resolution), static_cast<uint32_t>(render_options.shadow_resolution)};
        
        command_buffer->set_render_pass(info);
        
        Viewport viewport = {};
        viewport.width = render_options.shadow_resolution;
        viewport.height = render_options.shadow_resolution;
        
        command_buffer->set_viewport(viewport);
        
        const Vector3 lightPos = scene.get<Transform>(light_object).position;
        
        const Matrix4x4 projection = transform::orthographic(-25.0f, 25.0f, -25.0f, 25.0f, 0.1f, 100.0f);
        const Matrix4x4 view = transform::look_at(lightPos, Vector3(0), Vector3(0, 1, 0));
        
        const Matrix4x4 realMVP = projection * engine->get_renderer()->correction_matrix * view;
        
        scene.lightSpace = projection * view;
        
        const auto frustum = normalize_frustum(extract_frustum(projection * view));
                
        if(light.enable_shadows)
            render_meshes(command_buffer, scene, realMVP, Matrix4x4(), lightPos, Light::Type::Sun, frustum);
        
        scene.sun_light_dirty = false;
    }
}

void ShadowPass::render_spot(GFXCommandBuffer* command_buffer, Scene& scene, Object light_object, Light& light) {
    if((last_spot_light + 1) == max_spot_shadows)
        return;
    
    if(scene.spot_light_dirty[last_spot_light] || light.use_dynamic_shadows) {
        GFXRenderPassBeginInfo info = {};
        info.framebuffer = offscreen_framebuffer;
        info.render_pass = cube_render_pass;
        info.render_area.extent = {static_cast<uint32_t>(render_options.shadow_resolution), static_cast<uint32_t>(render_options.shadow_resolution)};
        
        command_buffer->set_render_pass(info);
        
        Viewport viewport = {};
        viewport.width = render_options.shadow_resolution;
        viewport.height = render_options.shadow_resolution;
        
        command_buffer->set_viewport(viewport);
        
        const Matrix4x4 perspective = transform::perspective(radians(90.0f), 1.0f, 0.1f);
        
        const Matrix4x4 realMVP = perspective * engine->get_renderer()->correction_matrix * inverse(scene.get<Transform>(light_object).model);
        
        scene.spotLightSpaces[last_spot_light] = perspective * inverse(scene.get<Transform>(light_object).model);
        
        const auto frustum = normalize_frustum(extract_frustum(perspective * inverse(scene.get<Transform>(light_object).model)));
        
        if(light.enable_shadows)
            render_meshes(command_buffer, scene, realMVP, Matrix4x4(), scene.get<Transform>(light_object).get_world_position(), Light::Type::Spot, frustum);
        
        command_buffer->copy_texture(offscreen_depth, render_options.shadow_resolution, render_options.shadow_resolution, scene.spotLightArray, 0, last_spot_light, 0);
        
        scene.spot_light_dirty[last_spot_light] = false;
    }
    
    last_spot_light++;
}

void ShadowPass::render_point(GFXCommandBuffer* command_buffer, Scene& scene, Object light_object, Light& light) {
    if(!render_options.enable_point_shadows)
        return;
    
    if((last_point_light + 1) == max_point_shadows)
        return;
    
    if(scene.point_light_dirty[last_point_light] || light.use_dynamic_shadows) {
        for(int face = 0; face < 6; face++) {
            GFXRenderPassBeginInfo info = {};
            info.framebuffer = offscreen_framebuffer;
            info.render_pass = cube_render_pass;
            info.render_area.extent = {static_cast<uint32_t>(render_options.shadow_resolution), static_cast<uint32_t>(render_options.shadow_resolution)};

            command_buffer->set_render_pass(info);
            
            Viewport viewport = {};
            viewport.width = render_options.shadow_resolution;
            viewport.height = render_options.shadow_resolution;
            
            command_buffer->set_viewport(viewport);
            
            const Vector3 lightPos = scene.get<Transform>(light_object).get_world_position();
            
            const Matrix4x4 projection = transform::perspective(radians(90.0f), 1.0f, 0.1f);
            const Matrix4x4 model = transform::translate(Matrix4x4(), Vector3(-lightPos.x, -lightPos.y, -lightPos.z));
            
            const auto frustum = normalize_frustum(extract_frustum(projection * shadowTransforms[face] * model));
            
            render_meshes(command_buffer, scene, projection * engine->get_renderer()->correction_matrix * shadowTransforms[face], model, lightPos, Light::Type::Point, frustum);
            
            command_buffer->copy_texture(offscreen_color_texture, render_options.shadow_resolution, render_options.shadow_resolution, scene.pointLightArray, face, last_point_light, 0);
        }
        
        scene.point_light_dirty[last_point_light] = false;
    }
    
    last_point_light++;
}

void ShadowPass::create_render_passes() {
    auto gfx = engine->get_gfx();
    
    // render pass
    GFXRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachments.push_back(GFXPixelFormat::DEPTH_32F);
    
    render_pass = gfx->create_render_pass(renderPassInfo);
    
    renderPassInfo.attachments.clear();
    renderPassInfo.attachments.push_back(GFXPixelFormat::R_32F);
    renderPassInfo.attachments.push_back(GFXPixelFormat::DEPTH_32F);
    
    cube_render_pass = gfx->create_render_pass(renderPassInfo);
}

void ShadowPass::create_pipelines() {
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.shaders.vertex_path = "shadow.vert";
    pipelineInfo.shaders.fragment_path = "shadow.frag";
    
    pipelineInfo.shader_input.bindings = {
        {0, GFXBindingType::PushConstant}
    };
    
    pipelineInfo.shader_input.push_constants = {
        {sizeof(PushConstant), 0}
    };
    
    pipelineInfo.render_pass = render_pass;
    
    pipelineInfo.depth.depth_mode = GFXDepthMode::LessOrEqual;
    pipelineInfo.rasterization.culling_mode = GFXCullingMode::None;
    
    // sun
    {
        pipelineInfo.label = "Sun Shadow";

        auto [static_pipeline, skinned_pipeline] = material_compiler.create_pipeline_permutations(pipelineInfo, true);
        
        static_sun_pipeline = static_pipeline;
        skinned_sun_pipeline = skinned_pipeline;
    }
    
    // spot
    {
        pipelineInfo.label = "Spot Shadow";

        pipelineInfo.render_pass = cube_render_pass;
        
        auto [static_pipeline, skinned_pipeline] = material_compiler.create_pipeline_permutations(pipelineInfo, true);
        
        static_spot_pipeline = static_pipeline;
        skinned_spot_pipeline = skinned_pipeline;
    }
    
    // point
    {
        pipelineInfo.label = "Point Shadow";

        pipelineInfo.shaders.fragment_path = "omnishadow.frag";
        
        auto [static_pipeline, skinned_pipeline] = material_compiler.create_pipeline_permutations(pipelineInfo, true);
        
        static_point_pipeline = static_pipeline;
        skinned_point_pipeline = skinned_pipeline;
    }
}

void ShadowPass::create_offscreen_resources() {
    auto gfx = engine->get_gfx();
    
    GFXTextureCreateInfo textureInfo = {};
    textureInfo.width = render_options.shadow_resolution;
    textureInfo.height = render_options.shadow_resolution;
    textureInfo.format = GFXPixelFormat::R_32F;
    textureInfo.usage = GFXTextureUsage::Attachment;
    textureInfo.samplingMode = SamplingMode::ClampToEdge;
    
    offscreen_color_texture = gfx->create_texture(textureInfo);
    
    GFXTextureCreateInfo depthTextureInfo = {};
    depthTextureInfo.width = render_options.shadow_resolution;
    depthTextureInfo.height = render_options.shadow_resolution;
    depthTextureInfo.format = GFXPixelFormat::DEPTH_32F;
    depthTextureInfo.usage = GFXTextureUsage::Attachment;
    depthTextureInfo.samplingMode = SamplingMode::ClampToEdge;
    
    offscreen_depth = gfx->create_texture(depthTextureInfo);
    
    GFXFramebufferCreateInfo info;
    info.attachments = {offscreen_color_texture, offscreen_depth};
    info.render_pass = render_pass;
    
    offscreen_framebuffer = gfx->create_framebuffer(info);
}
