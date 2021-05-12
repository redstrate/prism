#include "renderer.hpp"

#include <array>

#include "gfx_commandbuffer.hpp"
#include "math.hpp"
#include "screen.hpp"
#include "file.hpp"
#include "scene.hpp"
#include "font.hpp"
#include "vector.hpp"
#include "imguipass.hpp"
#include "gfx.hpp"
#include "log.hpp"
#include "pass.hpp"
#include "shadowpass.hpp"
#include "smaapass.hpp"
#include "scenecapture.hpp"
#include "materialcompiler.hpp"
#include "assertions.hpp"
#include "dofpass.hpp"
#include "frustum.hpp"
#include "shadercompiler.hpp"
#include "asset.hpp"
#include "debug.hpp"

using prism::renderer;

struct ElementInstance {
    prism::float4 color;
    uint32_t position = 0, size = 0;
    uint32_t padding[2];
};

struct GlyphInstance {
    uint32_t position = 0, index = 0, instance = 0;
};

struct GylphMetric {
    uint32_t x0_y0, x1_y1;
    float xoff, yoff;
    float xoff2, yoff2;
};

struct StringInstance {
    uint32_t xy;
};

struct SceneMaterial {
    prism::float4 color, info;
};

struct SceneLight {
    prism::float4 positionType;
    prism::float4 directionPower;
    prism::float4 colorSize;
    prism::float4 shadowsEnable;
};

struct SceneProbe {
    prism::float4 position, size;
};

struct SceneInformation {
    prism::float4 options;
    prism::float4 camPos;
    Matrix4x4 vp, lightspace;
    Matrix4x4 spotLightSpaces[max_spot_shadows];
    SceneMaterial materials[max_scene_materials];
    SceneLight lights[max_scene_lights];
    SceneProbe probes[max_environment_probes];
    int numLights;
    int p[3];
};

struct PostPushConstants {
    prism::float4 viewport, options, transform_ops;
};

struct UIPushConstant {
    prism::float2 screenSize;
    Matrix4x4 mvp;
};

struct SkyPushConstant {
    Matrix4x4 view;
    prism::float4 sun_position_fov;
    float aspect;
};

renderer::renderer(GFX* gfx, const bool enable_imgui) : gfx(gfx) {
    Expects(gfx != nullptr);
    
    shader_compiler.set_include_path(prism::get_domain_path(prism::domain::internal).string());

    create_dummy_texture();
    create_histogram_resources();

    shadow_pass = std::make_unique<ShadowPass>(gfx);
    scene_capture = std::make_unique<SceneCapture>(gfx);
    smaa_pass = std::make_unique<SMAAPass>(gfx, this);
    
    if(enable_imgui)
        addPass<ImGuiPass>();

    generate_brdf();
    
    GFXRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.label = "Offscreen";
    renderPassInfo.attachments.push_back(GFXPixelFormat::RGBA_32F);
    renderPassInfo.attachments.push_back(GFXPixelFormat::DEPTH_32F);
    renderPassInfo.will_use_in_shader = true;

    offscreen_render_pass = gfx->create_render_pass(renderPassInfo);

    renderPassInfo.label = "Offscreen (UNORM)";
    renderPassInfo.attachments = {GFXPixelFormat::R8G8B8A8_UNORM};

    unorm_render_pass = gfx->create_render_pass(renderPassInfo);

    create_font_texture();
    create_sky_pipeline();
}

renderer::~renderer() = default;

RenderTarget* renderer::allocate_render_target(const prism::Extent extent) {
    auto target = new RenderTarget();
    
    resize_render_target(*target, extent);
    
    render_targets.push_back(target);
    
    return target;
}

void renderer::resize_render_target(RenderTarget& target, const prism::Extent extent) {
    target.extent = extent;
        
    create_render_target_resources(target);
    smaa_pass->create_render_target_resources(target);
    create_post_pipelines();
    
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "Text";
    
    pipelineInfo.shaders.vertex_src = ShaderSource(prism::path("text.vert"));
    pipelineInfo.shaders.fragment_src = ShaderSource(prism::path("text.frag"));
    
    pipelineInfo.rasterization.primitive_type = GFXPrimitiveType::TriangleStrip;
    
    pipelineInfo.blending.enable_blending = true;
    pipelineInfo.blending.src_rgb = GFXBlendFactor::SrcAlpha;
    pipelineInfo.blending.dst_rgb = GFXBlendFactor::OneMinusSrcColor;
    
    pipelineInfo.shader_input.bindings = {
        {4, GFXBindingType::PushConstant},
        {0, GFXBindingType::StorageBuffer},
        {1, GFXBindingType::StorageBuffer},
        {2, GFXBindingType::StorageBuffer},
        {3, GFXBindingType::Texture}
    };
    
    pipelineInfo.shader_input.push_constants = {
        {sizeof(UIPushConstant), 0}
    };

    text_pipeline = gfx->create_graphics_pipeline(pipelineInfo);
    
    if(world_text_pipeline == nullptr) {
        pipelineInfo.render_pass = offscreen_render_pass;
        pipelineInfo.label = "Text World";
        pipelineInfo.depth.depth_mode = GFXDepthMode::LessOrEqual;

        world_text_pipeline = gfx->create_graphics_pipeline(pipelineInfo);
    }

    create_ui_pipelines();

    for(auto& pass : passes)
        pass->create_render_target_resources(target);
}

