#pragma once

#include <vector>
#include <cstring>
#include <array>
#include <string_view>

#include "common.hpp"

class GFXPipeline;
class GFXBuffer;
class GFXFramebuffer;
class GFXRenderPass;
class GFXSampler;

struct GFXRenderPassBeginInfo {
    struct ClearColor {
        float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
    } clear_color;
    
    prism::Rectangle render_area;
    
    GFXRenderPass* render_pass = nullptr;
    GFXFramebuffer* framebuffer = nullptr;
};

struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float min_depth = 0.0f;
    float max_depth = 1.0f;
};

enum class IndexType : int {
    UINT16 = 0,
    UINT32 = 1
};

enum class GFXCommandType {
    Invalid,
    SetRenderPass,
    SetGraphicsPipeline,
    SetComputePipeline,
    SetVertexBuffer,
    SetIndexBuffer,
    SetPushConstant,
    BindShaderBuffer,
    BindTexture,
    BindSampler,
    Draw,
    DrawIndexed,
    MemoryBarrier,
    CopyTexture,
    SetViewport,
    SetScissor,
    GenerateMipmaps,
    SetDepthBias,
    PushGroup,
    PopGroup,
    InsertLabel,
    Dispatch
};

struct GFXDrawCommand {
    GFXCommandType type = GFXCommandType::Invalid;
    
    struct CommandData {
        GFXRenderPassBeginInfo set_render_pass;
        
        struct SetGraphicsPipelineData {
            GFXPipeline* pipeline = nullptr;
        } set_graphics_pipeline;
        
        struct SetComputePipelineData {
            GFXPipeline* pipeline = nullptr;
        } set_compute_pipeline;
        
        struct SetVertexData {
            GFXBuffer* buffer = nullptr;
            int offset = 0;
            int index = 0;
        } set_vertex_buffer;
        
        struct SetIndexData {
            GFXBuffer* buffer = nullptr;
			IndexType index_type = IndexType::UINT32;
        } set_index_buffer;
        
        struct SetPushData {
            std::vector<unsigned char> bytes;
            size_t size = 0;
        } set_push_constant;
        
        struct BindShaderData {
            GFXBuffer* buffer = nullptr;
            int offset = 0;
            int index = 0;
            int size = 0;
        } bind_shader_buffer;
        
        struct BindTextureData {
            GFXTexture* texture = nullptr;
            int index = 0;
        } bind_texture;
        
        struct BindSamplerData {
            GFXSampler* sampler = nullptr;
            int index = 0;
        } bind_sampler;
        
        struct DrawData {
            int vertex_offset = 0;
            int vertex_count = 0;
            int base_instance = 0;
            int instance_count = 0;
        } draw;
        
        struct DrawIndexedData {
            int index_count = 0;
            int first_index = 0;
            int vertex_offset = 0;
            int base_instance = 0;
        } draw_indexed;

        struct CopyTextureData {
            GFXTexture* src = nullptr, *dst = nullptr;
            int width = 0, height = 0;
            int to_slice = 0;
            int to_layer = 0;
            int to_level = 0;
        } copy_texture;
        
        struct SetViewportData {
            Viewport viewport;
        } set_viewport;
        
        struct SetScissorData {
            prism::Rectangle rect;
        } set_scissor;
        
        struct GenerateMipmapData {
            GFXTexture* texture = nullptr;
            int mip_count = 0;
        } generate_mipmaps;
        
        struct SetDepthBiasData {
            float constant = 0.0f;
            float clamp = 0.0f;
            float slope_factor = 0.0f;
        } set_depth_bias;
        
        struct PushGroupData {
            std::string_view name;
        } push_group;
        
        struct InsertLabelData {
            std::string_view name;
        } insert_label;
        
        struct DispatchData {
            uint32_t group_count_x, group_count_y, group_count_z;
        } dispatch;
    } data;
};

class GFXCommandBuffer {
public:
    void set_render_pass(GFXRenderPassBeginInfo& info) {
        GFXDrawCommand command;
        command.type = GFXCommandType::SetRenderPass;
        command.data.set_render_pass = info;
        
        commands.push_back(command);
    }
    
    void set_graphics_pipeline(GFXPipeline* pipeline) {
        GFXDrawCommand command;
        command.type = GFXCommandType::SetGraphicsPipeline;
        command.data.set_graphics_pipeline.pipeline = pipeline;
        
        commands.push_back(command);
    }
    
    void set_compute_pipeline(GFXPipeline* pipeline) {
        GFXDrawCommand command;
        command.type = GFXCommandType::SetComputePipeline;
        command.data.set_compute_pipeline.pipeline = pipeline;
        
        commands.push_back(command);
    }
    
    void set_vertex_buffer(GFXBuffer* buffer, int offset, int index) {
        GFXDrawCommand command;
        command.type = GFXCommandType::SetVertexBuffer;
        command.data.set_vertex_buffer.buffer = buffer;
        command.data.set_vertex_buffer.offset = offset;
        command.data.set_vertex_buffer.index = index;
        
        commands.push_back(command);
    }
    
    void set_index_buffer(GFXBuffer* buffer, IndexType indexType) {
        GFXDrawCommand command;
        command.type = GFXCommandType::SetIndexBuffer;
        command.data.set_index_buffer.buffer = buffer;
		command.data.set_index_buffer.index_type = indexType;
        
        commands.push_back(command);
    }
    
