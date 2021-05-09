#include "debugpass.hpp"

#include "gfx_commandbuffer.hpp"
#include "engine.hpp"
#include "scene.hpp"
#include "transform.hpp"
#include "file.hpp"
#include "gfx.hpp"
#include "asset.hpp"
#include "log.hpp"
#include "materialcompiler.hpp"
#include "renderer.hpp"

struct BillPushConstant {
    Matrix4x4 mvp;
    Vector4 color;
};

void DebugPass::initialize() {
    scene_info_buffer = engine->get_gfx()->create_buffer(nullptr, sizeof(Matrix4x4), true, GFXBufferUsage::Storage);

    {
        GFXGraphicsPipelineCreateInfo createInfo;
        createInfo.shaders.vertex_src = ShaderSource(file::Path("debug.vert"));
        createInfo.shaders.fragment_src = ShaderSource(file::Path("debug.frag"));

        GFXVertexInput vertexInput = {};
        vertexInput.stride = sizeof(Vector3);

        createInfo.vertex_input.inputs.push_back(vertexInput);

        GFXVertexAttribute positionAttribute = {};
        positionAttribute.format = GFXVertexFormat::FLOAT3;

        createInfo.vertex_input.attributes.push_back(positionAttribute);

        createInfo.shader_input.push_constants = {
            {sizeof(Matrix4x4) + sizeof(Vector4), 0}
        };

        createInfo.shader_input.bindings = {
            {1, GFXBindingType::PushConstant}
        };

        createInfo.render_pass = engine->get_renderer()->offscreen_render_pass;
        createInfo.rasterization.polygon_type = GFXPolygonType::Line;

        primitive_pipeline = engine->get_gfx()->create_graphics_pipeline(createInfo);

        cubeMesh = assetm->get<Mesh>(file::app_domain / "models/cube.model");
        sphereMesh = assetm->get<Mesh>(file::app_domain / "models/sphere.model");

        createInfo.rasterization.polygon_type = GFXPolygonType::Fill;

        arrow_pipeline = engine->get_gfx()->create_graphics_pipeline(createInfo);
        
        arrowMesh = assetm->get<Mesh>(file::app_domain / "models/arrow.model");
    }

    {
        // render pass
        GFXRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.label = "Select";
        renderPassInfo.attachments.push_back(GFXPixelFormat::R8G8B8A8_UNORM);
        renderPassInfo.attachments.push_back(GFXPixelFormat::DEPTH_32F);

        selectRenderPass = engine->get_gfx()->create_render_pass(renderPassInfo);

        // pipeline
        GFXGraphicsPipelineCreateInfo pipelineInfo = {};

        pipelineInfo.shaders.vertex_src = ShaderSource(file::Path("color.vert"));
        pipelineInfo.shaders.fragment_src = ShaderSource(file::Path("color.frag"));

        GFXVertexInput input;
        input.stride = sizeof(Vector3);

        pipelineInfo.vertex_input.inputs.push_back(input);

        GFXVertexAttribute attribute;
        attribute.format = GFXVertexFormat::FLOAT3;

        pipelineInfo.vertex_input.attributes.push_back(attribute);

        pipelineInfo.shader_input.bindings = {
            {1, GFXBindingType::PushConstant}
        };

        pipelineInfo.shader_input.push_constants = {
            {sizeof(Matrix4x4) + sizeof(Vector4), 0}
        };

        pipelineInfo.render_pass = selectRenderPass;
        pipelineInfo.depth.depth_mode = GFXDepthMode::Less;

        selectPipeline = engine->get_gfx()->create_graphics_pipeline(pipelineInfo);
    }
    
    // sobel
    {
        // render pass
        GFXRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.label = "Sobel";
        renderPassInfo.attachments.push_back(GFXPixelFormat::R8_UNORM);
        renderPassInfo.will_use_in_shader = true;
        
        sobelRenderPass = engine->get_gfx()->create_render_pass(renderPassInfo);

        // pipeline
        GFXGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.label = "Sobel";

        pipelineInfo.shaders.vertex_src = ShaderSource(file::Path("color.vert"));
        pipelineInfo.shaders.fragment_src = ShaderSource(file::Path("color.frag"));

        GFXVertexInput input;
        input.stride = sizeof(Vector3);

        pipelineInfo.vertex_input.inputs.push_back(input);

        GFXVertexAttribute attribute;
        attribute.format = GFXVertexFormat::FLOAT3;

        pipelineInfo.vertex_input.attributes.push_back(attribute);

        pipelineInfo.shader_input.bindings = {
            {1, GFXBindingType::PushConstant}
        };

        pipelineInfo.shader_input.push_constants = {
            {sizeof(Matrix4x4) + sizeof(Vector4), 0}
        };

        pipelineInfo.render_pass = sobelRenderPass;

        sobelPipeline = engine->get_gfx()->create_graphics_pipeline(pipelineInfo);
    }
    
    // billboard
    {
        // pipeline
        GFXGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.label = "Billboard";
        
        pipelineInfo.shaders.vertex_src = ShaderSource(file::Path("billboard.vert"));
        pipelineInfo.shaders.fragment_src = ShaderSource(file::Path("billboard.frag"));
        
        pipelineInfo.shader_input.bindings = {
            {1, GFXBindingType::PushConstant},
            {2, GFXBindingType::Texture},
            {3, GFXBindingType::StorageBuffer}
        };
        
        pipelineInfo.shader_input.push_constants = {
            {sizeof(BillPushConstant), 0}
        };
        
        pipelineInfo.depth.depth_mode = GFXDepthMode::Less;
        
        pipelineInfo.blending.enable_blending = true;
        
        pipelineInfo.render_pass = engine->get_renderer()->offscreen_render_pass;
        
        billboard_pipeline = engine->get_gfx()->create_graphics_pipeline(pipelineInfo);
        
        pointTexture = assetm->get<Texture>(file::app_domain / "textures/point.png");
        spotTexture = assetm->get<Texture>(file::app_domain / "textures/spot.png");
        sunTexture = assetm->get<Texture>(file::app_domain / "textures/sun.png");
        probeTexture = assetm->get<Texture>(file::app_domain / "textures/probe.png");
    }
}