void renderer::recreate_all_render_targets() {
    
}

void renderer::set_screen(ui::Screen* screen) {
    Expects(screen != nullptr);
    
    current_screen = screen;

    init_screen(screen);

	update_screen();
}

void renderer::init_screen(ui::Screen* screen) {
    Expects(screen != nullptr);

    std::array<GylphMetric, numGlyphs> metrics = {};
    for(int i = 0; i < numGlyphs; i++) {
        GylphMetric& metric = metrics[i];
        metric.x0_y0 = utility::pack_u32(font.sizes[fontSize][i].x0, font.sizes[fontSize][i].y0);
        metric.x1_y1 = utility::pack_u32(font.sizes[fontSize][i].x1, font.sizes[fontSize][i].y1);
        metric.xoff = font.sizes[fontSize][i].xoff;
        metric.yoff = font.sizes[fontSize][i].yoff;
        metric.xoff2 = font.sizes[fontSize][i].xoff2;
        metric.yoff2 = font.sizes[fontSize][i].yoff2;
    }

    screen->glyph_buffer = gfx->create_buffer(nullptr, instance_alignment + (sizeof(GylphMetric) * numGlyphs), false, GFXBufferUsage::Storage);

    gfx->copy_buffer(screen->glyph_buffer, metrics.data(), instance_alignment, sizeof(GylphMetric) * numGlyphs);

    screen->elements_buffer = gfx->create_buffer(nullptr, sizeof(ElementInstance) * 50, true, GFXBufferUsage::Storage);

    screen->instance_buffer = gfx->create_buffer(nullptr, sizeof(StringInstance) * 50, true, GFXBufferUsage::Storage);
}

void renderer::update_screen() {
	if(current_screen != nullptr) {
		for(auto& element : current_screen->elements) {
			if(!element.background.image.empty())
				element.background.texture = assetm->get<Texture>(prism::app_domain / element.background.image);
		}
	}
}

