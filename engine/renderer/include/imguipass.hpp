#pragma once

#include <cstddef>
#include <string_view>

#include "pass.hpp"

class GFXBuffer;
class GFXCommandBuffer;
class GFXPipeline;
class GFXTexture;
struct ImDrawData;

class ImGuiPass : public Pass {
public:
    void initialize() override;
    
    void resize(const Extent extent) override;

    void render_post(GFXCommandBuffer* command_buffer, const int index) override;

private:
    void load_font(const std::string_view filename);
    void create_font_texture();
    void update_buffers(const ImDrawData& draw_data);
    
    GFXPipeline* pipeline = nullptr;
    GFXTexture* font_texture = nullptr;

    GFXBuffer* vertex_buffer = nullptr;
    int current_vertex_size = 0;

    GFXBuffer* index_buffer = nullptr;
    int current_index_size = 0;
};