    void set_push_constant(const void* data, const size_t size) {
        GFXDrawCommand command;
        command.type = GFXCommandType::SetPushConstant;
        command.data.set_push_constant.size = size;
        command.data.set_push_constant.bytes.resize(size);
        
        memcpy(command.data.set_push_constant.bytes.data(), data, size);
        
        commands.push_back(command);
    }
    
    void bind_shader_buffer(GFXBuffer* buffer, int offset, int index, int size) {
        GFXDrawCommand command;
        command.type = GFXCommandType::BindShaderBuffer;
        command.data.bind_shader_buffer.buffer = buffer;
        command.data.bind_shader_buffer.offset = offset;
        command.data.bind_shader_buffer.index = index;
        command.data.bind_shader_buffer.size = size;

        
        commands.push_back(command);
    }
    
    void bind_texture(GFXTexture* texture, int index) {
        GFXDrawCommand command;
        command.type = GFXCommandType::BindTexture;
        command.data.bind_texture.texture = texture;
        command.data.bind_texture.index = index;
        
        commands.push_back(command);
    }
    
    void bind_sampler(GFXSampler* sampler, const int index) {
        GFXDrawCommand command;
        command.type = GFXCommandType::BindSampler;
        command.data.bind_sampler.sampler = sampler;
        command.data.bind_sampler.index = index;
        
        commands.push_back(command);
    }
    
    void draw(int offset, int count, int instance_base, int instance_count) {
        GFXDrawCommand command;
        command.type = GFXCommandType::Draw;
        command.data.draw.vertex_offset = offset;
        command.data.draw.vertex_count = count;
        command.data.draw.base_instance = instance_base;
        command.data.draw.instance_count = instance_count;
        
        commands.push_back(command);
    }
    
    void draw_indexed(int indexCount, int firstIndex, int vertexOffset, int base_instance) {
        GFXDrawCommand command;
        command.type = GFXCommandType::DrawIndexed;
        command.data.draw_indexed.vertex_offset = vertexOffset;
        command.data.draw_indexed.first_index = firstIndex;
        command.data.draw_indexed.index_count = indexCount;
        command.data.draw_indexed.base_instance = base_instance;
        
        commands.push_back(command);
    }
    
    void memory_barrier() {
        GFXDrawCommand command;
        command.type = GFXCommandType::MemoryBarrier;
        
        commands.push_back(command);
    }
    
    void copy_texture(GFXTexture* src, int width, int height, GFXTexture* dst, int to_slice, int to_layer, int to_level) {
        GFXDrawCommand command;
        command.type = GFXCommandType::CopyTexture;
        command.data.copy_texture.src = src;
        command.data.copy_texture.width = width;
        command.data.copy_texture.height = height;
        command.data.copy_texture.dst = dst;
        command.data.copy_texture.to_slice = to_slice;
        command.data.copy_texture.to_layer = to_layer;
        command.data.copy_texture.to_level = to_level;
        
        commands.push_back(command);
    }
    
    void set_viewport(Viewport viewport) {
        GFXDrawCommand command;
        command.type = GFXCommandType::SetViewport;
        command.data.set_viewport.viewport = viewport;
        
        commands.push_back(command);
    }
    
    void set_scissor(const prism::Rectangle rect) {
        GFXDrawCommand command;
        command.type = GFXCommandType::SetScissor;
        command.data.set_scissor.rect = rect;

        commands.push_back(command);
    }
    
    void generate_mipmaps(GFXTexture* texture, int mip_count) {
        GFXDrawCommand command;
        command.type = GFXCommandType::GenerateMipmaps;
        command.data.generate_mipmaps.texture = texture;
        command.data.generate_mipmaps.mip_count = mip_count;
        
        commands.push_back(command);
    }
    
    void set_depth_bias(const float constant, const float clamp, const float slope) {
        GFXDrawCommand command;
        command.type = GFXCommandType::SetDepthBias;
        command.data.set_depth_bias.constant = constant;
        command.data.set_depth_bias.clamp = clamp;
        command.data.set_depth_bias.slope_factor = slope;
        
        commands.push_back(command);
    }
    
    void push_group(const std::string_view name) {
        GFXDrawCommand command;
        command.type = GFXCommandType::PushGroup;
        command.data.push_group.name = name;
        
        commands.push_back(command);
    }
    
    void pop_group() {
        GFXDrawCommand command;
        command.type = GFXCommandType::PopGroup;
        
        commands.push_back(command);
    }
    
    void insert_label(const std::string_view name) {
        GFXDrawCommand command;
        command.type = GFXCommandType::InsertLabel;
        command.data.insert_label.name = name;
        
        commands.push_back(command);
    }
    
    void dispatch(const uint32_t group_count_x, const uint32_t group_count_y, const uint32_t group_count_z) {
        GFXDrawCommand command;
        command.type = GFXCommandType::Dispatch;
        command.data.dispatch.group_count_x = group_count_x;
        command.data.dispatch.group_count_y = group_count_y;
        command.data.dispatch.group_count_z = group_count_z;
        
        commands.push_back(command);
    }
    
    std::vector<GFXDrawCommand> commands;
};
