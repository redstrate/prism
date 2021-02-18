#pragma once

#include <string_view>

#include "pass.hpp"
#include "file.hpp"

class GFXBuffer;
class GFXCommandBuffer;
class GFXPipeline;
class GFXTexture;
struct ImDrawData;

class ImGuiPass : public Pass {
public:
    void initialize() override;
        
    void create_render_target_resources(RenderTarget& target) override;

    void render_post(GFXCommandBuffer* command_buffer, RenderTarget& target, const int index) override;

private:
    void load_font(const std::string_view filename);
    void create_font_texture();
    void update_buffers(RenderTarget& target, const ImDrawData& draw_data);

    std::unique_ptr<file::File> font_file;
    
    GFXPipeline* pipeline = nullptr;
    GFXTexture* font_texture = nullptr;
};
