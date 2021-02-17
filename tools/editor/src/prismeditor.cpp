#include "prismeditor.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include <nlohmann/json.hpp>

#include "engine.hpp"
#include "imguipass.hpp"
#include "file.hpp"
#include "json_conversions.hpp"
#include "platform.hpp"
#include "string_utils.hpp"
#include "screen.hpp"
#include "sceneeditor.hpp"
#include "materialeditor.hpp"
#include "prefabeditor.hpp"
#include "uieditor.hpp"
#include "log.hpp"

std::string get_filename(const std::string path) {
    return path.substr(path.find_last_of("/") + 1, path.length());
}

std::vector<Editor*> editors;

void app_main(Engine* engine) {
	CommonEditor* editor = (CommonEditor*)engine->get_app();

	platform::open_window("Prism Editor",
                          {editor->getDefaultX(),
                          editor->getDefaultY(),
        static_cast<uint32_t>(editor->getDefaultWidth()),
        static_cast<uint32_t>(editor->getDefaultHeight())},
                          WindowFlags::Resizable);

    engine->update_physics = false;
}

PrismEditor::PrismEditor() : CommonEditor("PrismEditor") {
    
}

void prepScene() {
    auto scene = engine->get_scene();

    auto camera = scene->add_object();
    scene->get(camera).name = "editor camera";
    scene->get(camera).editor_object = true;

    scene->add<Camera>(camera);
    
    camera_look_at(*scene, camera, Vector3(0, 2, 3), Vector3(0));
}

void prepThreePointLighting() {
    auto scene = engine->get_scene();
    
    auto probe = scene->add_object();
    scene->add<EnvironmentProbe>(probe).is_sized = false;

    auto sun_light = scene->add_object();
    scene->get(sun_light).name = "sun light";
    scene->get(sun_light).editor_object = true;

    scene->get<Transform>(sun_light).position = Vector3(15);
    scene->add<Light>(sun_light).type = Light::Type::Sun;
    scene->get<Light>(sun_light).power = 5.0f;

    scene->reset_shadows();
    scene->reset_environment();
}

void prepPrefabScene() {
    auto scene = engine->get_scene();

    auto plane = scene->add_object();
    scene->get(plane).name = "plane";
    scene->get(plane).editor_object = true;

    scene->get<Transform>(plane).position = Vector3(0, -1, 0);
    scene->get<Transform>(plane).scale = Vector3(50);

    scene->add<Renderable>(plane).mesh = assetm->get<Mesh>(file::app_domain / "models/plane.model");

    prepThreePointLighting();
}

Renderable* prepMaterialScene() {
    auto scene = engine->get_scene();
    
    auto plane = scene->add_object();
    scene->get(plane).name = "plane";
    scene->get(plane).editor_object = true;

    scene->get<Transform>(plane).position = Vector3(0, -1, 0);
    scene->get<Transform>(plane).scale = Vector3(50);

    scene->add<Renderable>(plane).mesh = assetm->get<Mesh>(file::app_domain / "models/plane.model");
    scene->get<Renderable>(plane).materials.push_back(assetm->get<Material>(file::app_domain / "materials/Material.material"));
    
    auto sphere = scene->add_object();
    scene->get(sphere).name = "sphere";
    scene->get(sphere).editor_object = true;

    scene->get<Transform>(sphere).rotation = euler_to_quat(Vector3(radians(90.0f), 0, 0));
    
    scene->add<Renderable>(sphere).mesh = assetm->get<Mesh>(file::app_domain / "models/sphere.model");

    prepThreePointLighting();
    
    return &scene->get<Renderable>(sphere);
}

struct OpenAssetRequest {
    OpenAssetRequest(std::string p, bool is_r) : path(p), is_relative(is_r) {}
    
    std::string path;
    bool is_relative;
};

std::vector<OpenAssetRequest> open_requests;

void PrismEditor::setup_editor(Editor* editor) {

}

void PrismEditor::open_asset(const file::Path path) {
    if(path.extension() == ".prefab") {
        PrefabEditor* editor = new PrefabEditor();
        editor->path = path.string();
        setup_editor(editor);

        engine->create_empty_scene();
        editor->root_object = engine->add_prefab(*engine->get_scene(), path);
        prepScene();
        prepPrefabScene();

        editor->scene = engine->get_scene();

        editors.push_back(editor);
    } else if(path.extension() == ".scene") {
        SceneEditor* editor = new SceneEditor();
        editor->path = path.string();
        setup_editor(editor);
  
        editor->scene = engine->load_scene(path);
        prepScene();

        editors.push_back(editor);
    } else if(path.extension() == ".material") {
        MaterialEditor* editor = new MaterialEditor();
        editor->path = path.string();
        setup_editor(editor);
        
        engine->create_empty_scene();
        prepScene();
        editor->renderable = prepMaterialScene();

        editor->material = assetm->get<Material>(path);
        
        editor->setup_material();

        editor->scene = engine->get_scene();

        editors.push_back(editor);
    } else if(path.extension() == ".json") {
        UIEditor* editor = new UIEditor();
        editor->path = path.string();
        setup_editor(editor);
        
        editor->screen = engine->load_screen(path);
        editor->screen->calculate_sizes();

        editors.push_back(editor);
    }
}