void renderer::render(GFXCommandBuffer* commandbuffer, Scene* scene, RenderTarget& target, int index) {
    const auto extent = target.extent;
    const auto render_extent = target.get_render_extent();
        
    if(index > 0) {
        GFXRenderPassBeginInfo beginInfo = {};
        beginInfo.render_area.extent = render_extent;

        commandbuffer->set_render_pass(beginInfo);
        
        Viewport viewport = {};
        viewport.width = static_cast<float>(render_extent.width);
        viewport.height = static_cast<float>(render_extent.height);
        
        commandbuffer->set_viewport(viewport);
        
        for(auto& pass : passes)
            pass->render_post(commandbuffer, target, index);
        
        return;
    }

    commandbuffer->push_group("Scene Rendering");
    
    GFXRenderPassBeginInfo beginInfo = {};
    beginInfo.framebuffer = target.offscreenFramebuffer;
    beginInfo.render_pass = offscreen_render_pass;
    beginInfo.render_area.extent = render_extent;

    controller_continuity continuity;

    if(scene != nullptr) {
        commandbuffer->push_group("Shadow Rendering");
        
        shadow_pass->render(commandbuffer, *scene);
        
        commandbuffer->pop_group();
        
        if(render_options.enable_ibl) {
            commandbuffer->push_group("Scene Capture");
            scene_capture->render(commandbuffer, scene);
            commandbuffer->pop_group();
        }
        
        commandbuffer->set_render_pass(beginInfo);
        
        const auto& cameras = scene->get_all<Camera>();
        for(auto& [obj, camera] : cameras) {            
            const bool requires_limited_perspective = render_options.enable_depth_of_field;
            if(requires_limited_perspective) {
                camera.perspective = prism::perspective(radians(camera.fov),
                                                            static_cast<float>(render_extent.width) / static_cast<float>(render_extent.height),
                                                        camera.near,
                                                        100.0f);
            } else {
                camera.perspective = prism::infinite_perspective(radians(camera.fov),
                                                                     static_cast<float>(render_extent.width) / static_cast<float>(render_extent.height),
                                                                 camera.near);
            }
        
            camera.view = inverse(scene->get<Transform>(obj).model);
            
            Viewport viewport = {};
            viewport.width = static_cast<float>(render_extent.width);
            viewport.height = static_cast<float>(render_extent.height);

            commandbuffer->set_viewport(viewport);
            
            commandbuffer->push_group("render camera");

            render_camera(commandbuffer, *scene, obj, camera, render_extent, target, continuity);

            commandbuffer->pop_group();
        }
    }
    
    commandbuffer->pop_group();

    commandbuffer->push_group("SMAA");
    
    smaa_pass->render(commandbuffer, target);
    
    commandbuffer->pop_group();

    if(render_options.enable_depth_of_field && dof_pass != nullptr)
        dof_pass->render(commandbuffer, *scene);

    commandbuffer->push_group("Post Processing");

    commandbuffer->end_render_pass();

    // begin auto exposure
    
    commandbuffer->set_compute_pipeline(histogram_pipeline);
    
    commandbuffer->bind_texture(target.offscreenColorTexture, 0);
    commandbuffer->bind_shader_buffer(histogram_buffer, 0, 1, sizeof(uint32_t) * 256);
    
    const float lum_range = render_options.max_luminance - render_options.min_luminance;

    prism::float4 params = prism::float4(render_options.min_luminance,
                             1.0f / lum_range,
                             static_cast<float>(render_extent.width),
                             static_cast<float>(render_extent.height));
    
    commandbuffer->set_push_constant(&params, sizeof(prism::float4));
    
    commandbuffer->dispatch(static_cast<uint32_t>(std::ceil(static_cast<float>(render_extent.width) / 16.0f)),
                            static_cast<uint32_t>(std::ceil(static_cast<float>(render_extent.height) / 16.0f)), 1);
    
    commandbuffer->set_compute_pipeline(histogram_average_pipeline);
    
    params = prism::float4(render_options.min_luminance,
                     lum_range,
                     std::clamp(1.0f - std::exp(-(1.0f / 60.0f) * 1.1f), 0.0f, 1.0f),
                     static_cast<float>(render_extent.width * render_extent.height));
    
    commandbuffer->set_push_constant(&params, sizeof(prism::float4));
    
    commandbuffer->bind_texture(average_luminance_texture, 0);
    
    commandbuffer->dispatch(1, 1, 1);

    // continue post processing
    beginInfo.framebuffer = nullptr;
    beginInfo.render_pass = nullptr;

    commandbuffer->set_render_pass(beginInfo);

    Viewport viewport = {};
    viewport.width = static_cast<float>(render_extent.width);
    viewport.height = static_cast<float>(render_extent.height);

    commandbuffer->set_viewport(viewport);

    commandbuffer->set_graphics_pipeline(post_pipeline);
    
    if(render_options.enable_depth_of_field)
        commandbuffer->bind_texture(dof_pass->normal_field, 1);
    else
        commandbuffer->bind_texture(target.offscreenColorTexture, 1);
    
    commandbuffer->bind_texture(target.blend_texture, 3);
    
    if(auto texture = get_requested_texture(PassTextureType::SelectionSobel))
        commandbuffer->bind_texture(texture, 5);
    else
        commandbuffer->bind_texture(dummy_texture, 5);
    
    commandbuffer->bind_texture(average_luminance_texture, 6);
    
    if(render_options.enable_depth_of_field)
        commandbuffer->bind_texture(dof_pass->far_field, 7);
    else
        commandbuffer->bind_texture(dummy_texture, 7);

    PostPushConstants pc;
    pc.options.x = render_options.enable_aa;
    pc.options.z = render_options.exposure;
    
    if(render_options.enable_depth_of_field)
        pc.options.w = 2;
    
    pc.transform_ops.x = static_cast<float>(render_options.display_color_space);
    pc.transform_ops.y = static_cast<float>(render_options.tonemapping);
    
    const auto [width, height] = render_extent;
    pc.viewport = prism::float4(1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height), static_cast<float>(width), static_cast<float>(height));
    
    commandbuffer->set_push_constant(&pc, sizeof(PostPushConstants));
    
    commandbuffer->draw(0, 4, 0, 1);

    commandbuffer->pop_group();
    
    if(current_screen != nullptr)
        render_screen(commandbuffer, current_screen, extent, continuity);
    
    commandbuffer->push_group("Extra Passes");
    
    for(auto& pass : passes)
        pass->render_post(commandbuffer, target, index);
    
    commandbuffer->pop_group();

    target.current_frame = (target.current_frame + 1) % RT_MAX_FRAMES_IN_FLIGHT;
}

