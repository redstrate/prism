#include "renderer.hpp"

#include <array>

#include "gfx_commandbuffer.hpp"
#include "math.hpp"
#include "screen.hpp"
#include "file.hpp"
#include "scene.hpp"
#include "font.hpp"
#include "vector.hpp"
#include "engine.hpp"
#include "imguipass.hpp"
#include "gfx.hpp"
#include "log.hpp"
#include "gaussianhelper.hpp"
#include "pass.hpp"
#include "shadowpass.hpp"
#include "smaapass.hpp"
#include "scenecapture.hpp"
#include "shadercompiler.hpp"
#include "assertions.hpp"
#include "dofpass.hpp"
#include "frustum.hpp"

struct ElementInstance {
    Vector4 color;
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
    Vector4 color, info;
};

struct SceneLight {
    Vector4 positionType;
    Vector4 directionPower;
    Vector4 colorSize;
    Vector4 shadowsEnable;
};

struct SceneProbe {
    Vector4 position, size;
};

struct SceneInformation {
    Vector4 options;
    Vector4 camPos;
    Matrix4x4 vp, lightspace;
    Matrix4x4 spotLightSpaces[max_spot_shadows];
    SceneMaterial materials[max_scene_materials];
    SceneLight lights[max_scene_lights];
    SceneProbe probes[max_environment_probes];
    int numLights;
    int p[3];
};

struct PostPushConstants {
    Vector4 viewport;
    Vector4 options;
};

struct UIPushConstant {
    Vector2 screenSize;
    Matrix4x4 mvp;
};

Renderer::Renderer(GFX* gfx, const bool enable_imgui) : gfx(gfx) {
    Expects(gfx != nullptr);
    
    createDummyTexture();

    shadow_pass = std::make_unique<ShadowPass>(gfx);
    scene_capture = std::make_unique<SceneCapture>(gfx);
    
    if(enable_imgui)
        addPass<ImGuiPass>();
    
    if(gfx->required_context() == GFXContext::Metal)
        correction_matrix[1][1] *= -1.0f;
    
    createBRDF();
}

void Renderer::resize(const Extent extent) {
    this->extent = extent;

    if(!viewport_mode) {
        createOffscreenResources();
        createMeshPipeline();
        createPostPipeline();
        createFontPipeline();
        createSkyPipeline();
        createUIPipeline();
        createGaussianResources();

        smaaPass = std::make_unique<SMAAPass>(gfx, this);
        dofPass = std::make_unique<DoFPass>(gfx, this);
    }

    for(auto& pass : passes)
        pass->resize(extent);
}

void Renderer::resize_viewport(const Extent extent) {
    this->viewport_extent = extent;

    createOffscreenResources();
    createMeshPipeline();
    createPostPipeline();
    createFontPipeline();
    createSkyPipeline();
    createUIPipeline();
    createGaussianResources();

    smaaPass = std::make_unique<SMAAPass>(gfx, this);
    dofPass = std::make_unique<DoFPass>(gfx, this);

    for(auto& pass : passes)
        pass->resize(get_render_extent());
}

void Renderer::set_screen(ui::Screen* screen) {
    Expects(screen != nullptr);
    
    current_screen = screen;

    init_screen(screen);

	update_screen();
}

void Renderer::init_screen(ui::Screen* screen) {
    Expects(screen != nullptr);

    std::array<GylphMetric, numGlyphs> metrics;
    for(int i = 0; i < numGlyphs; i++) {
        GylphMetric& metric = metrics[i];
        metric.x0_y0 = utility::pack_u32(font.sizes[fontSize][i].x0, font.sizes[fontSize][i].y0);
        metric.x1_y1 = utility::pack_u32(font.sizes[fontSize][i].x1, font.sizes[fontSize][i].y1);
        metric.xoff = font.sizes[fontSize][i].xoff;
        metric.yoff = font.sizes[fontSize][i].yoff;
        metric.xoff2 = font.sizes[fontSize][i].xoff2;
        metric.yoff2 = font.sizes[fontSize][i].yoff2;
    }

    screen->glyph_buffer = gfx->create_buffer(nullptr, instanceAlignment + (sizeof(GylphMetric) * numGlyphs), false, GFXBufferUsage::Storage);

    gfx->copy_buffer(screen->glyph_buffer, metrics.data(), instanceAlignment, sizeof(GylphMetric) * numGlyphs);

    screen->elements_buffer = gfx->create_buffer(nullptr, sizeof(ElementInstance) * 50, true, GFXBufferUsage::Storage);

    screen->instance_buffer = gfx->create_buffer(nullptr, sizeof(StringInstance) * 50, true, GFXBufferUsage::Storage);
}

