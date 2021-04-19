#pragma once

#include <string_view>
#include <vector>
#include <cmath>
#include <functional>

#include "pass.hpp"
#include "matrix.hpp"
#include "object.hpp"
#include "common.hpp"
#include "render_options.hpp"
#include "path.hpp"
#include "shadercompiler.hpp"
#include "rendertarget.hpp"

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
    explicit Renderer(GFX* gfx, bool enable_imgui = true);
    ~Renderer();

    RenderTarget* allocate_render_target(prism::Extent extent);
    void resize_render_target(RenderTarget& target, prism::Extent extent);
    void recreate_all_render_targets();
    
    void set_screen(ui::Screen* screen);
    void init_screen(ui::Screen* screen);
	void update_screen();

    struct ControllerContinuity {
        int elementOffset = 0;
    };

    void render(GFXCommandBuffer* command_buffer, Scene* scene, RenderTarget& target, int index);

    void render_screen(GFXCommandBuffer* commandBuffer, ui::Screen* screen, prism::Extent extent, ControllerContinuity& continuity, RenderScreenOptions options = RenderScreenOptions());
    void render_camera(GFXCommandBuffer* command_buffer, Scene& scene, Object camera_object, Camera& camera, prism::Extent extent, RenderTarget& target, ControllerContinuity& continuity);
    
    void create_mesh_pipeline(Material& material) const;
    
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
    
    GFXRenderPass* offscreenRenderPass = nullptr;
    
    std::unique_ptr<ShadowPass> shadow_pass;
    std::unique_ptr<SceneCapture> scene_capture;

    GFXTexture* dummyTexture = nullptr;
    GFXRenderPass* unormRenderPass = nullptr;
    GFXPipeline* renderToUnormTexturePipeline = nullptr;
    GFXRenderPass* viewportRenderPass = nullptr;
    
    ShaderSource register_shader(std::string_view shader_file);
    void associate_shader_reload(std::string_view shader_file, const std::function<void()>& reload_function);
    void reload_shader(std::string_view shader_file, std::string_view shader_source);
    
    struct RegisteredShader {
        std::string filename;
        std::string injected_shader_source;
        std::function<void()> reload_function;
    };
    
    std::vector<RegisteredShader> registered_shaders;
    
    bool reloading_shader = false;
    
    [[nodiscard]] std::vector<RenderTarget*> get_all_render_targets() const {
        return render_targets;
    }
    
private:
    void createDummyTexture();
    void create_render_target_resources(RenderTarget& target);
    void createPostPipelines();
    void createFontTexture();
    void createSkyPipeline();
    void createUIPipelines();
    void generateBRDF();
    void create_histogram_resources();

    GFX* gfx = nullptr;
    
    std::vector<RenderTarget*> render_targets;

    ui::Screen* current_screen = nullptr;
    
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
    std::unique_ptr<DoFPass> dofPass;
    
    std::vector<std::unique_ptr<Pass>> passes;
};
