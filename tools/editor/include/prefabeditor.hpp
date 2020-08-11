#pragma once

#include "prismeditor.hpp"

class PrefabEditor : public Editor {
public:
    Scene* scene = nullptr;
    Object root_object;

    bool showOutliner = true;
    bool showProperties = true;

    bool has_menubar() const override;
    std::string get_title() const override;

    Scene* get_scene() const override;
    
    void setup_windows(ImGuiID dockspace) override;

    void draw(CommonEditor* editor) override;
};
