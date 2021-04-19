#pragma once

#include <string>
#include <vector>

#include "app.hpp"
#include "utility.hpp"
#include <imgui.h>
#include "math.hpp"
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include "platform.hpp"
#include "file.hpp"
#include "object.hpp"
#include "undostack.hpp"
#include "components.hpp"
#include "engine.hpp"
#include "debugpass.hpp"
#include "assertions.hpp"
#include "log.hpp"
#include "asset.hpp"
#include "scene.hpp"
#include "renderer.hpp"

class TransformCommand : public Command {
public:
    Object transformed;
    
    Transform old_transform, new_transform;
    
    std::string fetch_name() override {
        return "Transform " + engine->get_scene()->get(transformed).name;
    }
    
    void undo() override {
        engine->get_scene()->get<Transform>(transformed) = old_transform;
    }
    
    void execute() override {
        engine->get_scene()->get<Transform>(transformed) = new_transform;
    }
};

class RenameCommand : public Command {
public:
    Object object;
    
    std::string old_name, new_name;
    
    std::string fetch_name() override {
        return "Rename " + old_name + " to " + new_name;
    }
    
    void undo() override {
        engine->get_scene()->get(object).name = old_name;
    }
    
    void execute() override {
        engine->get_scene()->get(object).name = new_name;
    }
};

class ChangeParentCommand : public Command {
public:
    Object object;
    
    Object old_parent;
    Object new_parent;
    
    std::string fetch_name() override {
        return "Change parent of " + engine->get_scene()->get(object).name + " to " + engine->get_scene()->get(new_parent).name;
    }
    
    void undo() override {
        engine->get_scene()->get(object).parent = old_parent;
    }
    
    void execute() override {
        engine->get_scene()->get(object).parent = new_parent;
    }
};

enum class AssetType {
    Unknown,
    Mesh,
    Texture,
    Material
};

template<typename T>
AssetType get_asset_type() {
    if constexpr(std::is_same<T, Mesh>::value) {
        return AssetType::Mesh;
    } else if constexpr(std::is_same<T, Material>::value) {
        return AssetType::Material;
    } else if constexpr(std::is_same<T, Texture>::value) {
        return AssetType::Texture;
    }
    
    return AssetType::Unknown;
}

constexpr int thumbnail_resolution = 256;

class CommonEditor : public App {
public:
	CommonEditor(std::string id);

	void initialize_render() override;

	void prepare_quit() override;
    bool should_quit() override;

	virtual void drawUI() {}

	void begin_frame() override;

    void update(float deltaTime) override;
    
    virtual void renderEditor([[maybe_unused]] GFXCommandBuffer* command_buffer) {}

    void render(GFXCommandBuffer* command_buffer) override {
        renderEditor(command_buffer);
    }

    virtual void updateEditor([[maybe_unused]] float deltaTime) {}

    virtual void object_selected([[maybe_unused]] Object object) {}
    virtual void asset_selected([[maybe_unused]] std::filesystem::path path, [[maybe_unused]] AssetType type) {}

    bool wants_no_scene_rendering() override { return true; }

    void createDockArea();
    void drawViewport(Scene* scene);
    void drawAssets();

	// options
	int getDefaultX() const;
	int getDefaultY() const;
	int getDefaultWidth() const;
	int getDefaultHeight() const;

	void addOpenedFile(std::string path);
	void clearOpenedFiles();
	std::vector<std::string> getOpenedFiles() const;

    void drawOutline();
    void drawPropertyEditor();
    void drawConsole();
    
    void set_undo_stack(UndoStack* stack);

    Object selected_object = NullObject;
    
    GFXTexture* get_material_preview(Material& material);
    GFXTexture* get_mesh_preview(Mesh& mesh);
    GFXTexture* get_texture_preview(Texture& texture);
    GFXTexture* generate_common_preview(Scene& scene, const Vector3 camera_position);

