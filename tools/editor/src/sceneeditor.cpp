#include "sceneeditor.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>

#include "engine.hpp"

bool SceneEditor::has_menubar() const {
    return true;
}

std::string SceneEditor::get_title() const {
    return path.empty() ? "New Scene" : get_filename(path);
}

Scene* SceneEditor::get_scene() const {
    return scene;
}

void SceneEditor::setup_windows(ImGuiID dockspace) {
    ImGuiID dock_main_id = dockspace;
    
    ImGuiID dock_id_top, dock_id_bottom;
    ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.70f, &dock_id_top, &dock_id_bottom);
    
    ImGui::DockBuilderDockWindow(get_window_name("Assets").c_str(), dock_id_bottom);
    
    ImGuiID dock_id_left, dock_id_right;
    ImGui::DockBuilderSplitNode(dock_id_top, ImGuiDir_Left, 0.70f, &dock_id_left, &dock_id_right);

    ImGui::DockBuilderDockWindow(get_window_name("Properties").c_str(), dock_id_right);
    ImGui::DockBuilderDockWindow(get_window_name("Scene Settings").c_str(), dock_id_right);

    ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Left, 0.20f, &dock_id_left, &dock_id_right);
    ImGui::DockBuilderDockWindow(get_window_name("Outliner").c_str(), dock_id_left);
    
    ImGui::DockBuilderDockWindow(get_window_name("Viewport").c_str(), dock_id_right);
    ImGui::DockBuilderDockWindow(get_window_name("Console").c_str(), dock_id_right);
}

void SceneEditor::draw(CommonEditor* editor) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if(ImGui::MenuItem("Save", "CTRL+S")) {
                if (path.empty()) {
                    platform::save_dialog([this](std::string path) {
                        this->path = path;

                        engine->save_scene(path);
                    });
                } else {
                     engine->save_scene(path);
                }
            }

            if(ImGui::MenuItem("Save as...", "CTRL+S")) {
                platform::save_dialog([this](std::string path) {
                    this->path = path;

                    engine->save_scene(path);
                });
            }
            
            ImGui::Separator();
            
            if(ImGui::MenuItem("Close"))
                wants_to_close = true;

            ImGui::EndMenu();
        }
                
        if (ImGui::BeginMenu("Edit")) {
            std::string previous_stack_name = "Undo";
            bool undo_available = false;
            
            std::string next_stack_name = "Redo";
            bool redo_available = false;
            
            if (auto command = undo_stack.get_last_command()) {
                undo_available = true;
                previous_stack_name += " " + command->get_name();
            }
            
            if (auto command = undo_stack.get_next_command()) {
                redo_available = true;
                next_stack_name += " " + command->get_name();
            }
            
            if (ImGui::MenuItem(previous_stack_name.c_str(), "CTRL+Z", false, undo_available))
                undo_stack.undo();

            if (ImGui::MenuItem(next_stack_name.c_str(), "CTRL+Y", false, redo_available))
                undo_stack.redo();
   
            ImGui::EndMenu();
        }
        
        if(ImGui::BeginMenu("Add...")) {
            if (ImGui::MenuItem("Empty")) {
                auto new_obj = engine->get_scene()->add_object();
                engine->get_scene()->get(new_obj).name = "new object";
                
                auto& command = undo_stack.new_command<AddObjectCommand>();
                command.id = new_obj;
                command.name = engine->get_scene()->get(new_obj).name;
            }
            
            if(ImGui::MenuItem("Prefab")) {
                platform::open_dialog(false, [](std::string path) {
                    engine->add_prefab(*engine->get_scene(), path);
                });
            }
            
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Outliner", nullptr, &showOutliner);
            ImGui::MenuItem("Properties", nullptr, &showProperties);
            ImGui::MenuItem("Scene Settings", nullptr, &showSceneSettings);
            ImGui::MenuItem("Viewport", nullptr, &showViewport);
            ImGui::MenuItem("Undo Stack", nullptr, &showUndoStack);
            ImGui::MenuItem("Assets", nullptr, &showAssets);

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    Scene* scene = engine->get_scene();
    if(scene != nullptr) {
        if(showSceneSettings) {
            if(begin("Scene Settings", &showSceneSettings)) {
                ImGui::InputText("Script Path", &scene->script_path);
            }

            ImGui::End();
        }

        if(showOutliner) {
            if(begin("Outliner", &showOutliner))
                editor->drawOutline();

            ImGui::End();
        }

        if(showProperties) {
            if(begin("Properties", &showProperties))
                editor->drawPropertyEditor();

            ImGui::End();
        }

        if(showViewport) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            if(begin("Viewport", &showViewport))
                editor->drawViewport(*renderer);

            ImGui::PopStyleVar();

            ImGui::End();
        }
        
        if(showUndoStack) {
            if(begin("Undo Stack", &showUndoStack)) {
                for(auto [i, command] : utility::enumerate(undo_stack.command_stack)) {
                    std::string name = command->get_name();
                    if(i == undo_stack.stack_position)
                        name = "-> " + name;
                    
                    ImGui::Selectable(name.c_str());
                }
            }
            
            ImGui::End();
        }
        
        if(showAssets) {
            if(begin("Assets", &showAssets))
                editor->drawAssets();
                    
            ImGui::End();
        }
        
        if(begin("Console"))
            editor->drawConsole();
        
        ImGui::End();
    }
}