void DebugPass::create_render_target_resources(RenderTarget& target) {
    this->extent = target.extent;
    
    createOffscreenResources();
}

void DebugPass::createOffscreenResources() {
    // selection resources
    {
        GFXTextureCreateInfo textureInfo = {};
        textureInfo.label = "Select Color";
        textureInfo.width = extent.width;
        textureInfo.height = extent.height;
        textureInfo.format = GFXPixelFormat::R8G8B8A8_UNORM;
        textureInfo.usage = GFXTextureUsage::Attachment;
        textureInfo.samplingMode = SamplingMode::ClampToEdge;
        
        selectTexture = engine->get_gfx()->create_texture(textureInfo);
        
        textureInfo.label = "Select Depth";
        textureInfo.format = GFXPixelFormat::DEPTH_32F;
        
        selectDepthTexture = engine->get_gfx()->create_texture(textureInfo);
        
        GFXFramebufferCreateInfo info;
        info.attachments = {selectTexture, selectDepthTexture};
        info.render_pass = selectRenderPass;

        selectFramebuffer = engine->get_gfx()->create_framebuffer(info);

        selectBuffer = engine->get_gfx()->create_buffer(nullptr, extent.width * extent.height * 4 * sizeof(uint8_t), false, GFXBufferUsage::Storage);
    }
        
    // sobel
    {
        GFXTextureCreateInfo textureInfo = {};
        textureInfo.label = "Sobel";
        textureInfo.width = extent.width;
        textureInfo.height = extent.height;
        textureInfo.format = GFXPixelFormat::R8_UNORM;
        textureInfo.usage = GFXTextureUsage::Attachment;
        textureInfo.samplingMode = SamplingMode::ClampToEdge;
        
        sobelTexture = engine->get_gfx()->create_texture(textureInfo);

        GFXFramebufferCreateInfo info;
        info.attachments = {sobelTexture};
        info.render_pass = sobelRenderPass;

        sobelFramebuffer = engine->get_gfx()->create_framebuffer(info);
    }
}

