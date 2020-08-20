#pragma once

#include <cmath>

#include "vector.hpp"

struct Plane {
    float a = 0.0, b = 0.0, c = 0.0, d = 0.0;
};

inline Plane normalize(const Plane& plane) {
    const float magnitude = std::sqrt(plane.a * plane.a +
                                      plane.b * plane.b +
                                      plane.c * plane.c);
    
    Plane normalized_plane = plane;
    normalized_plane.a = plane.a / magnitude;
    normalized_plane.b = plane.b / magnitude;
    normalized_plane.c = plane.c / magnitude;
    normalized_plane.d = plane.d / magnitude;
    
    return normalized_plane;
}

inline float distance_to_point(const Plane& plane, const Vector3& point) {
    return plane.a * point.x +
           plane.b * point.y +
           plane.c * point.z +
           plane.d;
}
