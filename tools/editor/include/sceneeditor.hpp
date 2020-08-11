#pragma once

#include "prismeditor.hpp"
#include "engine.hpp"

class AddObjectCommand : public Command {
public:
    Object id;
    std::string name;
    
    std::string fetch_name() override {
        return "Add object " + name;
    }
    
    void undo() override {
        engine->get_scene()->remove_object(id);
    }
    
    void execute() override {
        engine->get_scene()->add_object_by_id(id);
        engine->get_scene()->get(id).name = name;
    }
};

class SceneEditor : public Editor {
public:
    Scene* scene = nullptr;

    bool showSceneSettings = true;
    bool showOutliner = true;
    bool showProperties = true;
    bool showViewport = true;
    bool showAssets = true;
    bool showUndoStack = false;

    bool has_menubar() const override;
    std::string get_title() const override;

    Scene* get_scene() const override;

    void setup_windows(ImGuiID dockspace) override;

    void draw(CommonEditor* editor) override;
};