void renderer::render_camera(GFXCommandBuffer* command_buffer, Scene& scene, Object camera_object, Camera& camera, prism::Extent extent, RenderTarget& target, controller_continuity& continuity) {
    // frustum test
    const auto frustum = normalize_frustum(camera_extract_frustum(scene, camera_object));
            
    SceneInformation sceneInfo = {};
    sceneInfo.lightspace = scene.lightSpace;
    sceneInfo.options = prism::float4(1, 0, 0, 0);
    sceneInfo.camPos = scene.get<Transform>(camera_object).get_world_position();
    sceneInfo.camPos.w = 2.0f * camera.near * std::tan(camera.fov * 0.5f) * (static_cast<float>(extent.width) / static_cast<float>(extent.height));
    sceneInfo.vp =  camera.perspective * camera.view;
    
    for(const auto& [obj, light] : scene.get_all<Light>()) {
        SceneLight sl;
        sl.positionType = prism::float4(scene.get<Transform>(obj).get_world_position(), static_cast<float>(light.type));

        prism::float3 front = prism::float3(0.0f, 0.0f, 1.0f) * scene.get<Transform>(obj).rotation;
        
        sl.directionPower = prism::float4(-front, light.power);
        sl.colorSize = prism::float4(utility::from_srgb_to_linear(light.color), radians(light.spot_size));
        sl.shadowsEnable = prism::float4(light.enable_shadows, radians(light.size), 0, 0);
        
        sceneInfo.lights[sceneInfo.numLights++] = sl;
    }
    
    for(int i = 0; i < max_spot_shadows; i++)
        sceneInfo.spotLightSpaces[i] = scene.spotLightSpaces[i];
    
    int last_probe = 0;
    for(const auto& [obj, probe] : scene.get_all<EnvironmentProbe>()) {
        SceneProbe p;
        p.position = prism::float4(scene.get<Transform>(obj).position, probe.is_sized ? 1.0f : 2.0f);
        p.size = prism::float4(probe.size, probe.intensity);
        
        sceneInfo.probes[last_probe++] = p;
    }
    
    int numMaterialsInBuffer = 0;
    std::map<Material*, int> material_indices;
    
    const auto& meshes = scene.get_all<Renderable>();
    for(const auto& [obj, mesh] : meshes) {
        if(!mesh.mesh)
            continue;
        
        if(mesh.materials.empty())
            continue;
        
        for(auto& material : mesh.materials) {
            if(!material)
                continue;
            
            if(material->static_pipeline == nullptr || material->skinned_pipeline == nullptr)
                create_mesh_pipeline(*material.handle);
            
            if(!material_indices.count(material.handle)) {
                material_indices[material.handle] = numMaterialsInBuffer++;
            }
        }
        
        struct PushConstant {
            Matrix4x4 m;
        } pc;
        
        pc.m = scene.get<Transform>(obj).model;
        
        command_buffer->set_vertex_buffer(mesh.mesh->position_buffer, 0, position_buffer_index);
        command_buffer->set_vertex_buffer(mesh.mesh->normal_buffer, 0, normal_buffer_index);
        command_buffer->set_vertex_buffer(mesh.mesh->texture_coord_buffer, 0, texcoord_buffer_index);
        command_buffer->set_vertex_buffer(mesh.mesh->tangent_buffer, 0, tangent_buffer_index);
        command_buffer->set_vertex_buffer(mesh.mesh->bitangent_buffer, 0, bitangent_buffer_index);
        
        if(!mesh.mesh->bones.empty())
            command_buffer->set_vertex_buffer(mesh.mesh->bone_buffer, 0, bone_buffer_index);
        
        command_buffer->set_index_buffer(mesh.mesh->index_buffer, IndexType::UINT32);
        
        for(const auto& part : mesh.mesh->parts) {
            const int material_index = part.material_override == -1 ? 0 : part.material_override;
            
            if(material_index >= mesh.materials.size())
                continue;
            
            if(mesh.materials[material_index].handle == nullptr || mesh.materials[material_index]->static_pipeline == nullptr)
                continue;
            
            if(render_options.enable_frustum_culling && !test_aabb_frustum(frustum, get_aabb_for_part(scene.get<Transform>(obj), part)))
                continue;
            
            command_buffer->set_graphics_pipeline(mesh.mesh->bones.empty() ? mesh.materials[material_index]->static_pipeline : mesh.materials[material_index]->skinned_pipeline);
            
            command_buffer->bind_shader_buffer(target.sceneBuffer, 0, 1, sizeof(SceneInformation));

            command_buffer->bind_texture(scene.depthTexture, 2);
            command_buffer->bind_texture(scene.pointLightArray, 3);
            command_buffer->bind_texture(scene.spotLightArray, 6);
            command_buffer->bind_texture(scene.irradianceCubeArray, 7);
            command_buffer->bind_texture(scene.prefilteredCubeArray, 8);
            command_buffer->bind_texture(brdf_texture, 9);

            if(!mesh.mesh->bones.empty())
                command_buffer->bind_shader_buffer(part.bone_batrix_buffer, 0, 14, sizeof(Matrix4x4) * 128);
            
            command_buffer->set_push_constant(&pc, sizeof(PushConstant));
            
            for(const auto& [index, texture] : mesh.materials[material_index]->bound_textures) {
                GFXTexture* texture_to_bind = dummy_texture;
                if(texture)
                    texture_to_bind = texture->handle;
                
                command_buffer->bind_texture(texture_to_bind, index);
            }
            
            command_buffer->draw_indexed(part.index_count, part.index_offset, part.vertex_offset, 0);
        }
    }
    
    const auto& screens = scene.get_all<UI>();
    for(const auto& [obj, screen] : screens) {
        if(!screen.screen)
            continue;
        
        render_screen_options options = {};
        options.render_world = true;
        options.mvp = camera.perspective * camera.view * scene.get<Transform>(obj).model;
        
        render_screen(command_buffer, screen.screen, extent, continuity, options);
    }
    
    SkyPushConstant pc;
    pc.view = matrix_from_quat(scene.get<Transform>(camera_object).rotation);
    pc.aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    
    for(const auto& [obj, light] : scene.get_all<Light>()) {
        if(light.type == Light::Type::Sun)
            pc.sun_position_fov = prism::float4(scene.get<Transform>(obj).get_world_position(), radians(camera.fov));
    }
    
    command_buffer->set_graphics_pipeline(sky_pipeline);
    
    command_buffer->set_push_constant(&pc, sizeof(SkyPushConstant));
    
    command_buffer->draw(0, 4, 0, 1);
    
    if(render_options.enable_extra_passes) {
        for(auto& pass : passes)
            pass->render_scene(scene, command_buffer);
    }
    
    gfx->copy_buffer(target.sceneBuffer, &sceneInfo, 0, sizeof(SceneInformation));
}

