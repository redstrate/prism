#include "scenecapture.hpp"

#include "gfx_commandbuffer.hpp"
#include "scene.hpp"
#include "gfx.hpp"
#include "log.hpp"
#include "engine.hpp"
#include "renderer.hpp"
#include "shadowpass.hpp"
#include "materialcompiler.hpp"
#include "frustum.hpp"

struct PushConstant {
    Matrix4x4 m, v;
    int s;
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

const int mipLevels = 5;

const std::array<Matrix4x4, 6> sceneTransforms = {
    transform::look_at(Vector3(0), Vector3(-1.0, 0.0, 0.0), Vector3(0.0, -1.0, 0.0)), // right
    transform::look_at(Vector3(0), Vector3(1.0, 0.0, 0.0), Vector3(0.0, -1.0, 0.0)), // left
    transform::look_at(Vector3(0), Vector3( 0.0, -1.0, 0.0), Vector3(0.0, 0.0, -1.0)), // top
    transform::look_at(Vector3(0), Vector3( 0.0, 1.0, 0.0), Vector3(0.0, 0.0, 1.0)), // bottom
    transform::look_at(Vector3(0), Vector3( 0.0, 0.0, 1.0), Vector3(0.0, -1.0, 0.0)), // back
    transform::look_at(Vector3(0), Vector3( 0.0, 0.0, -1.0), Vector3(0.0, -1.0, 0.0)) // front
};

inline AssetPtr<Mesh> cubeMesh;

SceneCapture::SceneCapture(GFX* gfx) {
    cubeMesh = assetm->get<Mesh>(file::app_domain / "models/cube.model");
    
    // render pass
    GFXRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachments.push_back(GFXPixelFormat::R8G8B8A8_UNORM);
    renderPassInfo.attachments.push_back(GFXPixelFormat::DEPTH_32F);

    renderPass = gfx->create_render_pass(renderPassInfo);
    
    GFXTextureCreateInfo textureInfo = {};
    textureInfo.width = scene_cubemap_resolution;
    textureInfo.height = scene_cubemap_resolution;
    textureInfo.format = GFXPixelFormat::R8G8B8A8_UNORM;
    textureInfo.usage = GFXTextureUsage::Attachment;
    textureInfo.samplingMode = SamplingMode::ClampToEdge;
    
    offscreenTexture = gfx->create_texture(textureInfo);
    
    GFXTextureCreateInfo depthTextureInfo = {};
    depthTextureInfo.width = scene_cubemap_resolution;
    depthTextureInfo.height = scene_cubemap_resolution;
    depthTextureInfo.format = GFXPixelFormat::DEPTH_32F;
    depthTextureInfo.usage = GFXTextureUsage::Attachment;
    depthTextureInfo.samplingMode = SamplingMode::ClampToEdge;
    
    offscreenDepth = gfx->create_texture(depthTextureInfo);
    
    GFXFramebufferCreateInfo info;
    info.attachments = {offscreenTexture, offscreenDepth};
    info.render_pass = renderPass;
    
    offscreenFramebuffer = gfx->create_framebuffer(info);
    
    GFXTextureCreateInfo cubeTextureInfo = {};
    cubeTextureInfo.type = GFXTextureType::Cubemap;
    cubeTextureInfo.width = scene_cubemap_resolution;
    cubeTextureInfo.height = scene_cubemap_resolution;
    cubeTextureInfo.format = GFXPixelFormat::R8G8B8A8_UNORM;
    cubeTextureInfo.usage = GFXTextureUsage::Sampled;
    cubeTextureInfo.samplingMode = SamplingMode::ClampToEdge;
    cubeTextureInfo.mip_count = mipLevels;
    
    environmentCube = gfx->create_texture(cubeTextureInfo);
    
    sceneBuffer = gfx->create_buffer(nullptr, sizeof(SceneInformation), true, GFXBufferUsage::Storage);
    
    createSkyResources();
    createIrradianceResources();
    createPrefilterResources();
}

void SceneCapture::create_scene_resources(Scene& scene) {
    auto gfx = engine->get_gfx();
    
    if(gfx->supports_feature(GFXFeature::CubemapArray)) {
        GFXTextureCreateInfo cubeTextureInfo = {};
        cubeTextureInfo.type = GFXTextureType::CubemapArray;
        cubeTextureInfo.width = irradiance_cubemap_resolution;
        cubeTextureInfo.height = irradiance_cubemap_resolution;
        cubeTextureInfo.format = GFXPixelFormat::R8G8B8A8_UNORM;
        cubeTextureInfo.usage = GFXTextureUsage::Sampled;
        cubeTextureInfo.samplingMode = SamplingMode::ClampToEdge;
        cubeTextureInfo.array_length = max_environment_probes;
        
        scene.irradianceCubeArray = gfx->create_texture(cubeTextureInfo);
        
        cubeTextureInfo = {};
        cubeTextureInfo.type = GFXTextureType::CubemapArray;
        cubeTextureInfo.width = scene_cubemap_resolution;
        cubeTextureInfo.height = scene_cubemap_resolution;
        cubeTextureInfo.format = GFXPixelFormat::R8G8B8A8_UNORM;
        cubeTextureInfo.usage = GFXTextureUsage::Sampled;
        cubeTextureInfo.samplingMode = SamplingMode::ClampToEdge;
        cubeTextureInfo.array_length = max_environment_probes;
        cubeTextureInfo.mip_count = mipLevels;
        
        scene.prefilteredCubeArray = gfx->create_texture(cubeTextureInfo);
    }
}

void SceneCapture::render(GFXCommandBuffer* command_buffer, Scene* scene) {
    int last_probe = 0;
    auto probes = scene->get_all<EnvironmentProbe>();
    for(auto [obj, probe] : probes) {
        if(last_probe > max_environment_probes)
            return;
                
        if(scene->environment_dirty[last_probe]) {            
            std::map<Material*, int> material_indices;
            int numMaterialsInBuffer = 0;
            
            const Vector3 lightPos = scene->get<Transform>(obj).get_world_position();
            
            const Matrix4x4 projection = transform::infinite_perspective(radians(90.0f), 1.0f, 0.1f);
            const Matrix4x4 model = transform::translate(Matrix4x4(), Vector3(-lightPos.x, -lightPos.y, -lightPos.z));
            
            SceneInformation sceneInfo = {};
            sceneInfo.lightspace = scene->lightSpace;
            sceneInfo.numLights = 0;
            sceneInfo.camPos = lightPos;
            sceneInfo.vp = projection;
            
            for(const auto [obj, light] : scene->get_all<Light>()) {
                SceneLight sl;
                sl.positionType = Vector4(scene->get<Transform>(obj).get_world_position(), (int)light.type);
                
                Vector3 front = Vector3(0.0f, 0.0f, 1.0f) * scene->get<Transform>(obj).rotation;
                
                sl.directionPower = Vector4(-front, light.power);
                sl.colorSize = Vector4(utility::from_srgb_to_linear(light.color), radians(light.spot_size));
                sl.shadowsEnable = Vector4(light.enable_shadows, radians(light.size), 0, 0);
                
                sceneInfo.lights[sceneInfo.numLights++] = sl;
            }
            
            for(int i = 0; i < max_spot_shadows; i++)
                sceneInfo.spotLightSpaces[i] = scene->spotLightSpaces[i];
            
            const auto& meshes = scene->get_all<Renderable>();
            
            for(const auto [obj, mesh] : meshes) {
                if(!mesh.mesh)
                    continue;
                
                if(mesh.materials.empty())
                    continue;
                
                for(const auto& material : mesh.materials) {
                    if(!material)
                        continue;
                    
                    if(material->static_pipeline == nullptr || material->skinned_pipeline == nullptr)
                        engine->get_renderer()->create_mesh_pipeline(*material.handle);

                    if(!material_indices.count(material.handle)) {
                        material_indices[material.handle] = numMaterialsInBuffer++;
                    }
                }
            }
            
            const auto render_face = [this, command_buffer, scene, &meshes, &model, &probe = probe](int face) {
                const auto frustum = normalize_frustum(extract_frustum(sceneTransforms[face]));
                
                GFXRenderPassBeginInfo info = {};
                info.framebuffer = offscreenFramebuffer;
                info.render_pass = renderPass;
                info.render_area.extent = {scene_cubemap_resolution, scene_cubemap_resolution};
                
                command_buffer->set_render_pass(info);
                
                Viewport viewport = {};
                viewport.width = scene_cubemap_resolution;
                viewport.height = scene_cubemap_resolution;
                
                command_buffer->set_viewport(viewport);
                
                if(probe.is_sized) {
                    for(const auto [obj, mesh] : meshes) {
                        if(!mesh.mesh)
                            continue;
                        
                        if(mesh.materials.empty())
                            continue;
                        
                        PushConstant pc;
                        pc.m = scene->get<Transform>(obj).model;
                        pc.v = sceneTransforms[face] * model;
                        
                        command_buffer->set_vertex_buffer(mesh.mesh->position_buffer, 0, position_buffer_index);
                        command_buffer->set_vertex_buffer(mesh.mesh->normal_buffer, 0, normal_buffer_index);
                        command_buffer->set_vertex_buffer(mesh.mesh->texture_coord_buffer, 0, texcoord_buffer_index);
                        command_buffer->set_vertex_buffer(mesh.mesh->tangent_buffer, 0, tangent_buffer_index);
                        command_buffer->set_vertex_buffer(mesh.mesh->bitangent_buffer, 0, bitangent_buffer_index);
                        
                        command_buffer->set_index_buffer(mesh.mesh->index_buffer, IndexType::UINT32);
                        
                        command_buffer->bind_shader_buffer(sceneBuffer, 0, 1, sizeof(SceneInformation));
                        command_buffer->bind_texture(scene->depthTexture, 2);
                        command_buffer->bind_texture(scene->pointLightArray, 3);
                        command_buffer->bind_sampler(engine->get_renderer()->shadow_pass->shadow_sampler, 4);
                        command_buffer->bind_sampler(engine->get_renderer()->shadow_pass->pcf_sampler, 5);
                        command_buffer->bind_texture(scene->spotLightArray, 6);

                        if(mesh.mesh->bones.empty()) {
                            for (auto& part : mesh.mesh->parts) {
                                const int material_index = part.material_override == -1 ? 0 : part.material_override;
                                
                                if(material_index >= mesh.materials.size())
                                    continue;
                                
                                if(mesh.materials[material_index].handle == nullptr || mesh.materials[material_index]->static_pipeline == nullptr)
                                    continue;
                                
                                if(render_options.enable_frustum_culling && !test_aabb_frustum(frustum, get_aabb_for_part(scene->get<Transform>(obj), part)))
                                    continue;
                                
                                command_buffer->set_pipeline( mesh.materials[material_index]->capture_pipeline);
             
                                command_buffer->set_push_constant(&pc, sizeof(PushConstant));
                                
                                for(auto& [index, texture] : mesh.materials[material_index]->bound_textures) {
                                    GFXTexture* texture_to_bind = engine->get_renderer()->dummyTexture;
                                    if(texture)
                                        texture_to_bind = texture->handle;
                                    
                                    command_buffer->bind_texture(texture_to_bind, index);
                                }
                                
                                command_buffer->draw_indexed(part.index_count, part.index_offset, part.vertex_offset);
                            }
                        }
                    }
                }
                
                // render sky
                struct SkyPushConstant {
                    Matrix4x4 view;
                    Vector4 sun_position_fov;
                    float aspect;
                } pc;
                
                pc.view = sceneTransforms[face];
                pc.aspect = 1.0f;
                
                for(auto& [obj, light] : scene->get_all<Light>()) {
                    if(light.type == Light::Type::Sun)
                        pc.sun_position_fov = Vector4(scene->get<Transform>(obj).get_world_position(), radians(90.0f));
                }
                
                command_buffer->set_pipeline(skyPipeline);
                
                command_buffer->set_push_constant(&pc, sizeof(SkyPushConstant));
                
                command_buffer->draw(0, 4, 0, 1);
                
                command_buffer->copy_texture(offscreenTexture, scene_cubemap_resolution, scene_cubemap_resolution, environmentCube, face, 0, 0);
            };
            
            engine->get_gfx()->copy_buffer(sceneBuffer, &sceneInfo, 0, sizeof(SceneInformation));

            for (int face = 0; face < 6; face++)
                render_face(face);
            
            command_buffer->generate_mipmaps(environmentCube, mipLevels);

            // calculate irradiance
            const auto convulute_face = [this, scene, command_buffer, &last_probe, projection](int face) {
                GFXRenderPassBeginInfo info = {};
                info.framebuffer = irradianceFramebuffer;
                info.render_pass = irradianceRenderPass;
                info.render_area.extent = {irradiance_cubemap_resolution, irradiance_cubemap_resolution};
                
                command_buffer->set_render_pass(info);
                
                Viewport viewport = {};
                viewport.width = irradiance_cubemap_resolution;
                viewport.height = irradiance_cubemap_resolution;
                
                command_buffer->set_viewport(viewport);
                
                Matrix4x4 mvp = projection * sceneTransforms[face];
                
                command_buffer->set_vertex_buffer(cubeMesh->position_buffer, 0, 0);
                command_buffer->set_index_buffer(cubeMesh->index_buffer, IndexType::UINT32);
                
                command_buffer->set_pipeline(irradiancePipeline);
                command_buffer->bind_texture(environmentCube, 2);
                command_buffer->set_push_constant(&mvp, sizeof(Matrix4x4));
                
                command_buffer->draw_indexed(cubeMesh->num_indices, 0, 0);

                command_buffer->copy_texture(irradianceOffscreenTexture, irradiance_cubemap_resolution, irradiance_cubemap_resolution, scene->irradianceCubeArray, face, last_probe, 0);
            };
            
            for (int face = 0; face < 6; face++)
                convulute_face(face);
                        
            // prefilter
            const auto prefilter_face = [this, scene, command_buffer, &last_probe, projection](int face, int mip) {
                GFXRenderPassBeginInfo info = {};
                info.framebuffer = prefilteredFramebuffer;
                info.render_pass = irradianceRenderPass;
                
                const uint32_t resolution = scene_cubemap_resolution * std::pow(0.5, mip);
                info.render_area.extent = {resolution, resolution};
            
                command_buffer->set_render_pass(info);
                
                Viewport viewport = {};
                viewport.width = resolution;
                viewport.height = resolution;
                
                command_buffer->set_viewport(viewport);
                
                command_buffer->set_vertex_buffer(cubeMesh->position_buffer, 0, 0);
                command_buffer->set_index_buffer(cubeMesh->index_buffer, IndexType::UINT32);
                
                struct PushConstant {
                    Matrix4x4 mvp;
                    float roughness;
                } pc;
                
                pc.mvp = projection * sceneTransforms[face];
                pc.roughness = ((float)mip) / (float)(mipLevels - 1);

                command_buffer->set_pipeline(prefilterPipeline);
                command_buffer->bind_texture(environmentCube, 2);
                command_buffer->set_push_constant(&pc, sizeof(PushConstant));
                
                command_buffer->draw_indexed(cubeMesh->num_indices, 0, 0);
                
                command_buffer->copy_texture(prefilteredOffscreenTexture, info.render_area.extent.width, info.render_area.extent.height, scene->prefilteredCubeArray, face, last_probe, mip);
            };
            
            for(int mip = 0; mip < mipLevels; mip++) {
                for(int i = 0; i < 6; i++)
                    prefilter_face(i, mip);
            }
            
            scene->environment_dirty[last_probe] = false;
        }
        
        last_probe++;
    }
}

void SceneCapture::createSkyResources() {
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "Sky";
    pipelineInfo.render_pass = renderPass;
    
    pipelineInfo.shaders.vertex_path = "sky.vert";
    pipelineInfo.shaders.fragment_path = "sky.frag";
    
    pipelineInfo.shader_input.bindings = {
        {1, GFXBindingType::PushConstant}
    };
    
    pipelineInfo.shader_input.push_constants = {
        {sizeof(Matrix4x4) + sizeof(Vector4) + sizeof(float), 0}
    };
    
    pipelineInfo.depth.depth_mode = GFXDepthMode::LessOrEqual;
    
    skyPipeline = engine->get_gfx()->create_graphics_pipeline(pipelineInfo);
}

void SceneCapture::createIrradianceResources() {
    GFX* gfx = engine->get_gfx();
    
    GFXTextureCreateInfo textureInfo = {};
    textureInfo.width = irradiance_cubemap_resolution;
    textureInfo.height = irradiance_cubemap_resolution;
    textureInfo.format = GFXPixelFormat::R8G8B8A8_UNORM;
    textureInfo.usage = GFXTextureUsage::Attachment;
    textureInfo.samplingMode = SamplingMode::ClampToEdge;
    
    irradianceOffscreenTexture = gfx->create_texture(textureInfo);
    
    GFXFramebufferCreateInfo info;
    info.attachments = {irradianceOffscreenTexture};
    info.render_pass = renderPass;
    
    irradianceFramebuffer = gfx->create_framebuffer(info);

    GFXRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachments.push_back(GFXPixelFormat::R8G8B8A8_UNORM);
    
    irradianceRenderPass = gfx->create_render_pass(renderPassInfo);
    
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "Irradiance Convolution";
    pipelineInfo.shaders.vertex_path = "irradiance.vert";
    pipelineInfo.shaders.fragment_path = "irradiance.frag";
    
    GFXVertexInput input;
    input.stride = sizeof(Vector3);
    
    pipelineInfo.vertex_input.inputs.push_back(input);
    
    GFXVertexAttribute positionAttribute;
    positionAttribute.format = GFXVertexFormat::FLOAT3;
    
    pipelineInfo.vertex_input.attributes.push_back(positionAttribute);
    
    pipelineInfo.shader_input.push_constants = {
        {sizeof(Matrix4x4), 0}
    };
    
    pipelineInfo.shader_input.bindings = {
        {1, GFXBindingType::PushConstant},
        {2, GFXBindingType::Texture}
    };
    
    pipelineInfo.render_pass = irradianceRenderPass;
    
    irradiancePipeline = gfx->create_graphics_pipeline(pipelineInfo);
}

void SceneCapture::createPrefilterResources() {
    GFX* gfx = engine->get_gfx();
    
    GFXTextureCreateInfo textureInfo = {};
    textureInfo.width = scene_cubemap_resolution;
    textureInfo.height = scene_cubemap_resolution;
    textureInfo.format = GFXPixelFormat::R8G8B8A8_UNORM;
    textureInfo.usage = GFXTextureUsage::Attachment;
    textureInfo.samplingMode = SamplingMode::ClampToEdge;
    
    prefilteredOffscreenTexture = gfx->create_texture(textureInfo);
    
    GFXFramebufferCreateInfo info;
    info.attachments = {prefilteredOffscreenTexture};
    info.render_pass = irradianceRenderPass;
    
    prefilteredFramebuffer = gfx->create_framebuffer(info);
    
    GFXShaderConstant size_constant = {};
    size_constant.type = GFXShaderConstant::Type::Integer;
    size_constant.value = scene_cubemap_resolution;
    
    GFXGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.label = "Prefilter";
    pipelineInfo.shaders.vertex_path = "filter.vert";
    pipelineInfo.shaders.fragment_path = "filter.frag";
    
    pipelineInfo.shaders.fragment_constants = {size_constant};
    
    GFXVertexInput input;
    input.stride = sizeof(Vector3);
    
    pipelineInfo.vertex_input.inputs.push_back(input);
    
    GFXVertexAttribute positionAttribute;
    positionAttribute.format = GFXVertexFormat::FLOAT3;
    
    pipelineInfo.vertex_input.attributes.push_back(positionAttribute);
    
    pipelineInfo.shader_input.push_constants = {
        {sizeof(Matrix4x4) + sizeof(float), 0}
    };
    
    pipelineInfo.shader_input.bindings = {
        {1, GFXBindingType::PushConstant},
        {2, GFXBindingType::Texture}
    };
    
    pipelineInfo.render_pass = irradianceRenderPass;
    
    prefilterPipeline = gfx->create_graphics_pipeline(pipelineInfo);
}
