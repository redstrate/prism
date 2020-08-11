#include "uieditor.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>

#include "engine.hpp"
#include "imgui_utility.hpp"

UIEditor::UIEditor() : Editor() {
    screen = new ui::Screen();
    screen->extent.width = 1280;
    screen->extent.height = 720;
}

bool UIEditor::has_menubar() const {
    return true;
}

std::string UIEditor::get_title() const {
    return path.empty() ? "New UI" : get_filename(path);
}

Scene* UIEditor::get_scene() const {
    return nullptr;
}

void UIEditor::setup_windows(ImGuiID dockspace) {
    ImGuiID dock_main_id = dockspace;

    ImGuiID dock_id_left, dock_id_right;
    ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.60f, &dock_id_left, &dock_id_right);

    ImGui::DockBuilderDockWindow(get_window_name("Properties").c_str(), dock_id_right);

    ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Left, 0.30f, &dock_id_left, &dock_id_right);
    ImGui::DockBuilderDockWindow(get_window_name("Outliner").c_str(), dock_id_left);
    ImGui::DockBuilderDockWindow(get_window_name("Preview").c_str(), dock_id_right);
}

void UIEditor::edit_metric(const char* label, UIElement::Metrics::Metric* metric) {
    if(ImGui::DragInt(label, &metric->value, 1.0f, 0, 0, metric->type == MetricType::Absolute ? "%d" : "%d%%"))
        screen->calculate_sizes();

    ImGui::SameLine();

    ImGui::PushID(label);

    if(ImGui::ComboEnum("T", &metric->type))
        screen->calculate_sizes();

    ImGui::PopID();
}

std::string UIEditor::save_metric(UIElement::Metrics::Metric metric) {
    return std::to_string(metric.value) + (metric.type == MetricType::Absolute ? "px" : "%");
}

void UIEditor::save(std::string path) {
    nlohmann::json j;

    for (auto& element : screen->elements) {
        nlohmann::json e;
        e["id"] = element.id;
        e["text"] = element.text;
        e["visible"] = element.visible;
        e["wrapText"] = element.wrap_text;

        nlohmann::json m;
        m["x"] = save_metric(element.metrics.x);
        m["y"] = save_metric(element.metrics.y);
        m["width"] = save_metric(element.metrics.width);
        m["height"] = save_metric(element.metrics.height);

        e["metrics"] = m;

        if(element.text_location == UIElement::TextLocation::Center) {
            e["textLocation"] = "center";
        } else {
            e["textLocation"] = "topLeft";
        }

        j["elements"].push_back(e);
    }

    std::ofstream out(path);
    out << std::setw(4) << j;
}

void UIEditor::draw(CommonEditor*) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if(ImGui::MenuItem("Save", "CTRL+S")) {
                if (path.empty()) {
                    platform::save_dialog([this](std::string path) {
                        this->path = path;

                        save(path);
                    });
                } else {
                    save(path);
                }
            }

            if (ImGui::MenuItem("Save as...", "CTRL+S")) {
                platform::save_dialog([this](std::string path) {
                    this->path = path;

                    save(path);
                });
            }
            
            ImGui::Separator();
            
            if(ImGui::MenuItem("Close"))
                wants_to_close = true;
            
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    if(begin("Outliner")) {
        if(ImGui::Button("Add Element")) {
            screen->elements.push_back(UIElement());
            current_element = &screen->elements.back();
        }

        for(auto& element : screen->elements) {
            if(ImGui::Selectable(element.id.c_str()))
                current_element = &element;
        }
    }

    ImGui::End();

    if(begin("Properties")) {
        if(current_element != nullptr) {
            ImGui::InputText("ID", &current_element->id);

            edit_metric("X", &current_element->metrics.x);
            edit_metric("Y", &current_element->metrics.y);
            edit_metric("Width", &current_element->metrics.width);
            edit_metric("Height", &current_element->metrics.height);

            ImGui::InputText("Text", &current_element->text);

            ImGui::ColorEdit4("Background color", current_element->background.color.v);

            ImGui::Checkbox("Visible", &current_element->visible);
            ImGui::Checkbox("Wrap text", &current_element->wrap_text);

            bool setCenter = current_element->text_location == UIElement::TextLocation::Center;
            ImGui::Checkbox("Center Text", &setCenter);

            current_element->text_location = setCenter ? UIElement::TextLocation::Center : UIElement::TextLocation::TopLeft;

            if(ImGui::Button("Delete")) {
                utility::erase(screen->elements, *current_element);
            }

            if(ImGui::Button("Duplicate")) {
                UIElement element = *current_element;
                element.id += " (duplicate)";

                screen->elements.push_back(element);
                current_element = &screen->elements.back();
            }
        }
    }

    ImGui::End();

    if(begin("Preview")) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        static int width = 1280, height = 720;
        static int scale = 3;

        bool c = ImGui::InputInt("Width", &width);
        c |= ImGui::InputInt("Height", &height);
        c |= ImGui::InputInt("Scale", &scale);

        if(c) {
            screen->extent.width = width;
            screen->extent.height = height;
            screen->calculate_sizes();
        }

        const ImVec2 p = ImGui::GetCursorScreenPos();

        draw_list->AddRect(ImVec2(p.x, p.y), ImVec2(p.x + (width / scale), p.y + (height / scale)), IM_COL32_WHITE);

        for(auto& element : screen->elements) {
            if(element.absolute_width > 0 && element.absolute_height > 0) {
                draw_list->AddRect(
                                   ImVec2(p.x + (element.absolute_x / scale), p.y + (element.absolute_y / scale)),
                                   ImVec2(p.x + ((element.absolute_x + element.absolute_width) / scale), p.y + ((element.absolute_y + element.absolute_height) / scale)),
                                   IM_COL32_WHITE
                                   );
            }

            if(!element.text.empty()) {
                if(element.text_location == UIElement::TextLocation::Center) {
                    auto textSize = ImGui::CalcTextSize(element.text.c_str());

                    draw_list->AddText(ImVec2(p.x + (element.absolute_x + (element.absolute_width / 2)) / scale - (textSize.x / 2), p.y + (element.absolute_y + (element.absolute_height / 2)) / scale - (textSize.y / 2)), IM_COL32_WHITE, element.text.c_str());
                } else {
                    draw_list->AddText(ImVec2(p.x + (element.absolute_x / scale), p.y + (element.absolute_y / scale)), IM_COL32_WHITE, element.text.c_str());
                }
            }
        }
    }

    ImGui::End();
}
