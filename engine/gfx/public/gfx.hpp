#pragma once

#include <string>
#include <vector>
#include <variant>

#include "shadercompiler.hpp"

class GFXBuffer;
class GFXPipeline;
class GFXCommandBuffer;
class GFXTexture;
class GFXFramebuffer;
class GFXRenderPass;
class GFXObject;
class GFXSampler;

enum class GFXPixelFormat : int {
    R_32F = 0,
    RGBA_32F = 1,
    RGBA8_UNORM = 2,
    R8_UNORM = 3,
    R8G8_UNORM = 4,
    R8G8_SFLOAT = 5,
    R8G8B8A8_UNORM = 6,
    R16G16B16A16_SFLOAT = 7,
    DEPTH_32F = 8
};

enum class GFXVertexFormat : int {
    FLOAT2 = 0,
    FLOAT3 = 1,
    FLOAT4 = 2,
    INT = 3,
    UNORM4 = 4,
    INT4 = 5
};

enum class GFXTextureUsage : int {
    Sampled = 1,
    Attachment = 2
};

inline GFXTextureUsage operator|(const GFXTextureUsage a, const GFXTextureUsage b) {
    return static_cast<GFXTextureUsage>(static_cast<int>(a) | static_cast<int>(b));
}

inline GFXTextureUsage operator&(const GFXTextureUsage a, const GFXTextureUsage b) {
    return static_cast<GFXTextureUsage>(static_cast<int>(a) & static_cast<int>(b));
}

enum class GFXBufferUsage : int {
	Storage,
	Vertex,
	Index
};

enum class GFXBlendFactor : int {
	One,
    Zero,
    SrcColor,
    DstColor,
    SrcAlpha,
    DstAlpha,
    OneMinusSrcAlpha,
    OneMinusSrcColor
};

enum class SamplingMode : int {
    Repeat,
    ClampToEdge,
    ClampToBorder
};

enum class GFXPrimitiveType {
    Triangle,
    TriangleStrip
};

enum class GFXPolygonType {
    Fill,
    Line
};

enum class GFXCullingMode {
    Backface,
    Frontface,
    None
};

enum class GFXWindingMode {
    Clockwise,
    CounterClockwise
};

struct GFXVertexInput {
    int location = 0;
    int stride = 0;
};

struct GFXVertexAttribute {
    int binding = 0, location = 0, offset = 0;
    GFXVertexFormat format = GFXVertexFormat::FLOAT4;
};

enum class GFXDepthMode {
    None,
    Less,
    LessOrEqual,
    Greater
};

struct GFXPushConstant {
    int size = 0, offset = 0;
};

enum class GFXBindingType {
    StorageBuffer,
    PushConstant,
    Texture
};

enum class GFXTextureType {
    Single2D,
    Array2D,
    Cubemap,
    CubemapArray
};

struct GFXShaderBinding {
    int binding = 0;

    GFXBindingType type = GFXBindingType::StorageBuffer;
};

struct GFXShaderConstant {
    int index = 0;
    
    enum class Type {
        Integer
    } type;
    
    union {
        int value;
    };
};

using GFXShaderConstants = std::vector<GFXShaderConstant>;

struct GFXGraphicsPipelineCreateInfo {
    std::string label; // only used for debug
    
    struct Shaders {
        std::string_view vertex_path, fragment_path;

        ShaderSource vertex_src, fragment_src;
        
        GFXShaderConstants vertex_constants, fragment_constants;
    } shaders;

    struct Rasterization {
        GFXPrimitiveType primitive_type = GFXPrimitiveType::Triangle;

        GFXPolygonType polygon_type = GFXPolygonType::Fill;

        GFXCullingMode culling_mode = GFXCullingMode::None;

        GFXWindingMode winding_mode = GFXWindingMode::Clockwise;
    } rasterization;

    struct Blending {
        GFXBlendFactor src_rgb = GFXBlendFactor::One, dst_rgb = GFXBlendFactor::One;
        GFXBlendFactor src_alpha = GFXBlendFactor::One, dst_alpha = GFXBlendFactor::One;

        bool enable_blending = false;
    } blending;

    struct VertexInput {
        std::vector<GFXVertexInput> inputs;

        std::vector<GFXVertexAttribute> attributes;
    } vertex_input;

    struct ShaderBindings {
        std::vector<GFXPushConstant> push_constants;

        std::vector<GFXShaderBinding> bindings;
    } shader_input;

    struct Depth {
        GFXDepthMode depth_mode = GFXDepthMode::None;
    } depth;

    GFXRenderPass* render_pass = nullptr;
};

struct GFXComputePipelineCreateInfo {
    std::string label; // only used for debug
    
    struct Shaders {
        std::string_view compute_path;
    } shaders;
};

struct GFXFramebufferCreateInfo {
	GFXRenderPass* render_pass;
    std::vector<GFXTexture*> attachments;
};

struct GFXRenderPassCreateInfo {
    std::vector<GFXPixelFormat> attachments;
};

