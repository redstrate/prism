#include "materialeditor.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>

#include "engine.hpp"

bool MaterialEditor::has_menubar() const {
    return true;
}

std::string MaterialEditor::get_title() const {
    return path.empty() ? "New Material" : get_filename(path);
}

Scene* MaterialEditor::get_scene() const {
    return scene;
}

ImRect get_window_rect(MaterialNode& node) {
    auto pos = ImGui::GetCursorScreenPos();
    
    return ImRect(ImVec2(pos.x + node.x, pos.y + node.y), ImVec2(pos.x + node.x + node.width, pos.y + node.y + node.height));
}

ImRect get_input_rect(MaterialNode& node, int pos) {
    auto rect = get_window_rect(node);
    
    return ImRect(ImVec2(rect.Min.x, rect.Min.y + 20.0f + (pos * 20.0f)), ImVec2(rect.Min.x + 20.0f, rect.Min.y + 40.0f + (pos * 20.0f)));
}

ImRect get_output_rect(MaterialNode& node, int pos) {
    auto rect = get_window_rect(node);
    
    return ImRect(ImVec2(rect.Max.x - 20.0f, rect.Min.y + 20.0f + (pos * 20.0f)), ImVec2(rect.Max.x, rect.Min.y + 40.0f + (pos * 20.0f)));
}

ImRect get_input_rect(MaterialNode& node, MaterialConnector* connector) {
    const auto vec = node.inputs;
    const auto pos = find(vec.begin(), vec.end(), *connector) - vec.begin();
    
    return get_input_rect(node, pos);
}

ImRect get_output_rect(MaterialNode& node, MaterialConnector* connector) {
    const auto vec = node.outputs;
    const auto pos = find(vec.begin(), vec.end(), *connector) - vec.begin();
    
    return get_output_rect(node, pos);
}

void MaterialEditor::setup_windows(ImGuiID dockspace) {
    ImGuiID dock_main_id = dockspace;

    ImGuiID dock_id_left, dock_id_right;
    ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.40f, &dock_id_left, &dock_id_right);
    
    ImGui::DockBuilderDockWindow(get_window_name("Viewport").c_str(), dock_id_left);
    ImGui::DockBuilderDockWindow(get_window_name("Node Editor").c_str(), dock_id_right);
}

void MaterialEditor::setup_material() {
    renderable->materials.push_back(material);
}

void recompile(Material* material) {
    material->static_pipeline = nullptr;
    material->skinned_pipeline = nullptr;
}

