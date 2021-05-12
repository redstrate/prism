#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "pass.hpp"
#include "gfx_renderpass.hpp"
#include "gfx_pipeline.hpp"
#include "gfx_texture.hpp"
#include "gfx_buffer.hpp"
#include "gfx_framebuffer.hpp"
#include "matrix.hpp"
#include "object.hpp"
#include "assetptr.hpp"

class Texture;
class Mesh;

using Object = uint64_t;

struct SelectableObject {
    enum class Type {
        Object,
        Handle
    } type;
    
    Matrix4x4 axis_model;
    
    enum class Axis {
        X,
        Y,
        Z
    } axis;
    
    enum class RenderType {
        Mesh,
        Sphere
    } render_type;
    
    float sphere_size = 1.0f;
    
    Object object;
};

class DebugPass : public Pass {
public:
    void initialize() override;

    void create_render_target_resources(RenderTarget& target) override;

	void render_scene(Scene& scene, GFXCommandBuffer* commandBuffer) override;

    void get_selected_object(int x, int y, std::function<void(SelectableObject)> callback);
    void draw_arrow(GFXCommandBuffer* commandBuffer, prism::float3 color, Matrix4x4 model);
    
    GFXTexture* get_requested_texture(PassTextureType type) override {
        if(type == PassTextureType::SelectionSobel)
            return sobelTexture;
        
        return nullptr;
    }
    
    GFXTexture* selectTexture = nullptr;
    GFXTexture* selectDepthTexture = nullptr;
    GFXTexture* sobelTexture = nullptr;

    Object selected_object = NullObject;
    
private:
    void createOffscreenResources();
    
    prism::Extent extent;
    
    std::vector<SelectableObject> selectable_objects;
    
    AssetPtr<Texture> pointTexture, spotTexture, sunTexture, probeTexture;
    AssetPtr<Mesh> cubeMesh, arrowMesh, sphereMesh;

    GFXPipeline* primitive_pipeline = nullptr;
    GFXPipeline* arrow_pipeline = nullptr;
    GFXPipeline* billboard_pipeline = nullptr;

    GFXPipeline* selectPipeline = nullptr;
    GFXRenderPass* selectRenderPass = nullptr;
    GFXFramebuffer* selectFramebuffer = nullptr;
    GFXBuffer* selectBuffer = nullptr;
    
    GFXPipeline* sobelPipeline = nullptr;
    GFXFramebuffer* sobelFramebuffer = nullptr;
    GFXRenderPass* sobelRenderPass = nullptr;

    GFXBuffer* scene_info_buffer = nullptr;
};