void renderer::render_screen(GFXCommandBuffer *commandbuffer, ui::Screen* screen, prism::Extent extent, controller_continuity& continuity, render_screen_options options) {
    std::array<GlyphInstance, maxInstances> instances;
    std::vector<ElementInstance> elementInstances;
    std::array<StringInstance, 50> stringInstances = {};

    int stringLen = 0;
    int numElements = 0;

    for(auto [i, element] : utility::enumerate(screen->elements)) {
        if(!element.visible)
            continue;

        ElementInstance instance;

        instance.position = utility::pack_u32(utility::to_fixed(element.absolute_x), utility::to_fixed(element.absolute_y));
        instance.size = utility::pack_u32(utility::to_fixed(element.absolute_width), utility::to_fixed(element.absolute_height));
        instance.color[0] = element.background.color.r;
        instance.color[1] = element.background.color.g;
        instance.color[2] = element.background.color.b;
        instance.color[3] = element.background.color.a;

        elementInstances.push_back(instance);

        stringInstances[i].xy = utility::pack_u32(utility::to_fixed(element.absolute_x + element.text_x), utility::to_fixed(element.absolute_y + element.text_y));

        float advance = 0.0f;
        float yadvance = 0.0f;

        bool do_y_advance = false; // so we can not advance the text by a space if we're wrapping per word

        for(size_t j = 0; j < element.text.length(); j++) {
            auto index = element.text[j] - 32;

            if(do_y_advance) {
                advance = 0.0f;
                yadvance += font.ascentSizes[fontSize];
                do_y_advance = false;
            }

            if(element.wrap_text) {
                if(element.text[j] == ' ') {
                    std::string t;
                    float temp_advance = 0.0f;
                    for(size_t k = j + 1; k < element.text.length() + j; k++) {
                        t += element.text[k];
                        auto index = element.text[k] - 32;
                        temp_advance += font.sizes[fontSize][index].xadvance;

                        if(element.text[k] == ' ')
                            break;
                    }

                    if((temp_advance + advance) >= element.absolute_width)
                        do_y_advance = true;
                }
            }

            GlyphInstance& instance = instances[stringLen + j];
            instance.position = utility::pack_u32(utility::to_fixed(advance), utility::to_fixed(yadvance));
            instance.index = utility::pack_u32(index, 0);
            instance.instance = i;

            advance += font.sizes[fontSize][index].xadvance;

            if(element.text[j] == '\n') {
                advance = 0.0f;
                yadvance += font.ascentSizes[fontSize];
            }
        }

        stringLen += static_cast<int>(element.text.length());
        numElements++;
    }

    gfx->copy_buffer(screen->glyph_buffer, instances.data(), 0, instance_alignment);
    gfx->copy_buffer(screen->instance_buffer, stringInstances.data(), 0, sizeof(StringInstance) * 50);
    gfx->copy_buffer(screen->elements_buffer, elementInstances.data(), sizeof(ElementInstance) * continuity.elementOffset, sizeof(ElementInstance) * elementInstances.size());

    const prism::float2 windowSize = {static_cast<float>(extent.width), static_cast<float>(extent.height)};

    for(auto [i, element] : utility::enumerate(screen->elements)) {
        if(!element.visible)
            continue;

        UIPushConstant pc;
        pc.screenSize = windowSize;

        if(options.render_world) {
            commandbuffer->set_graphics_pipeline(world_general_pipeline);
            pc.mvp = options.mvp;
        } else {
            commandbuffer->set_graphics_pipeline(general_pipeline);
        }

        if (element.background.texture) {
            commandbuffer->bind_texture(element.background.texture->handle, 2);
        }
        else {
            commandbuffer->bind_texture(dummy_texture, 2);
        }

        commandbuffer->set_push_constant(&pc, sizeof(UIPushConstant));

        commandbuffer->bind_shader_buffer(screen->elements_buffer, 0, 0, sizeof(ElementInstance) * 50);
        commandbuffer->draw(0, 4, i + continuity.elementOffset, 1);
    }

    UIPushConstant pc;
    pc.screenSize = windowSize;

    if(options.render_world) {
        commandbuffer->set_graphics_pipeline(world_text_pipeline);
        pc.mvp = options.mvp;
    } else {
        commandbuffer->set_graphics_pipeline(text_pipeline);
    }

    commandbuffer->set_push_constant(&pc, sizeof(UIPushConstant));

    commandbuffer->bind_shader_buffer(screen->glyph_buffer, 0, 0, instance_alignment);
    commandbuffer->bind_shader_buffer(screen->glyph_buffer, instance_alignment, 1, sizeof(GylphMetric) * numGlyphs);
    commandbuffer->bind_shader_buffer(screen->instance_buffer, 0, 2, sizeof(StringInstance) * 50);
    commandbuffer->bind_texture(font_texture, 3);

    if(stringLen > 0)
        commandbuffer->draw(0, 4, 0, stringLen);

    continuity.elementOffset += numElements;
}