void Renderer::update_screen() {
	if(current_screen != nullptr) {
        if(current_screen->blurs_background) {
            startSceneBlur();
        } else {
            stopSceneBlur();
        }

		for(auto& element : current_screen->elements) {
			if(!element.background.image.empty())
				element.background.texture = assetm->get<Texture>(file::app_domain / element.background.image);
		}
	}
}

void Renderer::startCrossfade() {
    // copy the current screen
    gfx->copy_texture(offscreenColorTexture, offscreenBackTexture);

    fading = true;
    fade = 1.0f;
}

void Renderer::startSceneBlur() {
    blurring = true;
    hasToStore = true;
}

void Renderer::stopSceneBlur() {
    blurring = false;
}

void Renderer::render(Scene* scene, int index) {
    GFXCommandBuffer* commandbuffer = engine->get_gfx()->acquire_command_buffer();

    const auto extent = get_extent();
    const auto render_extent = get_render_extent();
    
    if(!gui_only_mode) {
        commandbuffer->push_group("Scene Rendering");
        
        GFXRenderPassBeginInfo beginInfo = {};
        beginInfo.framebuffer = offscreenFramebuffer;
        beginInfo.render_pass = offscreenRenderPass;
        beginInfo.render_area.extent = render_extent;
        
        bool hasToRender = true;
        if(blurring && !hasToStore)
            hasToRender = false;

        ControllerContinuity continuity;

        if(scene != nullptr && hasToRender) {
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
                const int actual_width = render_extent.width / cameras.size();
                
                camera.perspective = transform::perspective(radians(camera.fov), static_cast<float>(actual_width) / static_cast<float>(render_extent.height), camera.near);
                camera.view = inverse(scene->get<Transform>(obj).model);
                
                Viewport viewport = {};
                viewport.x = (actual_width * 0);
                viewport.width = actual_width;
                viewport.height = render_extent.height;
                
                commandbuffer->set_viewport(viewport);
                
                commandbuffer->push_group("render camera");

                render_camera(commandbuffer, *scene, obj, camera, continuity);
                
                commandbuffer->pop_group();
            }
        }
        
        commandbuffer->pop_group();

        commandbuffer->push_group("SMAA");
        
        smaaPass->render(commandbuffer);
        
        commandbuffer->pop_group();

        if(!viewport_mode) {
            beginInfo.framebuffer = nullptr;
            beginInfo.render_pass = nullptr;
        } else {
            beginInfo.framebuffer = viewportFramebuffer;
            beginInfo.render_pass = viewportRenderPass;
        }
        
        commandbuffer->set_render_pass(beginInfo);
        
        Viewport viewport = {};
        viewport.width = render_extent.width;
        viewport.height = render_extent.height;
        
        commandbuffer->set_viewport(viewport);
        
        commandbuffer->push_group("Post Processing");
    
        commandbuffer->set_pipeline(viewport_mode ? renderToViewportPipeline : postPipeline);
        commandbuffer->bind_texture(offscreenColorTexture, 1);
        commandbuffer->bind_texture(offscreenBackTexture, 2);
        commandbuffer->bind_texture(smaaPass->blend_texture, 3);
        
        if(auto texture = get_requested_texture(PassTextureType::SelectionSobel)) {
            commandbuffer->bind_texture(texture, 4);
        } else {
            commandbuffer->bind_texture(dummyTexture, 4);
        }
        
        PostPushConstants pc;
        pc.options.x = render_options.enable_aa;
        pc.options.y = fade;
        pc.options.z = 1.0;
        
        const auto [width, height] = render_extent;
        pc.viewport = Vector4(1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height), width, height);
        
        commandbuffer->set_push_constant(&pc, sizeof(PostPushConstants));
        
        if(fading) {
            if(fade > 0.0f)
                fade -= 0.1f;
            else if(fade <= 0.0f) {
                fading = false;
                fade = 0.0f;
            }
        }
        
        commandbuffer->draw(0, 4, 0, 1);
        
        commandbuffer->pop_group();
        
        if(current_screen != nullptr)
            render_screen(commandbuffer, current_screen, continuity);
    } else {
        GFXRenderPassBeginInfo beginInfo = {};
        beginInfo.render_area.extent = extent;

        commandbuffer->set_render_pass(beginInfo);
        
        Viewport viewport = {};
        viewport.width = extent.width;
        viewport.height = extent.height;
        
        commandbuffer->set_viewport(viewport);
    }
    
    commandbuffer->push_group("Extra Passes");
    
    for(auto& pass : passes)
        pass->render_post(commandbuffer, index);
    
    commandbuffer->pop_group();

    gfx->submit(commandbuffer, index);
}

