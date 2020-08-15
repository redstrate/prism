#include "screen.hpp"

#include <fstream>
#include <string_view>
#include <nlohmann/json.hpp>

#include "file.hpp"
#include "font.hpp"
#include "engine.hpp"
#include "string_utils.hpp"
#include "log.hpp"
#include "assertions.hpp"

void ui::Screen::calculate_sizes() {
    unsigned int numChildren = 0;

    int actualWidth = extent.width, actualHeight = extent.height;
    int offsetX = 0, offsetY = 0;

    for(size_t i = 0; i < elements.size(); i++) {
        auto& element = elements[i];

        auto id = "ui_" + element.id;
        //if(engine->has_localization(id))
        //    element.text = engine->localize(id);

        UIElement* parent = nullptr;
        if(!element.parent.empty())
            parent = find_element(element.parent);

        const auto& fillAbsolute = [parent](const auto& metric, auto& absolute, auto base, auto offset) {
            switch(metric.type) {
                case MetricType::Absolute:
                    absolute = offset + metric.value;
                    break;
                case MetricType::Relative:
                    absolute = offset + ((base - offset) * (metric.value / 100.0f));
                    break;
                case MetricType::Offset:
                {
                    if(parent == nullptr)
                        absolute = base - metric.value;
                    else
                        absolute = parent->absolute_height - metric.value;
                }
                    break;
            }
        };

        fillAbsolute(element.metrics.x, element.absolute_x, offsetX + actualWidth, offsetX);
        fillAbsolute(element.metrics.y, element.absolute_y, offsetY + actualHeight, offsetY);

        fillAbsolute(element.metrics.width, element.absolute_width, actualWidth, 0);
        fillAbsolute(element.metrics.height, element.absolute_height, actualHeight, 0);

        if(parent) {
            const float heightPerRow = actualWidth / (float)elements.size();

            element.absolute_x = (float)numChildren * heightPerRow;
            element.absolute_y = actualHeight - 50.0f;
            element.absolute_width = (float)heightPerRow;
            element.absolute_height = (float)50.0f;

            numChildren++;
        }

        if(element.text_location == UIElement::TextLocation::Center) {
            if(get_string_width(element.text) < element.absolute_width)
                element.text_x = (element.absolute_width / 2.0f) - (get_string_width(element.text) / 2.0f);

            if(get_font_height() < element.absolute_height)
                element.text_y = (element.absolute_height / 2.0f) - (get_font_height() / 2.0f);
        }
    }
}

void ui::Screen::process_mouse(const int x, const int y) {
    Expects(x >= 0);
    Expects(y >= 0);
}

UIElement* ui::Screen::find_element(const std::string& id) {
    Expects(!id.empty());

    UIElement* foundElement = nullptr;
    for(auto& element : elements) {
        if(element.id == id)
            foundElement = &element;
    }
    
    Expects(foundElement != nullptr);

    return foundElement;
}

ui::Screen::Screen(const file::Path path) {
    auto file = file::open(path);
    if(!file.has_value()) {
        console::error(System::Core, "Failed to load UI from {}!", path);
        return;
    }

    nlohmann::json j;
    file->read_as_stream() >> j;

    for(auto& element : j["elements"]) {
        UIElement ue;
        ue.id = element["id"];

        if(element.contains("on_click"))
            ue.on_click_script = element["on_click"];

        if(element.contains("text"))
            ue.text = element["text"];

        if(element.contains("parent"))
            ue.parent = element["parent"];

        if(element.contains("metrics")) {
            const auto& metrics = element["metrics"];

            const auto parseMetric = [](const std::string& str) {
                UIElement::Metrics::Metric m;

                if(string_contains(str, "px")) {
                    auto v = remove_substring(str, "px");

                    if(string_contains(str, "@")) {
                        m.type = MetricType::Offset;
                        m.value = std::stoi(remove_substring(v, "@"));
                    } else {
                        m.type = MetricType::Absolute;
                        m.value = std::stoi(v);
                    }
                } else if(string_contains(str, "%")) {
                    m.type = MetricType::Relative;
                    m.value = std::stoi(remove_substring(str, "%"));
                }

                return m;
            };

            if(metrics.contains("x"))
                ue.metrics.x = parseMetric(metrics["x"]);

            if(metrics.contains("y"))
                ue.metrics.y = parseMetric(metrics["y"]);

            if(metrics.contains("width"))
                ue.metrics.width = parseMetric(metrics["width"]);

            if(metrics.contains("height"))
                ue.metrics.height = parseMetric(metrics["height"]);
        }

        if(element.contains("background")) {
            if(element["background"].contains("color")) {
                auto tokens = tokenize(element["background"]["color"].get<std::string_view>(), ",");
                ue.background.color.r = std::stof(tokens[0]);
                ue.background.color.g = std::stof(tokens[1]);
                ue.background.color.b = std::stof(tokens[2]);
                ue.background.color.a = std::stof(tokens[3]);
            }

            if(element["background"].contains("image")) {
                ue.background.image = element["background"]["image"];
            }
        }

        if(element.contains("wrap"))
            ue.wrap_text = element["wrap"];

        if(element.contains("textLocation")) {
            if(element["textLocation"] == "topLeft")
                ue.text_location = UIElement::TextLocation::TopLeft;
            else if(element["textLocation"] == "center")
                ue.text_location = UIElement::TextLocation::Center;
        }

        elements.push_back(ue);
    }
}

void ui::Screen::add_listener(const std::string& id, std::function<void()> callback) {
    listeners[id] = callback;
}

void ui::Screen::process_event(const std::string& type, const std::string data) {

}