    template<typename T>
    GFXTexture* get_asset_thumbnail(AssetPtr<T>& asset) {
        Expects(asset.handle != nullptr);
        
        if(asset_thumbnails.count(asset->path)) {
            return asset_thumbnails[asset->path];
        } else {
            if constexpr(std::is_same_v<T, Material>) {
                auto texture = get_material_preview(*asset.handle);
                
                asset_thumbnails[asset->path] = texture;
                
                return texture;
            } else if constexpr(std::is_same_v<T, Mesh>) {
                auto texture = get_mesh_preview(*asset.handle);
                
                asset_thumbnails[asset->path] = texture;
                
                return texture;
            } else if constexpr(std::is_same_v<T, Texture>) {
                auto texture = get_texture_preview(*asset.handle);
                
                asset_thumbnails[asset->path] = texture;
                
                return texture;
            } else {
                return engine->get_renderer()->dummyTexture;
            }
        }
    }
    
    GFXTexture* get_asset_thumbnail(const file::Path path) {
        if(asset_thumbnails.count(path.string())) {
            return asset_thumbnails[path.string()];
        } else {
            auto [asset, block] = assetm->load_asset_generic(path);
            
            // store as dummy texture, as to stop infinite reload because of failure (e.g. out of date model)
            if(asset == nullptr) {
                asset_thumbnails[path.string()] = engine->get_renderer()->dummyTexture;

                return engine->get_renderer()->dummyTexture;
            }
                        
            if(can_load_asset<Material>(path)) {
                auto ptr = AssetPtr<Material>(static_cast<Material*>(asset), block);

                return get_asset_thumbnail(ptr);
            } else if(can_load_asset<Mesh>(path)) {
                auto ptr = AssetPtr<Mesh>(static_cast<Mesh*>(asset), block);
                
                return get_asset_thumbnail(ptr);
            } else if(can_load_asset<Texture>(path)) {
                auto ptr = AssetPtr<Texture>(static_cast<Texture*>(asset), block);
                
                return get_asset_thumbnail(ptr);
            } else {
                return engine->get_renderer()->dummyTexture;
            }
        }
    }
    
    bool has_asset_edit_changed = false;
    
    template<typename T>
    bool edit_asset(const char* name, AssetPtr<T>& asset) {
        ImGui::PushID(&asset);
        
        auto draw_list = ImGui::GetWindowDrawList();
        const auto window_pos = ImGui::GetCursorScreenPos();
        
        const float thumbnail_size = 35.0f;
        const auto line_height = ImGui::GetTextLineHeight();

        const float item_width = ImGui::CalcItemWidth();
        const float inner_spacing = ImGui::GetStyle().ItemInnerSpacing.x;
        
        const auto frame_color = ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
        const auto text_color = ImGui::GetStyle().Colors[ImGuiCol_Text];

        const ImRect edit_rect = ImRect(window_pos, ImVec2(window_pos.x + thumbnail_size, window_pos.y + thumbnail_size));
    
        if(asset)
            draw_list->AddImageRounded(get_asset_thumbnail(asset), edit_rect.Min, edit_rect.Max, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 3.0f);
        
        ImRect path_rect = ImRect(ImVec2(window_pos.x + thumbnail_size + 10.0f, window_pos.y), ImVec2(window_pos.x + item_width - inner_spacing, window_pos.y + 20.0f));

        draw_list->AddText(ImVec2(window_pos.x + item_width, window_pos.y + (thumbnail_size / 2.0f) - (line_height / 2.0f)), ImColor(text_color), name);
                
        draw_list->AddRectFilled(path_rect.Min, path_rect.Max, ImColor(frame_color), 3.0f);
        
        ImRect clear_rect = ImRect(ImVec2(window_pos.x + thumbnail_size + 10.0f, window_pos.y + 20.0f), ImVec2(window_pos.x + thumbnail_size + 30.0f, window_pos.y + 40.0f));
        
        draw_list->AddRectFilled(clear_rect.Min, clear_rect.Max, ImColor(255, 0, 0, 255), 3.0f);
        
        std::string path;
        if(asset)
            path = asset->path;
        else
            path = "None";
        
        ImGui::PushClipRect(path_rect.Min, path_rect.Max, false);

        draw_list->AddText(ImVec2(window_pos.x + thumbnail_size + 10.0f, window_pos.y), ImColor(text_color), path.c_str());

        ImGui::Dummy(ImVec2(thumbnail_size, thumbnail_size + 10.0f));
        
        ImGui::ItemAdd(path_rect, ImGui::GetID("path"));
        
        ImGui::PopClipRect();

        if(ImGui::IsItemClicked()) {
            current_asset_type = get_asset_type<T>();
            open_asset_popup = true;
            on_asset_select = [&asset, this](auto p) {
                asset = assetm->get<T>(file::app_domain / p);
                has_asset_edit_changed = true;
            };
        }
        
        ImGui::ItemAdd(edit_rect, ImGui::GetID("edit"));
        
        if(ImGui::IsItemClicked())
            asset_selected(asset->path, get_asset_type<T>());
        
        ImGui::ItemAdd(clear_rect, ImGui::GetID("clear"));

        if(ImGui::IsItemClicked())
            asset.clear();
        
        ImGui::PopID();
        
        if(has_asset_edit_changed) {
            has_asset_edit_changed = false;
            return true;
        } else {
            return false;
        }
    }
    
