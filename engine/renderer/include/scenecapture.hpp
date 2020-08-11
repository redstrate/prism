#pragma once

#include "math.hpp"

class GFX;
class GFXCommandBuffer;
class GFXFramebuffer;
class GFXPipeline;
class GFXRenderPass;
class GFXTexture;
class GFXBuffer;
class Scene;

const int scene_cubemap_resolution = 1024;
const int irradiance_cubemap_resolution = 32;

class SceneCapture {
public:
    SceneCapture(GFX* gfx);
    
    void create_scene_resources(Scene& scene);
    
    void render(GFXCommandBuffer* command_buffer, Scene* scene);
    
    GFXPipeline* irradiancePipeline, *prefilterPipeline = nullptr, *skyPipeline = nullptr;
    GFXRenderPass* renderPass = nullptr, *irradianceRenderPass = nullptr;
    
    GFXTexture* environmentCube = nullptr;

    GFXTexture* offscreenTexture = nullptr, *irradianceOffscreenTexture = nullptr, *prefilteredOffscreenTexture = nullptr;
    GFXTexture* offscreenDepth = nullptr;
    GFXFramebuffer* offscreenFramebuffer = nullptr, *irradianceFramebuffer = nullptr, *prefilteredFramebuffer = nullptr;
    
    GFXBuffer* sceneBuffer = nullptr;
    
    void createSkyResources();
    void createIrradianceResources();
    void createPrefilterResources();
};
