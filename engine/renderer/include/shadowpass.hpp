#pragma once

#include "math.hpp"
#include "object.hpp"
#include "components.hpp"

class GFX;
class GFXCommandBuffer;
class GFXFramebuffer;
class GFXPipeline;
class GFXRenderPass;
class GFXTexture;
class GFXSampler;
class GFXBuffer;
class Scene;
struct CameraFrustum;

class ShadowPass {
public:
    ShadowPass(GFX* gfx);
    
    void create_scene_resources(Scene& scene);
    
    void render(GFXCommandBuffer* command_buffer, Scene& scene);
    
private:
    void render_meshes(GFXCommandBuffer* command_buffer, Scene& scene, const Matrix4x4 light_matrix, const Matrix4x4 model, const prism::float3 light_position, const Light::Type type, const CameraFrustum& frustum, const int base_instance);
    
    void render_sun(GFXCommandBuffer* command_buffer, Scene& scene, Object light_object, Light& light);
    void render_spot(GFXCommandBuffer* command_buffer, Scene& scene, Object light_object, Light& light);
    void render_point(GFXCommandBuffer* command_buffer, Scene& scene, Object light_object, Light& light);
    
    int last_point_light = 0;
    int last_spot_light = 0;
    
    void create_render_passes();
    void create_pipelines();
    void create_offscreen_resources();
    
    GFXRenderPass* render_pass = nullptr;
    GFXRenderPass* cube_render_pass = nullptr;
    
    GFXTexture* offscreen_color_texture = nullptr;
    GFXTexture* offscreen_depth = nullptr;
    GFXFramebuffer* offscreen_framebuffer = nullptr;

    GFXBuffer* point_location_buffer = nullptr;
    prism::float3* point_location_map = nullptr;
    
    // sun
    GFXPipeline* static_sun_pipeline = nullptr;
    GFXPipeline* skinned_sun_pipeline = nullptr;
    
    // spot
    GFXPipeline* static_spot_pipeline = nullptr;
    GFXPipeline* skinned_spot_pipeline = nullptr;

    // point
    GFXPipeline* static_point_pipeline = nullptr;
    GFXPipeline* skinned_point_pipeline = nullptr;
};
