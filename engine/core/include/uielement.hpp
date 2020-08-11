#pragma once

#include <string>

#include "asset.hpp"

enum class MetricType {
    Absolute,
    Relative,
    Offset
};

struct Color {
    Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}

    Color(const float v) : r(v), g(v), b(v), a(1.0f) {}

    union {
        struct {
            float r, g, b, a;
        };

        float v[4];
    };
};

class GFXTexture;
class Texture;

class UIElement {
public:
    struct Metrics {
        struct Metric {
            MetricType type = MetricType::Absolute;
            int value = 0;
        };

        Metric x, y, width, height;
    } metrics;

    struct Background {
        Color color = Color(0.0f);
        std::string image;

        AssetPtr<Texture> texture;
    } background;

    enum class TextLocation {
        TopLeft,
        Center
    } text_location = TextLocation::TopLeft;

    std::string id, text, parent;

    bool wrap_text = false;

    float absolute_x = 0.0f, absolute_y = 0.0f, absolute_width = 0.0f, absolute_height = 0.0f;
    float text_x = 0.0f, text_y = 0.0f;

    bool visible = true;

    std::string on_click_script;
};

inline bool operator==(const UIElement& a, const UIElement& b) {
    return a.id == b.id;
}