enum class GFXBorderColor {
    OpaqueWhite,
    OpaqueBlack
};

enum class GFXCompareFunction {
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
};

enum class GFXFilter {
    Nearest,
    Linear
};

struct GFXTextureCreateInfo {
    GFXTextureType type = GFXTextureType::Single2D;
    uint32_t width = 0;
    uint32_t height = 0;
    GFXPixelFormat format;
    GFXTextureUsage usage;
    int array_length = 1;
    int mip_count = 1;
     
    // sampler
    SamplingMode samplingMode = SamplingMode::Repeat;
    GFXBorderColor border_color = GFXBorderColor::OpaqueWhite;
    bool compare_enabled = false;
    GFXCompareFunction compare_function = GFXCompareFunction::Never;
};

struct GFXSamplerCreateInfo {
    GFXFilter min_filter = GFXFilter::Linear, mag_filter = GFXFilter::Linear;
    SamplingMode samplingMode = SamplingMode::Repeat;
    GFXBorderColor border_color = GFXBorderColor::OpaqueWhite;
    bool compare_enabled = false;
    GFXCompareFunction compare_function = GFXCompareFunction::Never;
};

using GFXSize = uint64_t;

enum class GFXContext {
	None,
    Metal,
	OpenGL,
	DirectX,
    Vulkan
};

struct GFXCreateInfo {
    bool api_validation_enabled = false;
};

enum class GFXFeature {
    CubemapArray
};

class GFX {
public:
	// check for runtime support
	virtual bool is_supported() { return false; }
	virtual GFXContext required_context() { return GFXContext::None; }
    virtual ShaderLanguage accepted_shader_language() {}
    virtual const char* get_name() { return nullptr; }
    
    virtual bool supports_feature([[maybe_unused]] const GFXFeature feature) { return true; }

	// try to initialize
    virtual bool initialize([[maybe_unused]] const GFXCreateInfo& createInfo) { return false; }

    virtual void initialize_view([[maybe_unused]] void* native_handle,
                                 [[maybe_unused]] const int identifier,
                                 [[maybe_unused]] const uint32_t width,
                                 [[maybe_unused]] const uint32_t height) {}
    
	virtual void recreate_view([[maybe_unused]] const int identifier,
                               [[maybe_unused]] const uint32_t width,
                               [[maybe_unused]] const uint32_t height) {}
    
    virtual void remove_view([[maybe_unused]] const int identifier) {}

    // buffer operations
    virtual GFXBuffer* create_buffer([[maybe_unused]] void* data,
                                     [[maybe_unused]] const GFXSize size,
                                     [[maybe_unused]] const bool is_dynamic,
                                     [[maybe_unused]] const GFXBufferUsage usage) { return nullptr; }
    virtual void copy_buffer([[maybe_unused]] GFXBuffer* buffer,
                             [[maybe_unused]] void* data,
                             [[maybe_unused]] const GFXSize offset,
                             [[maybe_unused]] const GFXSize size) { }

    virtual void* get_buffer_contents([[maybe_unused]] GFXBuffer* buffer) { return nullptr; }
    virtual void release_buffer_contents([[maybe_unused]] GFXBuffer* buffer,
                                         [[maybe_unused]] void* handle) {}

    // texture operations
    virtual GFXTexture* create_texture([[maybe_unused]] const GFXTextureCreateInfo& info) { return nullptr; }
    virtual void copy_texture([[maybe_unused]] GFXTexture* texture,
                              [[maybe_unused]] void* data,
                              [[maybe_unused]] const GFXSize size) {}
    virtual void copy_texture([[maybe_unused]] GFXTexture* from,
                              [[maybe_unused]] GFXTexture* to) {}
    virtual void copy_texture([[maybe_unused]] GFXTexture* from,
                              [[maybe_unused]] GFXBuffer* to) {}
    
    // sampler opeations
    virtual GFXSampler* create_sampler([[maybe_unused]] const GFXSamplerCreateInfo& info) { return nullptr; }

    // framebuffer operations
    virtual GFXFramebuffer* create_framebuffer([[maybe_unused]] const GFXFramebufferCreateInfo& info) { return nullptr; }

    // render pass operations
    virtual GFXRenderPass* create_render_pass([[maybe_unused]] const GFXRenderPassCreateInfo& info) { return nullptr; }

    // pipeline operations
    virtual GFXPipeline* create_graphics_pipeline([[maybe_unused]] const GFXGraphicsPipelineCreateInfo& info) { return nullptr; }
    virtual GFXPipeline* create_compute_pipeline([[maybe_unused]] const GFXComputePipelineCreateInfo& info) { return nullptr; }

    // misc operations
    virtual GFXSize get_alignment(const GFXSize size) { return size; }

    virtual GFXCommandBuffer* acquire_command_buffer() { return nullptr; }
    
    virtual void submit([[maybe_unused]] GFXCommandBuffer* command_buffer,
                        [[maybe_unused]] const int window = -1) {}
};
