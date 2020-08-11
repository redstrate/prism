#include "cutsceneeditor.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <nlohmann/json.hpp>

#include "engine.hpp"
#include "imguipass.hpp"
#include "file.hpp"
#include "json_conversions.hpp"
#include "platform.hpp"
#include "screen.hpp"
#include "cutscene.hpp"

Shot* currentShot = nullptr;
AnimationChannel* currentChannel = nullptr;
PositionKeyFrame* currentFrame = nullptr;

std::string currentPath;

std::unique_ptr<Renderer> renderer;

void app_main(Engine* engine) {
	CommonEditor* editor = (CommonEditor*)engine->get_app();

	platform::open_window("Cutscene Editor",
                          {editor->getDefaultX(),
                            editor->getDefaultY(),
        static_cast<uint32_t>(editor->getDefaultWidth()),
        static_cast<uint32_t>(editor->getDefaultHeight())}, WindowFlags::Resizable);
}

CutsceneEditor::CutsceneEditor() : CommonEditor("CutsceneEditor") {}

void CutsceneEditor::drawUI() {
    if(!renderer) {
        renderer = std::make_unique<Renderer>(engine->get_gfx(), false);
        renderer->viewport_mode = true;
        
        renderer->resize_viewport({static_cast<uint32_t>(viewport_width), static_cast<uint32_t>(viewport_height)});
    }
        
    createDockArea();
    
    const ImGuiID editor_dockspace = ImGui::GetID("dockspace");

    ImGui::End();
        
    if(ImGui::DockBuilderGetNode(editor_dockspace) == nullptr) {
        const auto size = ImGui::GetMainViewport()->Size;
        
        ImGui::DockBuilderRemoveNode(editor_dockspace);
        ImGui::DockBuilderAddNode(editor_dockspace, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(editor_dockspace, size);
        
        ImGuiID outline_parent, view;
        ImGui::DockBuilderSplitNode(editor_dockspace, ImGuiDir_Left, 0.2f, &outline_parent, &view);
        
        ImGuiID outline, playback;
        ImGui::DockBuilderSplitNode(outline_parent, ImGuiDir_Down, 0.8f, &outline, &playback);
        
        ImGui::DockBuilderDockWindow("Outliner", outline);
        ImGui::DockBuilderDockWindow("Playback Settings", playback);

        ImGuiID viewport_parent, properties;
        ImGui::DockBuilderSplitNode(view, ImGuiDir_Left, 0.7f, &viewport_parent, &properties);
        
        ImGui::DockBuilderDockWindow("Properties", properties);

        ImGuiID lowerbar, viewport;
        ImGui::DockBuilderSplitNode(viewport_parent, ImGuiDir_Down, 0.3f, &lowerbar, &viewport);
        
        ImGui::DockBuilderDockWindow("Viewport", viewport);

        ImGui::DockBuilderDockWindow("Timeline", lowerbar);
        ImGui::DockBuilderDockWindow("Animation Editor", lowerbar);

        ImGui::DockBuilderFinish(editor_dockspace);
    }
    
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if(ImGui::MenuItem("New", "CTRL+N")) {
                engine->cutscene = std::make_unique<Cutscene>();
                
				platform::set_window_title(0, "Cutscene Editor");
            }

            if(ImGui::MenuItem("Open", "CTRL+O")) {
                platform::open_dialog(true, [this](std::string path){
					currentPath = path;
                    
                    engine->load_cutscene(path);

					platform::set_window_title(0, ("Cutscene Editor - " + path).c_str());

					addOpenedFile(path);
                });
            }

			const auto& recents = getOpenedFiles();
			if (ImGui::BeginMenu("Open Recent...", !recents.empty())) {
				for (auto& file : recents) {
					if (ImGui::MenuItem(file.c_str())) {
						currentPath = file;
                        
                        engine->load_cutscene(file);

						platform::set_window_title(0, ("Cutscene Editor - " + file).c_str());
					}
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Clear")) {
					clearOpenedFiles();
				}

				ImGui::EndMenu();
			}

            if(ImGui::MenuItem("Save", "CTRL+S")) {
				if (currentPath.empty()) {
					platform::save_dialog([](std::string path) {
						currentPath = path;
                        
                        engine->save_cutscene(path);
                        
						platform::set_window_title(0, ("Cutscene Editor - " + path).c_str());
					});
				}
				else {
                    engine->save_cutscene(currentPath);
                }
            }

			if (ImGui::MenuItem("Save as...", "CTRL+S")) {
				platform::save_dialog([](std::string path) {
					currentPath = path;
                    
                    engine->save_cutscene(path);

					platform::set_window_title(0, ("Cutscene Editor - " + path).c_str());
				});
			}

            ImGui::Separator();

            if(ImGui::MenuItem("Quit", "CTRL+Q"))
                engine->quit();

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
        }
        
        if(ImGui::BeginMenu("Add...")) {
            if (ImGui::MenuItem("Empty")) {
                auto new_obj = engine->get_scene()->add_object();
                engine->get_scene()->get(new_obj).name = "new object";
            }
            
            if(ImGui::MenuItem("Prefab")) {
                platform::open_dialog(false, [](std::string path) {
                    engine->add_prefab(*engine->get_scene(), path);
                });
            }
            
            ImGui::EndMenu();
        }

		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem("About")) {

			}

			ImGui::EndMenu();
		}

        ImGui::EndMainMenuBar();
    }
    
    if(ImGui::Begin("Outliner")) {
        drawOutline();
    }
    
    ImGui::End();

    if(ImGui::Begin("Properties")) {
        drawPropertyEditor();
    }
    
    ImGui::End();

    if(ImGui::Begin("Timeline")) {
        if(engine->cutscene != nullptr) {
            if(ImGui::Button("Add Shot")) {
                engine->cutscene->shots.push_back(Shot());
                currentShot = &engine->cutscene->shots.back();
            }
                        
            if(currentShot != nullptr) {
                ImGui::SameLine();

                if(ImGui::Button("Change scene")) {
                    platform::open_dialog(true, [](std::string path) {
                        currentShot->scene = engine->load_scene(path);
                    });
                }
                                
                ImGui::SameLine();
                
                ImGui::SetNextItemWidth(25.0f);
                
                ImGui::DragInt("Begin", &currentShot->begin, 1.0f, 0, 1000);
                
                ImGui::SameLine();
                
                ImGui::SetNextItemWidth(25.0f);

                ImGui::DragInt("Length", &currentShot->length, 1.0f, 0, 1000);
                
                ImGui::SameLine();
                
                if(ImGui::Button("Remove")) {
                    utility::erase(engine->cutscene->shots, *currentShot);
                    currentShot = nullptr;
                }
            }
            
            ImGui::Separator();
            
            const auto& draw_list = ImGui::GetWindowDrawList();
            const ImVec2 p = ImGui::GetCursorScreenPos();
            
            int i = 0;
            for(auto& shot : engine->cutscene->shots) {
                auto a = ImVec2(p.x + (5 * shot.begin), p.y);
                auto b = ImVec2(p.x + (5 * (shot.begin + shot.length)), p.y + 50);
                
                if(&shot == currentShot) {
                    draw_list->AddRectFilled(a, b, IM_COL32(0, 0, 100, 100));
                }
                
                draw_list->AddRect(a, b, IM_COL32_WHITE);
                
                std::string name = "Shot " + std::to_string(i);
                draw_list->AddText(ImVec2(a.x + 2, a.y + 2), IM_COL32_WHITE, name.c_str());
                
                if(ImGui::IsMouseClicked(0)) {
                    auto c = ImGui::GetMousePos();
                    if(c.x > a.x && c.x < b.x && c.y > a.y && c.y < b.y) {
                        if(currentShot == &shot) {
                            currentShot = nullptr;
                        } else {
                            currentShot = &shot;
                        }
                    }
                }
                
                i++;
            }
            
            draw_list->AddLine(ImVec2(p.x + (engine->current_cutscene_time * 5), p.y), ImVec2(p.x + (engine->current_cutscene_time * 5), p.y + 75), IM_COL32_WHITE);
        } else {
            ImGui::Text("No cutscene loaded.");
        }
    }
    
    ImGui::End();

    if(ImGui::Begin("Animation Editor")) {
        if(currentShot != nullptr && currentShot->scene != nullptr) {
            ImGui::BeginChild("animations", ImVec2(150, -1), true);
            
            if(ImGui::Button("Add Channel")) {
                currentShot->channels.push_back(AnimationChannel());
                currentChannel = &currentShot->channels.back();
            }
            
            int i = 0;
            for(auto& channel : currentShot->channels) {
                std::string name = "Channel " + std::to_string(i);
                if(ImGui::Selectable(name.c_str(), &channel == currentChannel))
                    currentChannel = &channel;
                                     
                i++;
            }
            
            if(currentShot->channels.empty())
                ImGui::TextDisabled("No channels in this shot.");
                
            ImGui::EndChildFrame();
            
            ImGui::SameLine();
            
            ImGui::BeginChild("editanim", ImVec2(-1, -1), false);
            
            if(currentChannel != nullptr) {
                if(ImGui::Button("Add Position Frame")) {
                    currentChannel->positions.push_back(PositionKeyFrame());
                    currentFrame = &currentChannel->positions.back();
                }
                
                ImGui::SameLine();
                
                ImGui::SetNextItemWidth(50.0f);
                                
                const char* preview_value = "None";
                if(currentChannel->target != NullObject && currentShot->scene->has<Data>(currentChannel->target))
                    preview_value = currentShot->scene->get(currentChannel->target).name.c_str();
                
                if(ImGui::BeginCombo("Target", preview_value)) {
                    for(auto& object : currentShot->scene->get_objects()) {
                        if(ImGui::Selectable(currentShot->scene->get(object).name.c_str())) {
                            currentChannel->target = object;
                            currentChannel->bone = nullptr;
                        }
                        
                        if(currentShot->scene->has<Renderable>(object) && !currentShot->scene->get<Renderable>(object).mesh->bones.empty()) {
                            ImGui::Indent();
                            
                            for(auto& bone : currentShot->scene->get<Renderable>(object).mesh->bones) {
                                if(ImGui::Selectable(bone.name.c_str())) {
                                    currentChannel->bone = &bone;
                                    currentChannel->target = NullObject;
                                }
                            }
                            
                            ImGui::Unindent();
                        }
                    }
                    
                    ImGui::EndCombo();
                }
                
                ImGui::SameLine();
                
                if(ImGui::Button("Remove Channel")) {
                    utility::erase(currentShot->channels, *currentChannel);
                    currentChannel = nullptr;
                }
                                
                if(currentFrame != nullptr) {
                    ImGui::SameLine();
                    
                    ImGui::SetNextItemWidth(10.0f);
                    
                    ImGui::DragFloat("FT", &currentFrame->time, 1.0f, 0.0f, 10000.0f);
                    
                    ImGui::SameLine();
                    
                    ImGui::SetNextItemWidth(50.0f);

                    ImGui::DragFloat3("FV", currentFrame->value.ptr());
                    
                    ImGui::SameLine();
                    
                    if(ImGui::Button("Remove Keyframe")) {
                        utility::erase(currentChannel->positions, *currentFrame);
                        currentFrame = nullptr;
                    }
                }
                
                if(currentChannel != nullptr) {
                    const auto& draw_list = ImGui::GetWindowDrawList();
                    const ImVec2 p = ImGui::GetCursorScreenPos();
                    
                    for(auto& keyframe : currentChannel->positions) {
                        ImVec2 c = ImVec2(p.x + (keyframe.time * 5), p.y);
                        
                        if(&keyframe == currentFrame)
                            draw_list->AddCircleFilled(c, 5.0f, IM_COL32(0, 0, 100, 255));
                        
                        draw_list->AddCircle(c, 5.0f, IM_COL32_WHITE);
                                            
                        if(ImGui::IsMouseClicked(0)) {
                             ImVec2 a = ImVec2(p.x + (keyframe.time * 5) - 5, p.y - 5);
                             ImVec2 b = ImVec2(p.x + (keyframe.time * 5) + 5, p.y + 5);
                             
                             auto c = ImGui::GetMousePos();
                             if(c.x > a.x && c.x < b.x && c.y > a.y && c.y < b.y) {
                                 if(currentFrame == &keyframe) {
                                     currentFrame = nullptr;
                                 } else {
                                     currentFrame = &keyframe;
                                 }
                             }
                         }
                    }
                    
                    ImVec2 shot_start = ImVec2(p.x + (currentShot->begin * 5), p.y + 10.0f);
                    ImVec2 shot_end = ImVec2(p.x + (currentShot->length * 5), p.y + 10.0f);

                    draw_list->AddCircleFilled(shot_start, 2.5f, IM_COL32(255, 0, 0, 255));
                    draw_list->AddCircleFilled(shot_end, 2.5f, IM_COL32(255, 0, 0, 255));

                    draw_list->AddLine(ImVec2(p.x + ((engine->current_cutscene_time - currentShot->begin) * 5), p.y), ImVec2(p.x + ((engine->current_cutscene_time - currentShot->begin) * 5), p.y + 75), IM_COL32_WHITE);
                }
            } else {
                ImGui::TextDisabled("No animation selected.");
            }
            
            ImGui::EndChild();
        } else {
            ImGui::TextDisabled("No shot selected.");
        }
    }
    
    ImGui::End();

    if(ImGui::Begin("Playback Settings")) {
        if(ImGui::Button(engine->play_cutscene ? "Stop" : "Play")) {
            engine->play_cutscene = !engine->play_cutscene;
        }
        
        ImGui::DragFloat("Time", &engine->current_cutscene_time, 1.0f, 0.0f, 1000.0f);
    }
    
    ImGui::End();

    if(ImGui::Begin("Viewport")) {
        drawViewport(*renderer);
    }
    
    ImGui::End();
    
    if(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
        engine->current_cutscene_time -= 1.0f;
    
    if(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
        engine->current_cutscene_time += 1.0f;
    
    renderer->render(engine->get_scene(), -1);
}
