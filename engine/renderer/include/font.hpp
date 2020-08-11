#pragma once

constexpr auto numGlyphs = 95, maxInstances = 11395;
constexpr auto fontSize = 0;

struct FontChar {
    unsigned short x0, y0, x1, y1;
    float xoff, yoff, xadvance;
    float xoff2, yoff2;
};

struct Font {
    int width, height;
    int ascent, descent, gap;
    FontChar sizes[2][numGlyphs];
    float ascentSizes[2];
};

inline Font font;

inline float get_string_width(std::string s) {
    float t = 0.0f;
    for(size_t i = 0; i < s.length(); i++) {
        auto index = s[i] - 32;

        t += font.sizes[fontSize][index].xadvance;
    }

    return t;
}

inline float get_font_height() {
    return font.ascentSizes[fontSize];
}
