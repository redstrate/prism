#pragma once

#include <string>

#include "commoneditor.hpp"
#include "undostack.hpp"

class Scene;

class Editor {
public:
    std::string path;
        
    bool has_been_docked = false;
    bool modified = false;
    bool wants_to_close = false;

    std::string get_window_title() {
        auto window_class = get_window_class();
        return get_title() + "###" + std::to_string(window_class.ClassId);
    }
    
    virtual Scene* get_scene() const {
        return nullptr;
    }

    virtual bool has_menubar() const { return false; }

    virtual std::string get_title() const { return ""; }
    virtual void draw([[maybe_unused]] CommonEditor* editor) {}
    virtual void setup_windows([[maybe_unused]] ImGuiID dockspace) {}

    ImGuiWindowClass get_window_class() const {
        ImGuiWindowClass window_class = {};
        window_class.ClassId = ImGui::GetID(this);
        window_class.DockingAllowUnclassed = true;

        return window_class;
    }
    
    std::string get_window_name(std::string title) {
        return title + "##" + std::to_string((uint64_t)this);
    }

    bool begin(std::string title, bool* p_open = nullptr) {
        return ImGui::Begin(get_window_name(title).c_str(), p_open);
    }
    
    UndoStack undo_stack;
};

class PrismEditor : public CommonEditor {
public:
    PrismEditor();
    
    void renderEditor(GFXCommandBuffer* command_buffer) override;
    void drawUI() override;
    void updateEditor(float deltaTime) override;
    
    void object_selected(Object object) override;
    void asset_selected(std::filesystem::path path, AssetType type) override;
    
private:
    void open_asset(const prism::Path path);
    void setup_editor(Editor* editor);
};

std::string get_filename(const std::string path);
