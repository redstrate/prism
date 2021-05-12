#pragma once

#include <array>

#include "vector.hpp"

namespace prism {
    /// A 3D axis aligned bounding box.
    struct aabb {
        float3 min, max;
    };

    /// Creates an array of 8 points, each of these being one extreme of the bounding box..
    inline std::array<float3, 8> get_points(const aabb& aabb) {
        return {
                float3(aabb.min.x, aabb.min.y, aabb.min.z),
                float3(aabb.max.x, aabb.min.y, aabb.min.z),
                float3(aabb.min.x, aabb.max.y, aabb.min.z),
                float3(aabb.max.x, aabb.max.y, aabb.min.z),
                float3(aabb.min.x, aabb.min.y, aabb.max.z),
                float3(aabb.max.x, aabb.min.y, aabb.max.z),
                float3(aabb.min.x, aabb.max.y, aabb.max.z),
                float3(aabb.max.x, aabb.max.y, aabb.max.z)};
    }
}