void Renderer::render_camera(GFXCommandBuffer* command_buffer, Scene& scene, Object camera_object, Camera& camera, ControllerContinuity& continuity) {
    // frustum test
    const auto frustum = normalize_frustum(camera_extract_frustum(scene, camera_object));
        
    const auto extent = get_render_extent();
    
    SceneInformation sceneInfo = {};
    sceneInfo.lightspace = scene.lightSpace;
    sceneInfo.options = Vector4(1, 0, 0, 0);
    sceneInfo.camPos = scene.get<Transform>(camera_object).get_world_position();
    sceneInfo.camPos.w = 2.0f * camera.near * std::tanf(camera.fov * 0.5f) * (static_cast<float>(extent.width) / static_cast<float>(extent.height));
    sceneInfo.vp =  camera.perspective * correction_matrix * camera.view;
    
    for(const auto [obj, light] : scene.get_all<Light>()) {
        SceneLight sl;
        sl.positionType = Vector4(scene.get<Transform>(obj).get_world_position(), (int)light.type);
        
        Vector3 front = Vector3(0.0f, 0.0f, 1.0f) * scene.get<Transform>(obj).rotation;
        
        sl.directionPower = Vector4(-front, light.power);
        sl.colorSize = Vector4(light.color, radians(light.spot_size));
        sl.shadowsEnable = Vector4(light.enable_shadows, radians(light.size), 0, 0);
        
        sceneInfo.lights[sceneInfo.numLights++] = sl;
    }
    
    for(int i = 0; i < max_spot_shadows; i++)
        sceneInfo.spotLightSpaces[i] = scene.spotLightSpaces[i];
    
    int last_probe = 0;
    for(const auto [obj, probe] : scene.get_all<EnvironmentProbe>()) {
        SceneProbe p;
        p.position = Vector4(scene.get<Transform>(obj).position, probe.is_sized ? 1.0f : 2.0f);
        p.size = Vector4(probe.size, probe.intensity);
        
        sceneInfo.probes[last_probe++] = p;
    }
    
    int numMaterialsInBuffer = 0;
    std::map<Material*, int> material_indices;
    
    const auto& meshes = scene.get_all<Renderable>();
    for(const auto [obj, mesh] : meshes) {
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
            int s;
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
        
        command_buffer->bind_shader_buffer(sceneBuffer, 0, 1, sizeof(SceneInformation));
        
        command_buffer->bind_texture(scene.depthTexture, 2);
        command_buffer->bind_texture(scene.pointLightArray, 3);
        command_buffer->bind_sampler(shadow_pass->shadow_sampler, 4);
        command_buffer->bind_sampler(shadow_pass->pcf_sampler, 5);
        command_buffer->bind_texture(scene.spotLightArray, 6);
        command_buffer->bind_texture(scene.irradianceCubeArray, 7);
        command_buffer->bind_texture(scene.prefilteredCubeArray, 8);
        command_buffer->bind_texture(brdfTexture, 9);
        
        for(const auto& part : mesh.mesh->parts) {
            const int material_index = part.material_override == -1 ? 0 : part.material_override;
            
            if(material_index >= mesh.materials.size())
                continue;
            
            if(mesh.materials[material_index].handle == nullptr || mesh.materials[material_index]->static_pipeline == nullptr)
                continue;
            
            if(render_options.enable_frustum_culling && !test_aabb_frustum(frustum, get_aabb_for_part(scene.get<Transform>(obj), part))) {
                console::debug(System::Renderer, "Culled {}!", scene.get(obj).name);
                continue;
            }
            
            command_buffer->set_pipeline(mesh.mesh->bones.empty() ? mesh.materials[material_index]->static_pipeline : mesh.materials[material_index]->skinned_pipeline);
            
            if(!mesh.mesh->bones.empty())
                command_buffer->bind_shader_buffer(part.bone_batrix_buffer, 0, 14, sizeof(Matrix4x4) * 128);
            
            pc.s = material_indices[mesh.materials[material_index].handle];
            command_buffer->set_push_constant(&pc, sizeof(PushConstant));
            
            for(const auto& [index, texture] : mesh.materials[material_index]->bound_textures) {
                GFXTexture* texture_to_bind = dummyTexture;
                if(texture)
                    texture_to_bind = texture->handle;
                
                command_buffer->bind_texture(texture_to_bind, index);
            }
            
            command_buffer->draw_indexed(part.index_count, part.index_offset, part.vertex_offset);
        }
    }
    
    const auto& screens = scene.get_all<UI>();
    for(const auto& [obj, screen] : screens) {
        if(!screen.screen)
            continue;
        
        RenderScreenOptions options = {};
        options.render_world = true;
        options.mvp = camera.perspective * camera.view * correction_matrix * scene.get<Transform>(obj).model;
        
        render_screen(command_buffer, screen.screen, continuity, options);
    }
    
    struct SkyPushConstant {
        Matrix4x4 view;
        Vector4 aspectFovY;
        Vector4 sunPosition;
    } pc;
    
    pc.view = matrix_from_quat(scene.get<Transform>(camera_object).rotation) * correction_matrix;
    pc.aspectFovY = Vector4(static_cast<float>(extent.width) / static_cast<float>(extent.height), radians(camera.fov), 0, 0);
    
    for(const auto& [obj, light] : scene.get_all<Light>()) {
        if(light.type == Light::Type::Sun)
            pc.sunPosition = Vector4(scene.get<Transform>(obj).get_world_position());
    }
    
    command_buffer->set_pipeline(skyPipeline);
    
    command_buffer->set_push_constant(&pc, sizeof(SkyPushConstant));
    
    command_buffer->draw(0, 4, 0, 1);
    
    if(render_options.enable_extra_passes) {
        for(auto& pass : passes)
            pass->render_scene(scene, command_buffer);
    }
    
    gfx->copy_buffer(sceneBuffer, &sceneInfo, 0, sizeof(SceneInformation));
}