void renderer::create_mesh_pipeline(Material& material) const {
    GFXShaderConstant materials_constant = {};
    materials_constant.type = GFXShaderConstant::Type::Integer;
    materials_constant.value = max_scene_materials;
    
    GFXShaderConstant lights_constant = {};
    lights_constant.index = 1;
    lights_constant.type = GFXShaderConstant::Type::Integer;
    lights_constant.value = max_scene_lights;
    
    GFXShaderConstant spot_lights_constant = {};
    spot_lights_constant.index = 2;
    spot_lights_constant.type = GFXShaderConstant::Type::Integer;
    spot_lights_constant.value = max_spot_shadows;
    
    GFXShaderConstant probes_constant = {};
    probes_constant.index = 3;
    probes_constant.type = GFXShaderConstant::Type::Integer;
    probes_constant.value = max_environment_probes;
    
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "Mesh";
    pipelineInfo.shaders.vertex_src = ShaderSource(prism::path("mesh.vert"));
    pipelineInfo.shaders.fragment_src = ShaderSource(prism::path("mesh.frag"));
    
    pipelineInfo.shaders.vertex_constants = {materials_constant, lights_constant, spot_lights_constant, probes_constant};
    pipelineInfo.shaders.fragment_constants = {materials_constant, lights_constant, spot_lights_constant, probes_constant};
    
    pipelineInfo.shader_input.push_constants = {
        {sizeof(Matrix4x4), 0}
    };
    
    pipelineInfo.shader_input.bindings = {
        {1, GFXBindingType::StorageBuffer},
        {0, GFXBindingType::PushConstant},
        {2, GFXBindingType::Texture},
        {3, GFXBindingType::Texture},
        {6, GFXBindingType::Texture},
        {7, GFXBindingType::Texture},
        {8, GFXBindingType::Texture},
        {9, GFXBindingType::Texture}
    };
    
    pipelineInfo.render_pass = offscreen_render_pass;
    pipelineInfo.depth.depth_mode = GFXDepthMode::Less;
    pipelineInfo.rasterization.culling_mode = GFXCullingMode::Backface;
    pipelineInfo.blending.src_rgb = GFXBlendFactor::SrcAlpha;
    pipelineInfo.blending.dst_rgb = GFXBlendFactor::OneMinusSrcAlpha;
    
    pipelineInfo.shaders.fragment_src = material_compiler.compile_material_fragment(material);

    for (auto [index, texture] : material.bound_textures) {
        GFXShaderBinding binding;
        binding.binding = index;
        binding.type = GFXBindingType::Texture;

        pipelineInfo.shader_input.bindings.push_back(binding);
    }
    
    auto [static_pipeline, skinned_pipeline] = material_compiler.create_pipeline_permutations(pipelineInfo);
    
    material.static_pipeline = static_pipeline;
    material.skinned_pipeline = skinned_pipeline;
    
    pipelineInfo.render_pass = scene_capture->renderPass;
    
    pipelineInfo.shaders.fragment_src = material_compiler.compile_material_fragment(material, false); // scene capture does not use IBL
    
    pipelineInfo.shader_input.push_constants[0].size += sizeof(Matrix4x4);

    material.capture_pipeline = material_compiler.create_static_pipeline(pipelineInfo, false, true);
}

void renderer::create_dummy_texture() {
    GFXTextureCreateInfo createInfo = {};
    createInfo.label = "Dummy";
    createInfo.width = 1;
    createInfo.height = 1;
    createInfo.format = GFXPixelFormat::R8G8B8A8_UNORM;
    createInfo.usage = GFXTextureUsage::Sampled | GFXTextureUsage::TransferDst;

    dummy_texture = gfx->create_texture(createInfo);

    uint8_t tex[4] = {0, 0, 0, 0};

    gfx->copy_texture(dummy_texture, tex, sizeof(tex));
}

void renderer::create_render_target_resources(RenderTarget& target) {
    const auto extent = target.get_render_extent();
    
    GFXTextureCreateInfo textureInfo = {};
    textureInfo.label = "Offscreen Color";
    textureInfo.width = extent.width;
    textureInfo.height = extent.height;
    textureInfo.format = GFXPixelFormat::RGBA_32F;
    textureInfo.usage = GFXTextureUsage::Attachment | GFXTextureUsage::Sampled | GFXTextureUsage::Storage;
    textureInfo.samplingMode = SamplingMode::ClampToEdge;

    target.offscreenColorTexture = gfx->create_texture(textureInfo);

    textureInfo.label = "Offscreen Depth";
    textureInfo.format = GFXPixelFormat::DEPTH_32F;
    textureInfo.usage = GFXTextureUsage::Attachment | GFXTextureUsage::Sampled;

    target.offscreenDepthTexture = gfx->create_texture(textureInfo);

    GFXFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.render_pass = offscreen_render_pass;
    framebufferInfo.attachments.push_back(target.offscreenColorTexture);
    framebufferInfo.attachments.push_back(target.offscreenDepthTexture);

    target.offscreenFramebuffer = gfx->create_framebuffer(framebufferInfo);
    
    if(post_pipeline == nullptr) {
        GFXGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.label = "Post";
        
        pipelineInfo.shaders.vertex_src = ShaderSource(prism::path("post.vert"));
        pipelineInfo.shaders.fragment_src = ShaderSource(prism::path("post.frag"));
        
        pipelineInfo.shader_input.bindings = {
            {4, GFXBindingType::PushConstant},
            {1, GFXBindingType::Texture},
            {2, GFXBindingType::Texture},
            {3, GFXBindingType::Texture},
            {5, GFXBindingType::Texture},
            {6, GFXBindingType::Texture},
            {7, GFXBindingType::Texture}
        };
        
        pipelineInfo.shader_input.push_constants = {
            {sizeof(PostPushConstants), 0}
        };

        post_pipeline = gfx->create_graphics_pipeline(pipelineInfo);
    }
    
    target.sceneBuffer = gfx->create_buffer(nullptr, sizeof(SceneInformation), true, GFXBufferUsage::Storage);
}