void DebugPass::draw_arrow(GFXCommandBuffer* commandBuffer, Vector3 color, Matrix4x4 model) {
    struct PushConstant {
        Matrix4x4 mvp;
        Vector4 color;
    } pc;
    pc.mvp = model;
    pc.color = color;
    
    commandBuffer->set_push_constant(&pc, sizeof(PushConstant));
    
    commandBuffer->set_vertex_buffer(arrowMesh->position_buffer, 0, 0);
    commandBuffer->set_index_buffer(arrowMesh->index_buffer, IndexType::UINT32);
    
    commandBuffer->draw_indexed(arrowMesh->num_indices, 0, 0, 0);
}

void DebugPass::render_scene(Scene& scene, GFXCommandBuffer* commandBuffer) {
    auto [camObj, camera] = scene.get_all<Camera>()[0];
    
    struct PushConstant {
        Matrix4x4 mvp;
        Vector4 color;
    };
    
    Matrix4x4 vp = camera.perspective * camera.view;

	commandBuffer->set_graphics_pipeline(primitive_pipeline);

    struct DebugPrimitive {
        Vector3 position, size;
        Quaternion rotation;
        Vector4 color;
    };

    std::vector<DebugPrimitive> primitives;
    
    struct DebugBillboard {
        Vector3 position;
        GFXTexture* texture;
        Vector4 color;
    };
    
    std::vector<DebugBillboard> billboards;

    for(auto& obj : scene.get_objects()) {
        if(scene.get(obj).editor_object)
            continue;

        auto& transform = scene.get<Transform>(obj);

        if(scene.has<Collision>(obj)) {
            auto& collision = scene.get<Collision>(obj);

            {
                DebugPrimitive prim;
                prim.position = transform.get_world_position();
                prim.rotation = transform.rotation;
                prim.size = collision.size / 2.0f;
                prim.color = collision.is_trigger ? Vector4(0, 0, 1, 1) : Vector4(0, 1, 0, 1);

                primitives.push_back(prim);
            }
        }
        
        if(scene.has<Light>(obj)) {
            DebugBillboard bill;
            bill.position = transform.get_world_position();
            bill.color = Vector4(scene.get<Light>(obj).color, 1.0f);
            
            switch(scene.get<Light>(obj).type) {
                case Light::Type::Point:
                    bill.texture = pointTexture->handle;
                    break;
                case Light::Type::Spot:
                    bill.texture = spotTexture->handle;
                    break;
                case Light::Type::Sun:
                    bill.texture = sunTexture->handle;
                    break;
            }
            
            billboards.push_back(bill);
        }
        
        if(scene.has<EnvironmentProbe>(obj)) {
            if(selected_object == obj) {
                auto& probe = scene.get<EnvironmentProbe>(obj);
                
                DebugPrimitive prim;
                prim.position = transform.get_world_position();
                prim.rotation = transform.rotation;
                prim.size = probe.size / 2.0f;
                prim.color = Vector4(0, 1, 1, 1);
                
                primitives.push_back(prim);
            }
            
            DebugBillboard bill;
            bill.position = transform.get_world_position();
            bill.color = Vector4(1.0f);
            bill.texture = probeTexture->handle;
            
            billboards.push_back(bill);
        }
    }

    // draw primitives
    for(auto& prim : primitives) {
        PushConstant pc;

        Matrix4x4 m = transform::translate(Matrix4x4(), prim.position);
        m *= matrix_from_quat(prim.rotation);
        m = transform::scale(m, prim.size);

        pc.mvp = vp * m;
        pc.color = prim.color;

        commandBuffer->set_push_constant(&pc, sizeof(PushConstant));

        commandBuffer->set_vertex_buffer(cubeMesh->position_buffer, 0, 0);
        commandBuffer->set_index_buffer(cubeMesh->index_buffer, IndexType::UINT32);

        commandBuffer->draw_indexed(cubeMesh->num_indices, 0, 0, 0);
    }
    
    commandBuffer->set_graphics_pipeline(billboard_pipeline);

    engine->get_gfx()->copy_buffer(scene_info_buffer, &camera.view, 0, sizeof(Matrix4x4));

    // draw primitives
    for(auto& bill : billboards) {
        Matrix4x4 m = transform::translate(Matrix4x4(), bill.position);
        
        BillPushConstant pc;
        pc.mvp = vp * m;
        pc.color = bill.color;
                
        commandBuffer->bind_texture(bill.texture, 2);
        commandBuffer->bind_shader_buffer(scene_info_buffer, 0, 3, sizeof(Matrix4x4));

        commandBuffer->set_push_constant(&pc, sizeof(BillPushConstant));
        commandBuffer->draw_indexed(4, 0, 0, 0);
    }
    
    commandBuffer->set_graphics_pipeline(arrow_pipeline);
    
    // draw handles for selected object;
    if(selected_object != NullObject && engine->get_scene()->has<Transform>(selected_object)) {
        const auto position = engine->get_scene()->get<Transform>(selected_object).get_world_position();
        
        const float base_scale = 0.05f;
        const float scale_factor = length(position - scene.get<Transform>(camObj).get_world_position());
        
        Matrix4x4 base_model = transform::translate(Matrix4x4(), position);
        base_model = transform::scale(base_model, base_scale * scale_factor);
                                                    
        // draw y axis
        draw_arrow(commandBuffer, Vector3(0, 1, 0), vp * base_model);
        
        // draw x axis
        draw_arrow(commandBuffer, Vector3(1, 0, 0), vp * base_model * matrix_from_quat(angle_axis(radians(-90.0f), Vector3(0, 0, 1))));
        
        // draw z axis
        draw_arrow(commandBuffer, Vector3(0, 0, 1), vp * base_model * matrix_from_quat(angle_axis(radians(90.0f), Vector3(1, 0, 0))));
    }
    
    // draw sobel
    GFXRenderPassBeginInfo info = {};
    info.framebuffer = sobelFramebuffer;
    info.render_pass = sobelRenderPass;
    info.render_area.extent = extent;

    commandBuffer->set_render_pass(info);
    
    if(selected_object != NullObject && engine->get_scene()->has<Renderable>(selected_object)) {
        commandBuffer->set_graphics_pipeline(sobelPipeline);

        auto renderable = engine->get_scene()->get<Renderable>(selected_object);
        
        if(!renderable.mesh)
            return;
        
        struct PC {
            Matrix4x4 mvp;
            Vector4 color;
        } pc;
        
        pc.mvp = vp * engine->get_scene()->get<Transform>(selected_object).model;
        pc.color = Vector4(1);
        
        commandBuffer->set_push_constant(&pc, sizeof(PC));
        
        commandBuffer->set_vertex_buffer(renderable.mesh->position_buffer, 0, 0);
        commandBuffer->set_index_buffer(renderable.mesh->index_buffer, IndexType::UINT32);
        
        if(renderable.mesh) {
            for (auto& part : renderable.mesh->parts)
                commandBuffer->draw_indexed(part.index_count, part.index_offset, part.vertex_offset, 0);
        }
    }
}