void Renderer::render_screen(GFXCommandBuffer *commandbuffer, ui::Screen* screen, ControllerContinuity& continuity, RenderScreenOptions options) {
    std::array<GlyphInstance, maxInstances> instances;
    std::vector<ElementInstance> elementInstances;
    std::array<StringInstance, 50> stringInstances;

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

    gfx->copy_buffer(screen->glyph_buffer, instances.data(), 0, instanceAlignment);
    gfx->copy_buffer(screen->instance_buffer, stringInstances.data(), 0, sizeof(StringInstance) * 50);
    gfx->copy_buffer(screen->elements_buffer, elementInstances.data(), sizeof(ElementInstance) * continuity.elementOffset, sizeof(ElementInstance) * elementInstances.size());

    const auto extent = get_render_extent();
    const Vector2 windowSize = {static_cast<float>(extent.width), static_cast<float>(extent.height)};

    for(auto [i, element] : utility::enumerate(screen->elements)) {
        if(!element.visible)
            continue;

        if(element.background.texture) {
            commandbuffer->bind_texture(element.background.texture->handle, 2);
        } else {
            commandbuffer->bind_texture(dummyTexture, 2);
        }

        UIPushConstant pc;
        pc.screenSize = windowSize;

        if(options.render_world) {
            commandbuffer->set_pipeline(worldGeneralPipeline);
            pc.mvp = options.mvp;
        } else {
            commandbuffer->set_pipeline(generalPipeline);
        }

        commandbuffer->set_push_constant(&pc, sizeof(UIPushConstant));

        commandbuffer->bind_shader_buffer(screen->elements_buffer, 0, 0, sizeof(ElementInstance) * 50);
        commandbuffer->draw(0, 4, i + continuity.elementOffset, 1);
    }

    UIPushConstant pc;
    pc.screenSize = windowSize;

    if(options.render_world) {
        commandbuffer->set_pipeline(worldTextPipeline);
        pc.mvp = options.mvp;
    } else {
        commandbuffer->set_pipeline(textPipeline);
    }

    commandbuffer->set_push_constant(&pc, sizeof(UIPushConstant));

    commandbuffer->bind_shader_buffer(screen->glyph_buffer, 0, 0, instanceAlignment);
    commandbuffer->bind_shader_buffer(screen->glyph_buffer, instanceAlignment, 1, sizeof(GylphMetric) * numGlyphs);
    commandbuffer->bind_shader_buffer(screen->instance_buffer, 0, 2, sizeof(StringInstance) * 50);
    commandbuffer->bind_texture(fontTexture, 3);

    if(stringLen > 0)
        commandbuffer->draw(0, 4, 0, stringLen);

    continuity.elementOffset += numElements;
}