    DebugPass* debugPass = nullptr;

    int viewport_width = 1, viewport_height = 1;
    
protected:
    struct ViewportRenderTarget {
        Scene* scene = nullptr;
        RenderTarget* target = nullptr;
    };
    
    std::unordered_map<ImGuiID, ViewportRenderTarget> viewport_render_targets;
    
private:
    void load_options();
    void save_options();
    
    void load_thumbnail_cache();
    void save_thumbnail_cache();
    
	std::string id;
    std::string iniFileName;
    
    std::unordered_map<std::string, GFXTexture*> asset_thumbnails;
    
    bool accepting_viewport_input = false, doing_viewport_input = false;
    int viewport_x = 1, viewport_y = 1;
    
    bool transforming_axis = false;
    SelectableObject::Axis axis;
    Vector3 last_object_position;
    
    bool open_asset_popup = false;
    AssetType current_asset_type;
    std::function<void(std::filesystem::path)> on_asset_select;
    
    UndoStack* current_stack = nullptr;

	int defaultX, defaultY, defaultWidth, defaultHeight;
	std::vector<std::string> lastOpenedFiles;

    void walkObject(Object object, Object parentObject = NullObject);
    
    void editTransform(Object object, Transform transform);
    void editRenderable(Renderable& mesh);
    
    void edit_asset_mesh(const char* name, AssetPtr<Mesh>& asset);
    void edit_asset_texture(const char* name, AssetPtr<Texture>& asset);
    void edit_asset_material(const char* name, AssetPtr<Material>& asset);
};

inline void editPath(const char* label, std::string& path, bool editable = true, const std::function<void()> on_selected = nullptr) {
    ImGui::PushID(label);

    if(!editable) {
        ImGui::Text("%s: %s", label, path.c_str());
    } else {
        ImGui::InputText(label, &path);
    }

    ImGui::SameLine();

    if(ImGui::Button("...")) {
        platform::open_dialog(false, [&path, &on_selected](std::string p) {
            path = file::get_relative_path(file::Domain::App, p).string();

            if(on_selected != nullptr)
                on_selected();
        });
    }

    ImGui::PopID();
}

class SelectionCommand : public Command {
public:
    CommonEditor* editor = nullptr;
    
    Object new_selection;
    Object old_selection;
    
    std::string fetch_name() override {
        return "Change selection to " + engine->get_scene()->get(new_selection).name;
    }
    
    void undo() override {
        editor->selected_object = old_selection;
    }
    
    void execute() override {
        editor->selected_object = new_selection;
    }
};