void renderer::create_post_pipelines() {
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "Post";
    
    pipelineInfo.shaders.vertex_src = ShaderSource(prism::path("post.vert"));
    pipelineInfo.shaders.fragment_src = ShaderSource(prism::path("post.frag"));
    
    pipelineInfo.shader_input.bindings = {
        {4, GFXBindingType::PushConstant},
        {1, GFXBindingType::Texture},
        {2, GFXBindingType::Texture},
        {3, GFXBindingType::Texture},
        {5, GFXBindingType::Texture},
        {6, GFXBindingType::Texture},
        {7, GFXBindingType::Texture}
    };
    
    pipelineInfo.shader_input.push_constants = {
        {sizeof(PostPushConstants), 0}
    };

    post_pipeline = gfx->create_graphics_pipeline(pipelineInfo);
}

void renderer::create_font_texture() {
    auto file = prism::open_file(prism::app_domain / "font.fp", true);
    if(file == std::nullopt) {
        prism::log::error(System::Renderer, "Failed to load font file!");
        return;
    }

    file->read(&font);

    std::vector<unsigned char> bitmap(font.width * font.height);
    file->read(bitmap.data(), font.width * font.height);

    instance_alignment = (int)gfx->get_alignment(sizeof(GlyphInstance) * maxInstances);

    GFXTextureCreateInfo textureInfo = {};
    textureInfo.label = "UI Font";
    textureInfo.width = font.width;
    textureInfo.height = font.height;
    textureInfo.format = GFXPixelFormat::R8_UNORM;
    textureInfo.usage = GFXTextureUsage::Sampled | GFXTextureUsage::TransferDst;

    font_texture = gfx->create_texture(textureInfo);

    gfx->copy_texture(font_texture, bitmap.data(), font.width * font.height);
}

void renderer::create_sky_pipeline() {
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "Sky";
    pipelineInfo.render_pass = offscreen_render_pass;

    pipelineInfo.shaders.vertex_src = register_shader("sky.vert");
    pipelineInfo.shaders.fragment_src = register_shader("sky.frag");

    pipelineInfo.shader_input.bindings = {
        {1, GFXBindingType::PushConstant}
    };

    pipelineInfo.shader_input.push_constants = {
        {sizeof(SkyPushConstant), 0}
    };

    pipelineInfo.depth.depth_mode = GFXDepthMode::LessOrEqual;

    sky_pipeline = gfx->create_graphics_pipeline(pipelineInfo);
    
    associate_shader_reload("sky.vert", [this] {
        create_sky_pipeline();
    });
    
    associate_shader_reload("sky.frag", [this] {
        create_sky_pipeline();
    });
}

void renderer::create_ui_pipelines() {
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "UI";

    pipelineInfo.shaders.vertex_src = ShaderSource(prism::path("ui.vert"));
    pipelineInfo.shaders.fragment_src = ShaderSource(prism::path("ui.frag"));

    pipelineInfo.rasterization.primitive_type = GFXPrimitiveType::TriangleStrip;

    pipelineInfo.blending.enable_blending = true;
    pipelineInfo.blending.src_rgb = GFXBlendFactor::SrcAlpha;
    pipelineInfo.blending.dst_rgb = GFXBlendFactor::OneMinusSrcAlpha;

    pipelineInfo.shader_input.bindings = {
        {1, GFXBindingType::PushConstant},
        {0, GFXBindingType::StorageBuffer},
        {2, GFXBindingType::Texture}
    };

    pipelineInfo.shader_input.push_constants = {
        {sizeof(UIPushConstant), 0}
    };

    general_pipeline = gfx->create_graphics_pipeline(pipelineInfo);

    pipelineInfo.label = "UI World";
    pipelineInfo.render_pass = offscreen_render_pass;
    pipelineInfo.depth.depth_mode = GFXDepthMode::LessOrEqual;

    world_general_pipeline = gfx->create_graphics_pipeline(pipelineInfo);
}

