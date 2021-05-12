#include "shadowpass.hpp"

#include "gfx_commandbuffer.hpp"
#include "scene.hpp"
#include "gfx.hpp"
#include "log.hpp"
#include "engine.hpp"
#include "materialcompiler.hpp"
#include "assertions.hpp"
#include "frustum.hpp"
#include "renderer.hpp"

struct PushConstant {
    Matrix4x4 mvp, model;
};

const std::array<Matrix4x4, 6> shadowTransforms = {
        prism::look_at(prism::float3(0), prism::float3(1.0, 0.0, 0.0), prism::float3(0.0, 1.0, 0.0)), // right
        prism::look_at(prism::float3(0), prism::float3(-1.0, 0.0, 0.0), prism::float3(0.0, 1.0, 0.0)), // left
        prism::look_at(prism::float3(0), prism::float3(0.0, 1.0, 0.0), prism::float3(0.0, 0.0, -1.0)), // top
        prism::look_at(prism::float3(0), prism::float3(0.0, -1.0, 0.0), prism::float3(0.0, 0.0, 1.0)), // bottom
        prism::look_at(prism::float3(0), prism::float3(0.0, 0.0, 1.0), prism::float3(0.0, 1.0, 0.0)), // back
        prism::look_at(prism::float3(0), prism::float3(0.0, 0.0, -1.0), prism::float3(0.0, 1.0, 0.0)) // front
};

ShadowPass::ShadowPass(GFX* gfx) {
    Expects(gfx != nullptr);
    Expects(render_options.shadow_resolution > 0);
    
    create_render_passes();
    create_pipelines();
    create_offscreen_resources();
}

void ShadowPass::create_scene_resources(Scene& scene) {
    auto gfx = engine->get_gfx();
    
    // sun light
    {
        GFXTextureCreateInfo textureInfo = {};
        textureInfo.label = "Shadow Depth";
        textureInfo.width = render_options.shadow_resolution;
        textureInfo.height = render_options.shadow_resolution;
        textureInfo.format = GFXPixelFormat::DEPTH_32F;
        textureInfo.usage = GFXTextureUsage::Sampled | GFXTextureUsage::Attachment;
        textureInfo.samplingMode = SamplingMode::ClampToBorder;
        textureInfo.border_color = GFXBorderColor::OpaqueWhite;

        scene.depthTexture = gfx->create_texture(textureInfo);
        
        GFXFramebufferCreateInfo info;
        info.attachments = {scene.depthTexture};
        info.render_pass = render_pass;
        
        scene.framebuffer = gfx->create_framebuffer(info);
    }
    
    // point lights
    if(gfx->supports_feature(GFXFeature::CubemapArray)) {
        GFXTextureCreateInfo cubeTextureInfo = {};
        cubeTextureInfo.label = "Point Light Array";
        cubeTextureInfo.type = GFXTextureType::CubemapArray;
        cubeTextureInfo.width = render_options.shadow_resolution;
        cubeTextureInfo.height = render_options.shadow_resolution;
        cubeTextureInfo.format = GFXPixelFormat::R_32F;
        cubeTextureInfo.usage = GFXTextureUsage::Sampled;
        cubeTextureInfo.array_length = max_point_shadows;
        cubeTextureInfo.samplingMode = SamplingMode::ClampToBorder;
        cubeTextureInfo.border_color = GFXBorderColor::OpaqueWhite;

        scene.pointLightArray = gfx->create_texture(cubeTextureInfo);
    }
    
    // spot lights
    {
        GFXTextureCreateInfo spotTextureInfo = {};
        spotTextureInfo.label = "Spot Light Array";
        spotTextureInfo.type = GFXTextureType::Array2D;
        spotTextureInfo.width = render_options.shadow_resolution;
        spotTextureInfo.height = render_options.shadow_resolution;
        spotTextureInfo.format = GFXPixelFormat::DEPTH_32F;
        spotTextureInfo.usage = GFXTextureUsage::Sampled | GFXTextureUsage::Attachment;
        spotTextureInfo.array_length = max_spot_shadows;
        spotTextureInfo.samplingMode = SamplingMode::ClampToBorder;
        spotTextureInfo.border_color = GFXBorderColor::OpaqueWhite;

        scene.spotLightArray = gfx->create_texture(spotTextureInfo);
    }
}