void PrismEditor::renderEditor(GFXCommandBuffer* command_buffer) {
    for(auto [id, render_target] : viewport_render_targets) {
        engine->get_renderer()->render(command_buffer, render_target.scene, *render_target.target, -1);
    }
}

void PrismEditor::drawUI() {
    createDockArea();

    auto dock_id = ImGui::GetID("dockspace");

    ImGui::End();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if(ImGui::BeginMenu("New")) {
                if(ImGui::MenuItem("Scene")) {
                    SceneEditor* editor = new SceneEditor();
                    editor->modified = true;
                    setup_editor(editor);
                    
                    engine->create_empty_scene();
                    editor->scene = engine->get_scene();
                    prepScene();

                    editors.push_back(editor);
                }
                
                if(ImGui::MenuItem("Prefab")) {
                    PrefabEditor* editor = new PrefabEditor();
                    editor->modified = true;
                    setup_editor(editor);
                    
                    engine->create_empty_scene();
                    editor->scene = engine->get_scene();
                    prepScene();

                    editors.push_back(editor);
                }
                
                if(ImGui::MenuItem("Material")) {
                    MaterialEditor* editor = new MaterialEditor();
                    editor->modified = true;
                    setup_editor(editor);
                    
                    engine->create_empty_scene();
                    prepScene();
                    editor->material = assetm->add<Material>();
                    editor->renderable = prepMaterialScene();
                    editor->scene = engine->get_scene();
                    
                    editor->setup_material();

                    editors.push_back(editor);
                }

                if(ImGui::MenuItem("UI")) {
                    UIEditor* editor = new UIEditor();
                    editor->modified = true;
                    setup_editor(editor);
                    
                    editors.push_back(editor);
                }

                ImGui::EndMenu();
            }

            if(ImGui::MenuItem("Open", "CTRL+O")) {
                platform::open_dialog(true, [this](std::string path) {
                    open_requests.emplace_back(path, false);

					addOpenedFile(path);
                });
            }

			const auto& recents = getOpenedFiles();
			if (ImGui::BeginMenu("Open Recent...", !recents.empty())) {
				for (auto& file : recents) {
					if (ImGui::MenuItem(file.c_str()))
                        open_requests.emplace_back(file, false);
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Clear"))
					clearOpenedFiles();

				ImGui::EndMenu();
            }

            ImGui::Separator();

            if(ImGui::MenuItem("Quit", "CTRL+Q"))
                engine->quit();

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    for(auto& editor : editors) {
        if(!editor->has_been_docked) {
            ImGui::DockBuilderDockWindow(editor->get_window_title().c_str(), dock_id);
            ImGui::DockBuilderFinish(dock_id);

            editor->has_been_docked = true;
        }
        
        const ImGuiID editor_dockspace = ImGui::GetID(editor);
        
        if(ImGui::DockBuilderGetNode(editor_dockspace) == nullptr) {
             const auto size = ImGui::GetMainViewport()->Size;

             ImGui::DockBuilderRemoveNode(editor_dockspace);
             ImGui::DockBuilderAddNode(editor_dockspace, ImGuiDockNodeFlags_DockSpace);
             ImGui::DockBuilderSetNodeSize(editor_dockspace, size);

             editor->setup_windows(editor_dockspace);

             ImGui::DockBuilderFinish(editor_dockspace);
        }
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        
        int window_flags = 0;
        if(editor->has_menubar())
            window_flags |= ImGuiWindowFlags_MenuBar;
        
        if(editor->modified)
            window_flags |= ImGuiWindowFlags_UnsavedDocument;
        
        bool should_draw = ImGui::Begin(editor->get_window_title().c_str(), nullptr, window_flags);
        
        ImGui::PopStyleVar();
        
        ImGui::DockSpace(editor_dockspace, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        
        if(should_draw) {
            /*debugPass = editor->debug_pass;
            if(debugPass != nullptr)
                debugPass->selected_object = selected_object;*/
            
            engine->set_current_scene(editor->get_scene());
            set_undo_stack(&editor->undo_stack);
            editor->draw(this);
        }

        ImGui::End();
    }
    
    utility::erase_if(editors, [](Editor* editor) {
        return editor->wants_to_close;
    });
}

void PrismEditor::object_selected(Object object) {
    selected_object = object;
}

void PrismEditor::asset_selected(std::filesystem::path path, AssetType) {
    open_requests.emplace_back(path.string(), true);
}

void PrismEditor::updateEditor(float) {
    for(auto [path, is_relative] : open_requests)
        open_asset(path);
    
    open_requests.clear();
}