void MaterialEditor::draw(CommonEditor* editor) {
    if(!material)
        return;
    
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if(ImGui::MenuItem("Save", "CTRL+S")) {
                if (path.empty()) {
                    platform::save_dialog([this](std::string path) {
                        this->path = path;
                        
                        save_material(*material, path);
                    });
                } else {
                    //save_material(*material, file::get_file_path(FileDomain::GameData, path));
                }
            }
            
            if (ImGui::MenuItem("Save as...", "CTRL+S")) {
                platform::save_dialog([this](std::string path) {
                    this->path = path;
                    
                    save_material(*material, path);
                });
            }
            
            ImGui::Separator();
            
            if(ImGui::MenuItem("Close"))
                wants_to_close = true;
            
            ImGui::EndMenu();
        }
        
        if(ImGui::BeginMenu("Add Node")) {
            if(ImGui::MenuItem("Material Output"))
                material->nodes.push_back(std::make_unique<MaterialOutput>());
            
            if(ImGui::MenuItem("Texture"))
                material->nodes.push_back(std::make_unique<TextureNode>());
            
            if(ImGui::MenuItem("Float Constant"))
                material->nodes.push_back(std::make_unique<FloatConstant>());
            
            if(ImGui::MenuItem("Vec3 Constant"))
                material->nodes.push_back(std::make_unique<Vector3Constant>());
            
            ImGui::EndMenu();
        }
        
        if(ImGui::Button("Recompile")) {
            recompile(*material);
        }
        
        ImGui::EndMenuBar();
    }
    
    Scene* scene = engine->get_scene();
    
    if(scene != nullptr) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if(begin("Viewport"))
            editor->drawViewport(*renderer);
        
        ImGui::End();
        
        ImGui::PopStyleVar();
    }
    
    if(begin("Node Editor")) {
        const auto draw_list = ImGui::GetWindowDrawList();
        const auto window_pos = ImGui::GetCursorScreenPos();
        
        static bool dragging = false;
        static MaterialNode* dragging_node = nullptr;
        
        static bool dragging_connector = false;
        static MaterialConnector* drag_connector = nullptr;
        static int drag_index = 0;
        
        std::vector<MaterialNode*> deferred_deletions;
        
        for(auto& node : material->nodes) {
            const auto rect = get_window_rect(*node);
            
            ImGui::PushID(&node);
            
            bool changed = false;

            draw_list->AddRectFilled(rect.Min, rect.Max, IM_COL32(50, 50, 50, 255));
            draw_list->AddRect(rect.Min, rect.Max, IM_COL32(255, 255, 255, 255), 5.0f);
            
            draw_list->AddText(rect.Min, IM_COL32(255, 255, 255, 255), node->get_name());
            
            for(auto [i, input] : utility::enumerate(node->inputs)) {
                auto input_rect = get_input_rect(*node, i);
                
                draw_list->AddText(input_rect.Min, IM_COL32(255, 255, 255, 255), input.name.c_str());
                
                ImGui::ItemAdd(input_rect, ImGui::GetID(node.get()));
                
                bool might_connect = false;
                
                if(dragging_connector && ImGui::IsItemHovered()) {
                    might_connect = true;
                }
                
                if(dragging_connector && ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    input.connected_connector = drag_connector;
                    input.connected_node = dragging_node;
                    input.connected_index = drag_index;
                    
                    drag_connector->connected_connector = &input;
                    drag_connector->connected_node = node.get();
                    drag_connector->connected_index = drag_index;
                    
                    changed = true;
                }
                
                draw_list->AddCircleFilled(input_rect.GetCenter(), 5.0, IM_COL32(255, 0, 0, 255));

                if(ImGui::IsItemClicked() && !dragging && !dragging_connector) {
                    dragging_connector = true;
                    drag_connector = &input;
                    drag_index = i;
                }
                
                if(input.connected_connector != nullptr) {
                    auto output_rect = get_output_rect(*input.connected_node, input.connected_index);
                    draw_list->AddLine(input_rect.GetCenter(), output_rect.GetCenter(), IM_COL32(255, 255, 255, 255));
                }
            }
            
            for(auto [i, output] : utility::enumerate(node->outputs)) {
                auto output_rect = get_output_rect(*node, i);
                
                draw_list->AddText(output_rect.Min, IM_COL32(255, 255, 255, 255), output.name.c_str());
                
                draw_list->AddCircleFilled(output_rect.GetCenter(), 5.0, IM_COL32(255, 0, 0, 255));
                
                ImGui::ItemAdd(output_rect, ImGui::GetID(node.get()));
                
                if(ImGui::IsItemClicked() && !dragging && !dragging_connector) {
                    dragging_connector = true;
                    drag_connector = &output;
                    drag_index = i;
                    dragging_node = node.get();
                }
            }
            
            ImGui::ItemAdd(rect, ImGui::GetID(node.get()));
            
            if(ImGui::IsItemClicked() && !dragging_connector) {
                dragging = true;
                dragging_node = node.get();
            }
            
            if(dragging && node.get() == dragging_node) {
                node->x += ImGui::GetIO().MouseDelta.x;
                node->y += ImGui::GetIO().MouseDelta.y;
            }
            
            if(ImGui::IsMouseReleased(ImGuiMouseButton_Left) && (dragging || dragging_connector)) {
                dragging = false;
                dragging_connector = false;
            }
            
            if(ImGui::BeginPopupContextItem()) {
                if(ImGui::Selectable("Delete"))
                    deferred_deletions.push_back(node.get());
                
                ImGui::EndPopup();
            }
            
            if(dragging_connector && node.get() == dragging_node) {
                const auto output_rect = get_output_rect(*node, drag_index);
                draw_list->AddLine(output_rect.GetCenter(), ImGui::GetIO().MousePos, IM_COL32(255, 255, 255, 255));
            }
            
            ImGui::SetCursorScreenPos(ImVec2(rect.Min.x + 5.0, rect.Min.y + 20.0));
            ImGui::SetNextItemWidth(node->width - 40.0);
            
            for(auto& property : node->properties) {
                switch(property.type) {
                    case DataType::Vector3:
                        changed |= ImGui::ColorEdit3(property.name.c_str(), property.value.ptr());
                        break;
                    case DataType::Float:
                        changed |= ImGui::DragFloat(property.name.c_str(), &property.float_value);
                        break;
                    case DataType::AssetTexture:
                        changed |= editor->edit_asset(property.name.c_str(), property.value_tex);
                        break;
                }
            }
            
            for(auto& node : deferred_deletions) {
                material->delete_node(node);
                changed = true;
            }
            
            if(changed)
                recompile(*material);
            
            ImGui::PopID();
            
            ImGui::SetCursorScreenPos(window_pos);
        }
    }
    
    ImGui::End();
}