void ShadowPass::render(GFXCommandBuffer* command_buffer, Scene& scene) {
    last_spot_light = 0;
    last_point_light = 0;

    if(scene.shadow_refresh_timer > 0) {
        scene.shadow_refresh_timer--;
        return;
    }
    
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

void ShadowPass::render_meshes(GFXCommandBuffer* command_buffer, Scene& scene, const Matrix4x4 light_matrix, const Matrix4x4 model, const prism::float3 light_position, const Light::Type type, const CameraFrustum& frustum, const int base_instance) {
    for(auto [obj, mesh] : scene.get_all<Renderable>()) {
        if(!mesh.mesh)
            continue;
        
        command_buffer->set_vertex_buffer(mesh.mesh->position_buffer, 0, position_buffer_index);
        
        command_buffer->set_index_buffer(mesh.mesh->index_buffer, IndexType::UINT32);
        
        PushConstant pc;
        pc.mvp = light_matrix * model * scene.get<Transform>(obj).model;
        pc.model = scene.get<Transform>(obj).model;
        
        if(mesh.mesh->bones.empty()) {
            switch(type) {
                case Light::Type::Sun:
                    command_buffer->set_graphics_pipeline(static_sun_pipeline);
                    break;
                case Light::Type::Spot:
                    command_buffer->set_graphics_pipeline(static_spot_pipeline);
                    break;
                case Light::Type::Point:
                    command_buffer->set_graphics_pipeline(static_point_pipeline);
                    break;
            }
            
            command_buffer->bind_shader_buffer(point_location_buffer, 0, 2, sizeof(prism::float3) * max_point_shadows);

            command_buffer->set_push_constant(&pc, sizeof(PushConstant));
            command_buffer->set_depth_bias(1.25f, 0.00f, 1.75f);

            for (auto& part : mesh.mesh->parts) {
                if(render_options.enable_frustum_culling && !test_aabb_frustum(frustum, get_aabb_for_part(scene.get<Transform>(obj), part)))
                    continue;
                
                command_buffer->draw_indexed(part.index_count, part.index_offset, part.vertex_offset, base_instance);
            }
        } else {
            switch(type) {
                case Light::Type::Sun:
                    command_buffer->set_graphics_pipeline(skinned_sun_pipeline);
                    break;
                case Light::Type::Spot:
                    command_buffer->set_graphics_pipeline(skinned_spot_pipeline);
                    break;
                case Light::Type::Point:
                    command_buffer->set_graphics_pipeline(skinned_point_pipeline);
                    break;
            }

            command_buffer->bind_shader_buffer(point_location_buffer, 0, 2, sizeof(prism::float3) * max_point_shadows);
            
            command_buffer->set_push_constant(&pc, sizeof(PushConstant));
            
            command_buffer->set_vertex_buffer(mesh.mesh->bone_buffer, 0, bone_buffer_index);
            command_buffer->set_depth_bias(1.25f, 0.00f, 1.75f);

            for (auto& part : mesh.mesh->parts) {
                if(render_options.enable_frustum_culling && !test_aabb_frustum(frustum, get_aabb_for_part(scene.get<Transform>(obj), part)))
                    continue;
                
                command_buffer->bind_shader_buffer(part.bone_batrix_buffer, 0, 14, sizeof(Matrix4x4) * 128);
                command_buffer->draw_indexed(part.index_count, part.index_offset, part.vertex_offset, base_instance);
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
        
        const prism::float3 lightPos = scene.get<Transform>(light_object).position;
        
        const Matrix4x4 projection = prism::orthographic(-25.0f, 25.0f, -25.0f, 25.0f, 0.1f, 100.0f);
        const Matrix4x4 view = prism::look_at(lightPos, prism::float3(0), prism::float3(0, 1, 0));
        
        const Matrix4x4 realMVP = projection * view;
        
        scene.lightSpace = projection;
        scene.lightSpace[1][1] *= -1;
        scene.lightSpace = scene.lightSpace * view;
        
        const auto frustum = normalize_frustum(extract_frustum(projection * view));
                
        if(light.enable_shadows)
            render_meshes(command_buffer, scene, realMVP, Matrix4x4(), lightPos, Light::Type::Sun, frustum, 0);
        
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
        
        const Matrix4x4 perspective = prism::perspective(radians(90.0f), 1.0f, 0.1f, 100.0f);
        
        const Matrix4x4 realMVP = perspective * inverse(scene.get<Transform>(light_object).model);
        
        scene.spotLightSpaces[last_spot_light] = perspective;
        scene.spotLightSpaces[last_spot_light][1][1] *= -1;
        scene.spotLightSpaces[last_spot_light] = scene.spotLightSpaces[last_spot_light] * inverse(scene.get<Transform>(light_object).model);
        
        const auto frustum = normalize_frustum(extract_frustum(perspective * inverse(scene.get<Transform>(light_object).model)));
        
        if(light.enable_shadows)
            render_meshes(command_buffer, scene, realMVP, Matrix4x4(), scene.get<Transform>(light_object).get_world_position(), Light::Type::Spot, frustum, 0);
        
        command_buffer->end_render_pass();
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
        *(point_location_map + last_point_light) = scene.get<Transform>(light_object).get_world_position();

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
            
            const prism::float3 lightPos = scene.get<Transform>(light_object).get_world_position();
            
            const Matrix4x4 projection = prism::perspective(radians(90.0f), 1.0f, 0.1f, 100.0f);
            const Matrix4x4 model = inverse(scene.get<Transform>(light_object).model);
            
            const auto frustum = normalize_frustum(extract_frustum(projection * shadowTransforms[face] * model));
            
            render_meshes(command_buffer, scene, projection * shadowTransforms[face], model, lightPos, Light::Type::Point, frustum, last_point_light);
            
            command_buffer->end_render_pass();
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
    renderPassInfo.label = "Shadow";
    renderPassInfo.attachments.push_back(GFXPixelFormat::DEPTH_32F);
    renderPassInfo.will_use_in_shader = true;
    
    render_pass = gfx->create_render_pass(renderPassInfo);
    
    renderPassInfo.label = "Shadow Cube";
    renderPassInfo.attachments.clear();
    renderPassInfo.attachments.push_back(GFXPixelFormat::R_32F);
    renderPassInfo.attachments.push_back(GFXPixelFormat::DEPTH_32F);
    
    cube_render_pass = gfx->create_render_pass(renderPassInfo);
}

void ShadowPass::create_pipelines() {
    GFXShaderConstant point_light_max_constant = {};
    point_light_max_constant.value = max_point_shadows;

    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.shaders.vertex_constants = {point_light_max_constant};
    pipelineInfo.shaders.vertex_src = ShaderSource(prism::path("shadow.vert"));

    pipelineInfo.shaders.fragment_constants = { point_light_max_constant };

    pipelineInfo.shader_input.bindings = {
        {0, GFXBindingType::PushConstant},
        {1, GFXBindingType::StorageBuffer},
        {2, GFXBindingType::StorageBuffer}
    };
    
    pipelineInfo.shader_input.push_constants = {
        {sizeof(PushConstant), 0}
    };
    
    pipelineInfo.render_pass = render_pass;
    
    pipelineInfo.depth.depth_mode = GFXDepthMode::LessOrEqual;

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

        pipelineInfo.shaders.fragment_src = ShaderSource(prism::path("omnishadow.frag"));
        
        auto [static_pipeline, skinned_pipeline] = material_compiler.create_pipeline_permutations(pipelineInfo, true);
        
        static_point_pipeline = static_pipeline;
        skinned_point_pipeline = skinned_pipeline;
    }
}

void ShadowPass::create_offscreen_resources() {
    auto gfx = engine->get_gfx();
    
    GFXTextureCreateInfo textureInfo = {};
    textureInfo.label = "Shadow Color";
    textureInfo.width = render_options.shadow_resolution;
    textureInfo.height = render_options.shadow_resolution;
    textureInfo.format = GFXPixelFormat::R_32F;
    textureInfo.usage = GFXTextureUsage::Attachment | GFXTextureUsage::Sampled;
    textureInfo.samplingMode = SamplingMode::ClampToEdge;

    offscreen_color_texture = gfx->create_texture(textureInfo);
    
    GFXTextureCreateInfo depthTextureInfo = {};
    depthTextureInfo.label = "Shadow Depth";
    depthTextureInfo.width = render_options.shadow_resolution;
    depthTextureInfo.height = render_options.shadow_resolution;
    depthTextureInfo.format = GFXPixelFormat::DEPTH_32F;
    depthTextureInfo.usage = GFXTextureUsage::Attachment | GFXTextureUsage::Sampled;
    depthTextureInfo.samplingMode = SamplingMode::ClampToEdge;
    
    offscreen_depth = gfx->create_texture(depthTextureInfo);
    
    GFXFramebufferCreateInfo info;
    info.attachments = {offscreen_color_texture, offscreen_depth};
    info.render_pass = cube_render_pass;
    
    offscreen_framebuffer = gfx->create_framebuffer(info);

    point_location_buffer = gfx->create_buffer(nullptr, sizeof(prism::float3) * max_point_shadows, false, GFXBufferUsage::Storage);
    point_location_map = reinterpret_cast<prism::float3*>(gfx->get_buffer_contents(point_location_buffer));
}
