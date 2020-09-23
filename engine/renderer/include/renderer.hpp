#pragma once

#include <string_view>
#include <vector>
#include <cmath>

#include "pass.hpp"
#include "matrix.hpp"
#include "object.hpp"
#include "common.hpp"
#include "render_options.hpp"
#include "path.hpp"

namespace ui {
    class Screen;
}

class GFX;
class GFXBuffer;
class GFXCommandBuffer;
class GFXFramebuffer;
class GFXPipeline;
class GFXRenderPass;
class GFXTexture;
class SMAAPass;
class ShadowPass;
class SceneCapture;
class GaussianHelper;
class DoFPass;

class Scene;
struct MeshData;
struct Camera;

constexpr int max_scene_materials = 25, max_scene_lights = 25;

struct RenderScreenOptions {
    bool render_world = false;
    Matrix4x4 mvp;
};

class Material;

class Renderer {
public:
    Renderer(GFX* gfx, const bool enable_imgui = true);
    ~Renderer();

    void resize(const prism::Extent extent);
    void resize_viewport(const prism::Extent extent);

    void set_screen(ui::Screen* screen);
    void init_screen(ui::Screen* screen);
	void update_screen();

    void startCrossfade();
    void startSceneBlur();
    void stopSceneBlur();

    float fade = 0.0f;
    bool fading = false;

    bool blurring = false;
    bool hasToStore = true;
    int blurFrame = 0;

    GFXTexture* blurStore = nullptr;

    struct ControllerContinuity {
        int elementOffset = 0;
    };

    void render(Scene* scene, int index);

    void render_screen(GFXCommandBuffer* commandBuffer, ui::Screen* screen, ControllerContinuity& continuity, RenderScreenOptions options = RenderScreenOptions());
    void render_camera(GFXCommandBuffer* command_buffer, Scene& scene, Object camera_object, Camera& camera, prism::Extent extent, ControllerContinuity& continuity);
    
    void create_mesh_pipeline(Material& material);
    
    // passes
    template<class T, typename... Args>
    T* addPass(Args&&... args) {
        auto t = std::make_unique<T>(args...);
        t->initialize();
        
        return static_cast<T*>(passes.emplace_back(std::move(t)).get());
    }

    GFXTexture* get_requested_texture(PassTextureType type) {
        for(auto& pass : passes) {
            auto texture = pass->get_requested_texture(type);
            if(texture != nullptr)
                return texture;
        }

        return nullptr;
    }

	GFXRenderPass* getOffscreenRenderPass() const {
		return offscreenRenderPass;
	}

    GFXTexture* offscreenColorTexture = nullptr;
    GFXTexture* offscreenDepthTexture = nullptr;

    GFXTexture* viewportColorTexture = nullptr;

    bool viewport_mode = false;
    prism::Extent viewport_extent;
    
    bool gui_only_mode = false;

    prism::Extent get_extent() const {
        return viewport_mode ? viewport_extent : extent;
    }

    prism::Extent get_render_extent() const {
        const auto extent = get_extent();
        
        return {static_cast<uint32_t>(std::max(int(extent.width * render_options.render_scale), 1)),
                static_cast<uint32_t>(std::max(int(extent.height * render_options.render_scale), 1))};
    }
    
    std::unique_ptr<ShadowPass> shadow_pass;
    std::unique_ptr<SceneCapture> scene_capture;

    GFXTexture* dummyTexture = nullptr;
    GFXRenderPass* unormRenderPass = nullptr;
    GFXPipeline* renderToUnormTexturePipeline = nullptr;
    GFXRenderPass* viewportRenderPass = nullptr;

private:
    void createDummyTexture();
    void createOffscreenResources();
    void createMeshPipeline();
    void createPostPipeline();
    void createFontPipeline();
    void createSkyPipeline();
    void createUIPipeline();
    void createGaussianResources();
    void createBRDF();
    void create_histogram_resources();

    GFX* gfx = nullptr;
    prism::Extent extent;

    ui::Screen* current_screen = nullptr;

    // offscreen
    GFXTexture* offscreenBackTexture = nullptr;
    GFXFramebuffer* offscreenFramebuffer = nullptr;
    GFXRenderPass* offscreenRenderPass = nullptr;

    GFXFramebuffer* viewportFramebuffer = nullptr;

    // mesh
    GFXBuffer* sceneBuffer = nullptr;

    // sky
    GFXPipeline* skyPipeline = nullptr;

    // post
    GFXPipeline* postPipeline = nullptr;
    GFXPipeline* renderToTexturePipeline = nullptr;
    GFXPipeline* renderToViewportPipeline = nullptr;

    // font
    GFXTexture* fontTexture = nullptr;
    GFXPipeline* textPipeline, *worldTextPipeline = nullptr;
    int instanceAlignment = 0;
    
    // brdf
    GFXPipeline* brdfPipeline = nullptr;
    GFXTexture* brdfTexture = nullptr;
    GFXFramebuffer* brdfFramebuffer = nullptr;
    GFXRenderPass* brdfRenderPass = nullptr;

    // general ui
    GFXPipeline* generalPipeline, *worldGeneralPipeline = nullptr;
    
    // histogram compute
    GFXPipeline* histogram_pipeline = nullptr, *histogram_average_pipeline = nullptr;
    GFXBuffer* histogram_buffer = nullptr;
    GFXTexture* average_luminance_texture = nullptr;

    std::unique_ptr<SMAAPass> smaaPass;
    std::unique_ptr<GaussianHelper> gHelper;
    std::unique_ptr<DoFPass> dofPass;
    
    std::vector<std::unique_ptr<Pass>> passes;
    
    double current_render_scale = 1.0;
};
