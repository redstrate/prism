#pragma once

#include "prismeditor.hpp"
#include "asset.hpp"
#include "scene.hpp"

class MaterialEditor : public Editor {
public:
    Renderable* renderable = nullptr;
    AssetPtr<Material> material;
    Scene* scene = nullptr;

    bool showOutliner = true;
    bool showProperties = true;

    bool has_menubar() const override;
    std::string get_title() const override;

    Scene* get_scene() const override;
    
    void setup_windows(ImGuiID dockspace) override;
    
    void setup_material();
    
    void draw(CommonEditor* editor) override;
};
