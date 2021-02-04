#include "imguipass.hpp"

#include <imgui.h>

#include "engine.hpp"
#include "file.hpp"
#include "gfx.hpp"
#include "gfx_commandbuffer.hpp"
#include "log.hpp"
#include "assertions.hpp"
#include "renderer.hpp"

void ImGuiPass::initialize() {
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "GFX";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

#if !defined(PLATFORM_TVOS) && !defined(PLATFORM_IOS)
    load_font("OpenSans-Regular.ttf");
#endif
    create_font_texture();
}

void ImGuiPass::resize(const prism::Extent extent) {
    GFXGraphicsPipelineCreateInfo createInfo;
    createInfo.label = "ImGui";
    createInfo.shaders.vertex_path = "imgui.vert";
    createInfo.shaders.fragment_path = "imgui.frag";

    GFXVertexInput vertexInput = {};
    vertexInput.stride = sizeof(ImDrawVert);

    createInfo.vertex_input.inputs.push_back(vertexInput);

    GFXVertexAttribute positionAttribute = {};
    positionAttribute.format = GFXVertexFormat::FLOAT2;
    positionAttribute.offset = offsetof(ImDrawVert, pos);

    createInfo.vertex_input.attributes.push_back(positionAttribute);

    GFXVertexAttribute uvAttribute = {};
    uvAttribute.location = 1;
    uvAttribute.format = GFXVertexFormat::FLOAT2;
    uvAttribute.offset = offsetof(ImDrawVert, uv);

    createInfo.vertex_input.attributes.push_back(uvAttribute);

    GFXVertexAttribute colAttribute = {};
    colAttribute.location = 2;
    colAttribute.format = GFXVertexFormat::UNORM4;
    colAttribute.offset = offsetof(ImDrawVert, col);

    createInfo.vertex_input.attributes.push_back(colAttribute);

    createInfo.blending.enable_blending = true;
    createInfo.blending.src_rgb = GFXBlendFactor::SrcAlpha;
    createInfo.blending.src_alpha= GFXBlendFactor::SrcAlpha;
    createInfo.blending.dst_rgb = GFXBlendFactor::OneMinusSrcAlpha;
    createInfo.blending.dst_alpha = GFXBlendFactor::OneMinusSrcAlpha;

    createInfo.shader_input.push_constants = {
        {sizeof(Matrix4x4), 0}
    };

    createInfo.shader_input.bindings = {
        {1, GFXBindingType::PushConstant},
        {2, GFXBindingType::Texture}
    };

    pipeline = engine->get_gfx()->create_graphics_pipeline(createInfo);
}

void ImGuiPass::render_post(GFXCommandBuffer* command_buffer, const int index) {
    ImDrawData* draw_data = nullptr;
    if(index == 0) {
        draw_data = ImGui::GetDrawData();
    } else {
        auto& io = ImGui::GetPlatformIO();
        for(int i = 1; i < io.Viewports.size(); i++) {
            if((io.Viewports[i]->Flags & ImGuiViewportFlags_Minimized) == 0)
                if(*(int*)io.Viewports[i]->PlatformHandle == index)
                    draw_data = io.Viewports[i]->DrawData;
        }
    }

    Expects(draw_data != nullptr);
    
    if(draw_data->TotalIdxCount == 0 || draw_data->TotalVtxCount == 0)
        return;
    
    const int framebuffer_width = static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    const int framebuffer_height = static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    
    Expects(framebuffer_width > 0);
    Expects(framebuffer_height > 0);
    
    update_buffers(*draw_data);

    command_buffer->set_graphics_pipeline(pipeline);

    command_buffer->set_vertex_buffer(vertex_buffer, 0, 0);
    command_buffer->set_index_buffer(index_buffer, IndexType::UINT16);

    const Matrix4x4 projection = transform::orthographic(draw_data->DisplayPos.x,
                                                draw_data->DisplayPos.x + draw_data->DisplaySize.x,
                                                draw_data->DisplayPos.y + draw_data->DisplaySize.y,
                                                draw_data->DisplayPos.y,
                                                0.0f,
                                                1.0f);
                 
    command_buffer->set_push_constant(&projection, sizeof(Matrix4x4));
    
    const ImVec2 clip_offset = draw_data->DisplayPos;
    const ImVec2 clip_scale = draw_data->FramebufferScale;

    int vertex_offset = 0;
    int index_offset = 0;
    for(int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for(int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

            if(pcmd->TextureId != nullptr)
                command_buffer->bind_texture((GFXTexture*)pcmd->TextureId, 2);
            
            if(pcmd->UserCallback != nullptr) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                ImVec4 clip_rect = {};
                clip_rect.x = (pcmd->ClipRect.x - clip_offset.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_offset.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_offset.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_offset.y) * clip_scale.y;

                if(clip_rect.x < framebuffer_width && clip_rect.y < framebuffer_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
                    command_buffer->draw_indexed(pcmd->ElemCount,
                                                (index_offset + pcmd->IdxOffset),
                                                (vertex_offset + pcmd->VtxOffset), 0);
                }
            }
        }

        index_offset += cmd_list->IdxBuffer.Size;
        vertex_offset += cmd_list->VtxBuffer.Size;
    }
}