void Renderer::create_mesh_pipeline(Material& material) {
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
    pipelineInfo.shaders.vertex_path = "mesh.vert";
    pipelineInfo.shaders.fragment_path = "mesh.frag";
    
    pipelineInfo.shaders.vertex_constants = {materials_constant, lights_constant, spot_lights_constant, probes_constant};
    pipelineInfo.shaders.fragment_constants = {materials_constant, lights_constant, spot_lights_constant, probes_constant};
    
    struct PushConstant {
        Matrix4x4 m;
        int s;
    };
    
    pipelineInfo.shader_input.push_constants = {
        {sizeof(PushConstant), 0}
    };
    
    pipelineInfo.shader_input.bindings = {
        {1, GFXBindingType::StorageBuffer},
        {0, GFXBindingType::PushConstant},
        {2, GFXBindingType::Texture},
        {3, GFXBindingType::Texture},
        {4, GFXBindingType::Texture},
        {5, GFXBindingType::Texture},
        {6, GFXBindingType::Texture},
        {7, GFXBindingType::Texture}
    };
    
    pipelineInfo.render_pass = offscreenRenderPass;
    pipelineInfo.depth.depth_mode = GFXDepthMode::Less;
    pipelineInfo.rasterization.culling_mode = GFXCullingMode::Backface;
    //pipelineInfo.blending.enable_blending = true;
    pipelineInfo.blending.src_rgb = GFXBlendFactor::SrcAlpha;
    pipelineInfo.blending.dst_rgb = GFXBlendFactor::OneMinusSrcAlpha;
    
    pipelineInfo.shaders.fragment_src = shader_compiler.compile_material_fragment(material);
    pipelineInfo.shaders.fragment_path = "";
    
    auto [static_pipeline, skinned_pipeline] = shader_compiler.create_pipeline_permutations(pipelineInfo);
    
    material.static_pipeline = static_pipeline;
    material.skinned_pipeline = skinned_pipeline;
    
    pipelineInfo.render_pass = scene_capture->renderPass;
    
    pipelineInfo.shaders.fragment_src = shader_compiler.compile_material_fragment(material, false); // scene capture does not use IBL

    material.capture_pipeline = shader_compiler.create_static_pipeline(pipelineInfo, false, true);
}

void Renderer::createDummyTexture() {
    GFXTextureCreateInfo createInfo = {};
    createInfo.width = 1;
    createInfo.height = 1;
    createInfo.format = GFXPixelFormat::R8G8B8A8_UNORM;
    createInfo.usage = GFXTextureUsage::Sampled;

    dummyTexture = gfx->create_texture(createInfo);

    uint8_t tex[4] = {0, 0, 0, 0};

    gfx->copy_texture(dummyTexture, tex, sizeof(tex));
}