void renderer::generate_brdf() {
    GFXRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.label = "BRDF Gen";
    renderPassInfo.attachments.push_back(GFXPixelFormat::R8G8_SFLOAT);
    renderPassInfo.will_use_in_shader = true;

    brdf_render_pass = gfx->create_render_pass(renderPassInfo);
    
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "BRDF";
    
    pipelineInfo.shaders.vertex_src = ShaderSource(prism::path("brdf.vert"));
    pipelineInfo.shaders.fragment_src = ShaderSource(prism::path("brdf.frag"));
    
    pipelineInfo.render_pass = brdf_render_pass;

    brdf_pipeline = gfx->create_graphics_pipeline(pipelineInfo);
    
    GFXTextureCreateInfo textureInfo = {};
    textureInfo.label = "BRDF LUT";
    textureInfo.format = GFXPixelFormat::R8G8_SFLOAT;
    textureInfo.width = brdf_resolution;
    textureInfo.height = brdf_resolution;
    textureInfo.usage = GFXTextureUsage::Sampled | GFXTextureUsage::Attachment;

    brdf_texture = gfx->create_texture(textureInfo);
    
    GFXFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.attachments = {brdf_texture};
    framebufferInfo.render_pass = brdf_render_pass;

    brdf_framebuffer = gfx->create_framebuffer(framebufferInfo);
    
    // render
    GFXCommandBuffer* command_buffer = gfx->acquire_command_buffer();
    
    GFXRenderPassBeginInfo beginInfo = {};
    beginInfo.render_pass = brdf_render_pass;
    beginInfo.framebuffer = brdf_framebuffer;
    beginInfo.render_area.extent = {brdf_resolution, brdf_resolution};
    
    command_buffer->set_render_pass(beginInfo);
    
    command_buffer->set_graphics_pipeline(brdf_pipeline);
    
    Viewport viewport = {};
    viewport.width = brdf_resolution;
    viewport.height = brdf_resolution;
    
    command_buffer->set_viewport(viewport);
    
    command_buffer->draw(0, 4, 0, 1);
    
    gfx->submit(command_buffer);
}

void renderer::create_histogram_resources() {
    GFXComputePipelineCreateInfo create_info = {};
    create_info.compute_src = ShaderSource(prism::path("histogram.comp"));
    create_info.workgroup_size_x = 16;
    create_info.workgroup_size_y = 16;
    
    create_info.shader_input.bindings = {
        {0, GFXBindingType::StorageImage},
        {1, GFXBindingType::StorageBuffer},
        {2, GFXBindingType::PushConstant}
    };

    create_info.shader_input.push_constants = {
        {sizeof(prism::float4), 0}
    };
    
    histogram_pipeline = gfx->create_compute_pipeline(create_info);
    
    create_info.compute_src = ShaderSource(prism::path("histogram-average.comp"));
    create_info.workgroup_size_x = 256;
    create_info.workgroup_size_y = 1;
    
    histogram_average_pipeline = gfx->create_compute_pipeline(create_info);

    histogram_buffer = gfx->create_buffer(nullptr, sizeof(uint32_t) * 256, false, GFXBufferUsage::Storage);
    
    GFXTextureCreateInfo texture_info = {};
    texture_info.label = "Average Luminance Store";
    texture_info.width = 1;
    texture_info.height = 1;
    texture_info.format = GFXPixelFormat::R_16F;
    texture_info.usage = GFXTextureUsage::Sampled | GFXTextureUsage::ShaderWrite | GFXTextureUsage::Storage;
    
    average_luminance_texture = gfx->create_texture(texture_info);
}

ShaderSource renderer::register_shader(const std::string_view shader_file) {
    if(!reloading_shader) {
        RegisteredShader shader;
        shader.filename = shader_file;
        
        registered_shaders.push_back(shader);
    }
    
    std::string found_shader_source;
    for(auto& shader : registered_shaders) {
        if(shader.filename == shader_file) {
            found_shader_source = shader.injected_shader_source;
        }
    }
    
    prism::path base_shader_path = get_shader_source_directory();
    
    // if shader editor system is not initialized, use prebuilt shaders
    if(base_shader_path.empty())
        return ShaderSource(prism::path(shader_file));
    
    shader_compiler.set_include_path(base_shader_path.string());
    
    prism::path shader_path = prism::path(shader_file);
    
    ShaderStage stage = ShaderStage::Vertex;
    if(shader_path.extension() == ".vert")
        stage = ShaderStage::Vertex;
    else if(shader_path.extension() == ".frag")
        stage = ShaderStage::Fragment;

    if(found_shader_source.empty()) {
        auto file = prism::open_file(base_shader_path / shader_path.replace_extension(shader_path.extension().string() + ".glsl"));
        
        return shader_compiler.compile(ShaderLanguage::GLSL, stage, ShaderSource(file->read_as_string()), gfx->accepted_shader_language()).value();
    } else {
        return shader_compiler.compile(ShaderLanguage::GLSL, stage, ShaderSource(found_shader_source), gfx->accepted_shader_language()).value();
    }
}

void renderer::associate_shader_reload(const std::string_view shader_file, const std::function<void()>& reload_function) {
    if(reloading_shader)
        return;
    
    for(auto& shader : registered_shaders) {
        if(shader.filename == shader_file)
            shader.reload_function = reload_function;
    }
}

void renderer::reload_shader(const std::string_view shader_file, const std::string_view shader_source) {
    for(auto& shader : registered_shaders) {
        if(shader.filename == shader_file) {
            shader.injected_shader_source = shader_source;
            reloading_shader = true;
            shader.reload_function();
            reloading_shader = false;
        }
    }
}