void ImGuiPass::load_font(const std::string_view filename) {
    Expects(!filename.empty());
    
    ImGuiIO& io = ImGui::GetIO();

#if !defined(PLATFORM_IOS) || !defined(PLATFORM_TVOS)
    if(io.Fonts->Fonts.empty()) {
        auto file = file::open(file::app_domain / filename);
        if(file != std::nullopt) {
            file->read_all();
            
            io.Fonts->AddFontFromMemoryTTF(file->cast_data<unsigned char>(), file->size(), 15.0 * platform::get_window_dpi(0));
            ImGui::GetIO().FontGlobalScale = 1.0 / platform::get_window_dpi(0);
        } else {
            console::error(System::Renderer, "Failed to load font file for imgui!");
            return;
        }
    }
#endif
}

void ImGuiPass::create_font_texture() {
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels = nullptr;
    int width = -1, height = -1;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    
    GFXTextureCreateInfo createInfo = {};
    createInfo.label = "ImGui Font";
    createInfo.width = width;
    createInfo.height = height;
    createInfo.format = GFXPixelFormat::RGBA8_UNORM;
    createInfo.usage = GFXTextureUsage::Sampled;
    
    font_texture = engine->get_gfx()->create_texture(createInfo);
    engine->get_gfx()->copy_texture(font_texture, pixels, width * height * 4);
    
    io.Fonts->TexID = reinterpret_cast<void*>(font_texture);
}

void ImGuiPass::update_buffers(const ImDrawData& draw_data) {
    const int new_vertex_size = draw_data.TotalVtxCount * sizeof(ImDrawVert);
    const int new_index_size = draw_data.TotalIdxCount * sizeof(ImDrawIdx);
    
    Expects(new_vertex_size > 0);
    Expects(new_index_size > 0);
    
    if(vertex_buffer == nullptr || current_vertex_size < new_vertex_size) {
        vertex_buffer = engine->get_gfx()->create_buffer(nullptr, new_vertex_size, true, GFXBufferUsage::Vertex);
        current_vertex_size = new_vertex_size;
    }
    
    if(index_buffer == nullptr || current_index_size < new_index_size) {
        index_buffer = engine->get_gfx()->create_buffer(nullptr, new_index_size, true, GFXBufferUsage::Index);
        current_index_size = new_index_size;
    }
    
    int vertex_offset = 0;
    int index_offset = 0;
    for(int i = 0; i < draw_data.CmdListsCount; i++) {
        const ImDrawList* cmd_list = draw_data.CmdLists[i];
        
        engine->get_gfx()->copy_buffer(vertex_buffer, cmd_list->VtxBuffer.Data, vertex_offset, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        engine->get_gfx()->copy_buffer(index_buffer, cmd_list->IdxBuffer.Data, index_offset, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        
        vertex_offset += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
        index_offset += cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
    }
}