void Renderer::createOffscreenResources() {
	GFXRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.attachments.push_back(GFXPixelFormat::RGBA_32F);
	renderPassInfo.attachments.push_back(GFXPixelFormat::DEPTH_32F);

	offscreenRenderPass = gfx->create_render_pass(renderPassInfo);
    
    renderPassInfo = {};
    renderPassInfo.attachments = {GFXPixelFormat::RGBA8_UNORM};
    
    unormRenderPass = gfx->create_render_pass(renderPassInfo);
    
    const auto extent = get_render_extent();
    
    GFXTextureCreateInfo textureInfo = {};
    textureInfo.width = extent.width;
    textureInfo.height = extent.height;
    textureInfo.format = GFXPixelFormat::RGBA_32F;
    textureInfo.usage = GFXTextureUsage::Attachment | GFXTextureUsage::Sampled;
    textureInfo.samplingMode = SamplingMode::ClampToEdge;

    offscreenColorTexture = gfx->create_texture(textureInfo);
    offscreenBackTexture = gfx->create_texture(textureInfo);

    textureInfo.format = GFXPixelFormat::DEPTH_32F;

    offscreenDepthTexture = gfx->create_texture(textureInfo);

    GFXFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.render_pass = offscreenRenderPass;
    framebufferInfo.attachments.push_back(offscreenColorTexture);
    framebufferInfo.attachments.push_back(offscreenDepthTexture);

    offscreenFramebuffer = gfx->create_framebuffer(framebufferInfo);

    if(viewport_mode) {
        GFXRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.attachments.push_back(GFXPixelFormat::RGBA8_UNORM);

        viewportRenderPass = gfx->create_render_pass(renderPassInfo);

        GFXTextureCreateInfo textureInfo = {};
        textureInfo.width = extent.width;
        textureInfo.height = extent.height;
        textureInfo.format = GFXPixelFormat::RGBA8_UNORM;
        textureInfo.usage = GFXTextureUsage::Attachment | GFXTextureUsage::Sampled;
        textureInfo.samplingMode = SamplingMode::ClampToEdge;

        viewportColorTexture = gfx->create_texture(textureInfo);

        framebufferInfo.render_pass = viewportRenderPass;
        framebufferInfo.attachments = {viewportColorTexture};

        viewportFramebuffer = gfx->create_framebuffer(framebufferInfo);
    }
}

void Renderer::createMeshPipeline() {
    sceneBuffer = gfx->create_buffer(nullptr, sizeof(SceneInformation), true, GFXBufferUsage::Storage);
}

void Renderer::createPostPipeline() {
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "Post";

    pipelineInfo.shaders.vertex_path = "post.vert";
    pipelineInfo.shaders.fragment_path = "post.frag";

    pipelineInfo.shader_input.bindings = {
        {4, GFXBindingType::PushConstant},
        {1, GFXBindingType::Texture},
        {2, GFXBindingType::Texture},
        {3, GFXBindingType::Texture}
    };

    pipelineInfo.shader_input.push_constants = {
        {sizeof(PostPushConstants), 0}
    };

    postPipeline = gfx->create_graphics_pipeline(pipelineInfo);

    pipelineInfo.label = "Render to Texture";
    pipelineInfo.render_pass = offscreenRenderPass;

    renderToTexturePipeline = gfx->create_graphics_pipeline(pipelineInfo);
    
    pipelineInfo.label = "Render to Texture (Unorm)";
    pipelineInfo.render_pass = unormRenderPass;
    
    renderToUnormTexturePipeline = gfx->create_graphics_pipeline(pipelineInfo);
    
    pipelineInfo.label = "Render to Viewport";
    pipelineInfo.render_pass = viewportRenderPass;

    renderToViewportPipeline = gfx->create_graphics_pipeline(pipelineInfo);
}

void Renderer::createFontPipeline() {
    auto file = file::open(file::app_domain / "font.fp", true);
    if(file == std::nullopt) {
        console::error(System::Renderer, "Failed to load font file!");
        return;
    }

    file->read(&font);

    std::vector<unsigned char> bitmap(font.width * font.height);
    file->read(bitmap.data(), font.width * font.height);

    instanceAlignment = (int)gfx->get_alignment(sizeof(GlyphInstance) * maxInstances);

    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "Text";

    pipelineInfo.shaders.vertex_path = "text.vert";
    pipelineInfo.shaders.fragment_path = "text.frag";

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

    textPipeline = gfx->create_graphics_pipeline(pipelineInfo);

    pipelineInfo.render_pass = offscreenRenderPass;
    pipelineInfo.label = "Text World";
    pipelineInfo.depth.depth_mode = GFXDepthMode::LessOrEqual;

    worldTextPipeline = gfx->create_graphics_pipeline(pipelineInfo);

    GFXTextureCreateInfo textureInfo = {};
    textureInfo.width = font.width;
    textureInfo.height = font.height;
    textureInfo.format = GFXPixelFormat::R8_UNORM;
    textureInfo.usage = GFXTextureUsage::Sampled;

    fontTexture = gfx->create_texture(textureInfo);

    gfx->copy_texture(fontTexture, bitmap.data(), font.width * font.height);
}

