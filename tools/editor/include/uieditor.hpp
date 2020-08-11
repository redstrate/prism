#pragma once

#include "prismeditor.hpp"

#include "screen.hpp"

class UIEditor : public Editor {
public:
    UIEditor();

    ui::Screen* screen = nullptr;
    UIElement* current_element = nullptr;

    bool has_menubar() const override;
    std::string get_title() const override;

    Scene* get_scene() const override;
    
    void setup_windows(ImGuiID dockspace) override;
    
    void edit_metric(const char* label, UIElement::Metrics::Metric* metric);
    std::string save_metric(UIElement::Metrics::Metric metric);

    void save(std::string path);

    void draw(CommonEditor* editor) override;
};
