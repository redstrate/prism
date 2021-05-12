#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

#include "uielement.hpp"
#include "file.hpp"
#include "common.hpp"

class GFXBuffer;

namespace ui {
    class Screen {
    public:
        Screen() {}
        Screen(const prism::Path path);

        void process_event(const std::string& type, std::string data = "");

        void process_mouse(const int x, const int y);
        void calculate_sizes();

        std::vector<UIElement> elements;

        UIElement* find_element(const std::string& id);

        using CallbackFunction = std::function<void()>;
        std::map<std::string, CallbackFunction> listeners;

        bool blurs_background = false;

        void add_listener(const std::string& id, std::function<void()> callback);

        prism::Extent extent;
        bool view_changed = false;

        GFXBuffer* glyph_buffer = nullptr;
        GFXBuffer* instance_buffer = nullptr;
        GFXBuffer* elements_buffer = nullptr;
    };
}