void Renderer::createSkyPipeline() {
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "Sky";
    pipelineInfo.render_pass = offscreenRenderPass;

    pipelineInfo.shaders.vertex_path = "sky.vert";
    pipelineInfo.shaders.fragment_path = "sky.frag";

    pipelineInfo.shader_input.bindings = {
        {1, GFXBindingType::PushConstant}
    };

    pipelineInfo.shader_input.push_constants = {
        {sizeof(Matrix4x4) + sizeof(Vector4) + sizeof(Vector4), 0}
    };

    pipelineInfo.depth.depth_mode = GFXDepthMode::LessOrEqual;

    skyPipeline = gfx->create_graphics_pipeline(pipelineInfo);
}

void Renderer::createUIPipeline() {
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "UI";

    pipelineInfo.shaders.vertex_path = "ui.vert";
    pipelineInfo.shaders.fragment_path = "ui.frag";

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

    generalPipeline = gfx->create_graphics_pipeline(pipelineInfo);

    pipelineInfo.label = "UI World";
    pipelineInfo.render_pass = offscreenRenderPass;
    pipelineInfo.depth.depth_mode = GFXDepthMode::LessOrEqual;

    worldGeneralPipeline = gfx->create_graphics_pipeline(pipelineInfo);
}

void Renderer::createGaussianResources() {
    const auto extent = get_render_extent();
    
    gHelper = std::make_unique<GaussianHelper>(gfx, extent);

    GFXTextureCreateInfo textureInfo = {};
    textureInfo.width = extent.width;
    textureInfo.height = extent.height;
    textureInfo.format = GFXPixelFormat::RGBA_32F;
    textureInfo.usage = GFXTextureUsage::Sampled;

    blurStore = gfx->create_texture(textureInfo);

    hasToStore = true;
}

void Renderer::createBRDF() {
    GFXRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachments.push_back(GFXPixelFormat::R8G8_SFLOAT);
    
    brdfRenderPass = gfx->create_render_pass(renderPassInfo);
    
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "BRDF";
    
    pipelineInfo.shaders.vertex_path = "brdf.vert";
    pipelineInfo.shaders.fragment_path = "brdf.frag";
    
    pipelineInfo.render_pass = brdfRenderPass;
    
    brdfPipeline = gfx->create_graphics_pipeline(pipelineInfo);
    
    GFXTextureCreateInfo textureInfo = {};
    textureInfo.format = GFXPixelFormat::R8G8_SFLOAT;
    textureInfo.width = brdf_resolution;
    textureInfo.height = brdf_resolution;
    textureInfo.usage = GFXTextureUsage::Sampled | GFXTextureUsage::Attachment;
    
    brdfTexture = gfx->create_texture(textureInfo);
    
    GFXFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.attachments = {brdfTexture};
    framebufferInfo.render_pass = brdfRenderPass;
    
    brdfFramebuffer = gfx->create_framebuffer(framebufferInfo);
    
    // render
    GFXCommandBuffer* command_buffer = gfx->acquire_command_buffer();
    
    GFXRenderPassBeginInfo beginInfo = {};
    beginInfo.render_pass = brdfRenderPass;
    beginInfo.framebuffer = brdfFramebuffer;
    beginInfo.render_area.extent = {brdf_resolution, brdf_resolution};
    
    command_buffer->set_render_pass(beginInfo);
    
    command_buffer->set_pipeline(brdfPipeline);
    
    Viewport viewport = {};
    viewport.width = brdf_resolution;
    viewport.height = brdf_resolution;
    
    command_buffer->set_viewport(viewport);
    
    command_buffer->draw(0, 4, 0, 1);
    
    gfx->submit(command_buffer);
}
