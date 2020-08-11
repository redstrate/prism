#pragma once

#include <cstdint>

/// A 2D extent.
struct Extent {
    Extent() : width(0), height(0) {}
    Extent(const uint32_t width, const uint32_t height) : width(width), height(height) {}
    
    uint32_t width = 0, height = 0;
};

/// A 2D offset.
struct Offset {
    Offset() : x(0), y(0) {}
    Offset(const int32_t x, const int32_t y) : x(x), y(y) {}
    
    int32_t x = 0, y = 0;
};

/// A 2D rectangle defined by a offset and an etent.
struct Rectangle {
    Rectangle() {}
    Rectangle(const Offset offset, const Extent extent) : offset(offset), extent(extent) {}
    Rectangle(const int32_t x, const int32_t y, const uint32_t width, const uint32_t height) : offset(x, y), extent(width, height) {}
    
    Offset offset;
    Extent extent;
};
