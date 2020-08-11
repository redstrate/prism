#include "prefabeditor.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>

#include "engine.hpp"

bool PrefabEditor::has_menubar() const {
    return true;
}

std::string PrefabEditor::get_title() const {
    return path.empty() ? "New Prefab" : get_filename(path);
}

Scene* PrefabEditor::get_scene() const {
    return scene;
}

void PrefabEditor::setup_windows(ImGuiID dockspace) {
    ImGuiID dock_main_id = dockspace;

    ImGuiID dock_id_left, dock_id_right;
    ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.60f, &dock_id_left, &dock_id_right);

    ImGui::DockBuilderDockWindow(get_window_name("Properties").c_str(), dock_id_right);

    ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Left, 0.30f, &dock_id_left, &dock_id_right);
    ImGui::DockBuilderDockWindow(get_window_name("Outliner").c_str(), dock_id_left);
    ImGui::DockBuilderDockWindow(get_window_name("Viewport").c_str(), dock_id_right);
}

void PrefabEditor::draw(CommonEditor* editor) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if(ImGui::MenuItem("Save", "CTRL+S")) {
                if (path.empty()) {
                    platform::save_dialog([this](std::string path) {
                        this->path = path;

                        engine->save_prefab(root_object, path);
                    });
                } else {
                    engine->save_prefab(root_object, path);
                }
            }

            if (ImGui::MenuItem("Save as...", "CTRL+S")) {
                platform::save_dialog([this](std::string path) {
                    this->path = path;

                    engine->save_prefab(root_object, path);
                });
            }
            
            ImGui::Separator();
            
            if(ImGui::MenuItem("Close"))
                wants_to_close = true;
            
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    Scene* scene = engine->get_scene();

    if(scene != nullptr) {
        auto window_class = get_window_class();

        if(showOutliner) {
            ImGui::SetNextWindowClass(&window_class);

            if(begin("Outliner", &showOutliner))
                editor->drawOutline();

            ImGui::End();
        }

        if (showProperties) {
            ImGui::SetNextWindowClass(&window_class);

            if (begin("Properties", &showProperties))
                editor->drawPropertyEditor();

            ImGui::End();
        }

        ImGui::SetNextWindowClass(&window_class);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if(begin("Viewport"))
            editor->drawViewport(*renderer);

        ImGui::End();

        ImGui::PopStyleVar();
    }
}
