#pragma once

#include <array>

#include "vector.hpp"

/// A 3D axis aligned bounding box.
struct AABB {
    Vector3 min, max;
};

/// Creates an array of 8 points, each of these being one extreme of the bounding box..
inline std::array<Vector3, 8> get_points(const AABB& aabb) {
    return {
        Vector3(aabb.min.x, aabb.min.y, aabb.min.z),
        Vector3(aabb.max.x, aabb.min.y, aabb.min.z),
        Vector3(aabb.min.x, aabb.max.y, aabb.min.z),
        Vector3(aabb.max.x, aabb.max.y, aabb.min.z),
        Vector3(aabb.min.x, aabb.min.y, aabb.max.z),
        Vector3(aabb.max.x, aabb.min.y, aabb.max.z),
        Vector3(aabb.min.x, aabb.max.y, aabb.max.z),
        Vector3(aabb.max.x, aabb.max.y, aabb.max.z)};
}
