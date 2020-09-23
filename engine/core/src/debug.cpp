#include "debug.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "engine.hpp"
#include "shadowpass.hpp"
#include "scenecapture.hpp"
#include "gfx.hpp"
#include "asset.hpp"
#include "imgui_utility.hpp"
#include "scene.hpp"
#include "renderer.hpp"

void draw_general() {
    ImGui::Text("Platform: %s", platform::get_name());
    ImGui::Text("GFX: %s", engine->get_gfx()->get_name());
}

void draw_asset() {
    ImGui::Text("Asset References");

    ImGui::Separator();
    
    ImGui::BeginChild("asset_child", ImVec2(-1, -1), true);
    
    for(auto& [path, block] : assetm->reference_blocks) {
        ImGui::PushID(&block);
        
        ImGui::Text("- %s has %llu reference(s)", path.c_str(), block->references);
        
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(200, 0, 0));
        
        if(ImGui::SmallButton("Force unload"))
            block->references = 0;
        
        ImGui::PopStyleColor();
            
        ImGui::PopID();
    }
    
    ImGui::EndChild();
}

void draw_lighting() {
    if(engine->get_scene() != nullptr) {
        const auto& lights = engine->get_scene()->get_all<Light>();
        
        ImGui::Text("Lights");
        
        ImGui::Separator();
                
        for(auto& [obj, light] : lights) {
            auto& transform = engine->get_scene()->get<Transform>(obj);
            ImGui::DragFloat3((engine->get_scene()->get(obj).name + "Position").c_str(), transform.position.ptr());
        }
        
        ImGui::Text("Environment");
        ImGui::Separator();
                
        if(ImGui::Button("Reload shadows")) {
            engine->get_scene()->reset_shadows();
        }
        
        if(ImGui::Button("Reload probes")) {
            engine->get_scene()->reset_environment();
        }
    } else {
        ImGui::TextDisabled("No scene loaded.");
    }
}

void draw_renderer() {
    if(engine->get_scene() != nullptr) {
        ImGui::Text("Budgets");
        ImGui::Separator();
        
        const auto& lights = engine->get_scene()->get_all<Light>();
        const auto& probes = engine->get_scene()->get_all<EnvironmentProbe>();
        const auto& renderables = engine->get_scene()->get_all<Renderable>();
        
        ImGui::ProgressBar("Light Budget", lights.size(), max_scene_lights);
        ImGui::ProgressBar("Probe Budget", probes.size(), max_environment_probes);
        
        int material_count = 0;
        for(const auto& [obj, renderable] : renderables) {
            for(auto& material : renderable.materials) {
                if(material)
                    material_count++;
            }
        }
        
        ImGui::ProgressBar("Material Budget", material_count, max_scene_materials);
    } else {
        ImGui::TextDisabled("No scene loaded for budgeting.");
    }
    
    ImGui::Text("Performance");
    ImGui::Separator();
    
    ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
    
    ImGui::Text("Options");
    ImGui::Separator();
    
    ImGui::ComboEnum("Display Color Space", &render_options.display_color_space);
    ImGui::ComboEnum("Tonemapping", &render_options.tonemapping);
    ImGui::DragFloat("Exposure", &render_options.exposure, 0.1f);
    ImGui::DragFloat("Min Luminance", &render_options.min_luminance);
    ImGui::DragFloat("Max Luminance", &render_options.max_luminance);
    
    ImGui::Checkbox("Enable DoF", &render_options.enable_depth_of_field);
    ImGui::DragFloat("DoF Strength", &render_options.depth_of_field_strength);

    bool should_recompile = false;
    
    ImGui::Checkbox("Enable Dynamic Resolution", &render_options.dynamic_resolution);
    
    float render_scale = render_options.render_scale;
    if(ImGui::DragFloat("Render Scale", &render_scale, 0.1f, 1.0f, 0.1f) && render_scale > 0.0f) {
        render_options.render_scale = render_scale;
        engine->get_renderer()->resize(engine->get_renderer()->get_extent());
    }
    
    if(ImGui::InputInt("Shadow Resolution", &render_options.shadow_resolution)) {
        engine->get_renderer()->shadow_pass->create_scene_resources(*engine->get_scene());
        engine->get_scene()->reset_shadows();
    }
    
    ImGui::Checkbox("Enable AA", &render_options.enable_aa);
    should_recompile |= ImGui::Checkbox("Enable IBL", &render_options.enable_ibl);
    should_recompile |= ImGui::Checkbox("Enable Normal Mapping", &render_options.enable_normal_mapping);
    should_recompile |= ImGui::Checkbox("Enable Normal Shadowing", &render_options.enable_normal_shadowing);
    should_recompile |= ImGui::Checkbox("Enable Point Shadows", &render_options.enable_point_shadows);
    should_recompile |= ImGui::ComboEnum("Shadow Filter", &render_options.shadow_filter);
    ImGui::Checkbox("Enable Extra Passes", &render_options.enable_extra_passes);
    ImGui::Checkbox("Enable Frustum Culling", &render_options.enable_frustum_culling);

    if(ImGui::Button("Force recompile materials (needed for some render option changes!)") || should_recompile) {
        for(auto material : assetm->get_all<Material>()) {
            material->capture_pipeline = nullptr;
            material->skinned_pipeline = nullptr;
            material->static_pipeline = nullptr;
        }
    }
}

void draw_debug_ui() {
    if(ImGui::Begin("General"))
        draw_general();
    
    ImGui::End();
    
    if(ImGui::Begin("Lighting"))
        draw_lighting();
    
    ImGui::End();

    if(ImGui::Begin("Assets"))
        draw_asset();

    ImGui::End();
    
    if(ImGui::Begin("Rendering"))
        draw_renderer();
        
    ImGui::End();
}