void DebugPass::get_selected_object(int x, int y, std::function<void(SelectableObject)> callback) {
    if(engine->get_scene() == nullptr)
        return;
    
    auto cameras = engine->get_scene()->get_all<Camera>();

    auto& [camObj, camera] = cameras[0];
    
    // calculate selectable objects
    selectable_objects.clear();
    
    for(auto& [obj, mesh] : engine->get_scene()->get_all<Renderable>()) {
        SelectableObject so;
        so.type = SelectableObject::Type::Object;
        so.object = obj;
        so.render_type = SelectableObject::RenderType::Mesh;
        
        selectable_objects.push_back(so);
    }
    
    for(auto& [obj, mesh] : engine->get_scene()->get_all<Light>()) {
        SelectableObject so;
        so.type = SelectableObject::Type::Object;
        so.object = obj;
        so.render_type = SelectableObject::RenderType::Sphere;
        so.sphere_size = 0.5f;
        
        selectable_objects.push_back(so);
    }
    
    for(auto& [obj, mesh] : engine->get_scene()->get_all<EnvironmentProbe>()) {
        SelectableObject so;
        so.type = SelectableObject::Type::Object;
        so.object = obj;
        so.render_type = SelectableObject::RenderType::Sphere;
        so.sphere_size = 0.5f;
        
        selectable_objects.push_back(so);
    }
    
    // add selections for currently selected object handles
    const auto add_arrow = [this](SelectableObject::Axis axis, Matrix4x4 model) {
        SelectableObject so;
        so.type = SelectableObject::Type::Handle;
        so.axis = axis;
        so.axis_model = model;
        so.object = selected_object;
        
        selectable_objects.push_back(so);
    };
    
    if(selected_object != NullObject) {
        const auto position = engine->get_scene()->get<Transform>(selected_object).get_world_position();
        
        const float base_scale = 0.05f;
        const float scale_factor = length(position - engine->get_scene()->get<Transform>(camObj).position);
        
        const Matrix4x4 translate_model = transform::translate(Matrix4x4(), position);
        const Matrix4x4 scale_model = transform::scale(Matrix4x4(), base_scale * scale_factor);
        
        add_arrow(SelectableObject::Axis::Y, translate_model * scale_model);
        
        add_arrow(SelectableObject::Axis::X, translate_model * matrix_from_quat(angle_axis(radians(-90.0f), Vector3(0, 0, 1))) * scale_model);
        
        add_arrow(SelectableObject::Axis::Z, translate_model * matrix_from_quat(angle_axis(radians(90.0f), Vector3(1, 0, 0))) * scale_model);
    }
    
    GFXCommandBuffer* commandBuffer = engine->get_gfx()->acquire_command_buffer();

    GFXRenderPassBeginInfo info = {};
    info.clear_color.a = 0.0;
    info.framebuffer = selectFramebuffer;
    info.render_pass = selectRenderPass;
    info.render_area.extent = extent;

    commandBuffer->set_render_pass(info);
    
    Viewport viewport = {};
    viewport.width = extent.width;
    viewport.height = extent.height;
    
    commandBuffer->set_viewport(viewport);

    commandBuffer->set_graphics_pipeline(selectPipeline);

    for(auto [i, object] : utility::enumerate(selectable_objects)) {
        AssetPtr<Mesh> mesh;
        Matrix4x4 model;
        if(object.type == SelectableObject::Type::Object) {
            if(object.render_type == SelectableObject::RenderType::Mesh) {
                auto& renderable = engine->get_scene()->get<Renderable>(object.object);
                
                if(!renderable.mesh)
                    continue;
                
                mesh = renderable.mesh;
            } else {
                mesh = sphereMesh;
            }
            
            model = engine->get_scene()->get<Transform>(object.object).model;
        } else {
            mesh = arrowMesh;
            model = object.axis_model;
        }

        commandBuffer->set_vertex_buffer(mesh->position_buffer, 0, 0);
        commandBuffer->set_index_buffer(mesh->index_buffer, IndexType::UINT32);

        struct PC {
            Matrix4x4 mvp;
            Vector4 color;
        } pc;

        pc.mvp = camera.perspective * camera.view * model;
        
        if(object.render_type == SelectableObject::RenderType::Sphere)
            pc.mvp = transform::scale(pc.mvp, Vector3(object.sphere_size));

        pc.color = {
            float((i & 0x000000FF) >> 0) / 255.0f,
            float((i & 0x0000FF00) >> 8) / 255.0f,
            float((i & 0x00FF0000) >> 16) / 255.0f,
            1.0,
        };

        commandBuffer->set_push_constant(&pc, sizeof(PC));

        for (auto& part : mesh->parts)
            commandBuffer->draw_indexed(part.index_count, part.index_offset, part.vertex_offset, 0);
    }

    engine->get_gfx()->submit(commandBuffer);
    
    engine->get_gfx()->copy_texture(selectTexture, selectBuffer);

    uint8_t* mapped_texture = reinterpret_cast<uint8_t*>(engine->get_gfx()->get_buffer_contents(selectBuffer));
    
    const int buffer_position = 4 * (y * extent.width + x);

    uint8_t a = mapped_texture[buffer_position + 3];

    const int id = mapped_texture[buffer_position] +
        mapped_texture[buffer_position + 1] * 256 +
        mapped_texture[buffer_position + 2] * 256 * 256;
    
    engine->get_gfx()->release_buffer_contents(selectBuffer, mapped_texture);

    if(a != 0) {
        callback(selectable_objects[id]);
    } else {
        SelectableObject o;
        o.object = NullObject;
        callback(o);
    }
}
