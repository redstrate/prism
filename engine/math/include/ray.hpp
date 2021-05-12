#pragma once

#include "vector.hpp"

namespace prism {
    struct ray {
        float3 origin, direction;
        float t = 0.0f;
    };

    float closest_distance_between_lines(ray& l1, ray& l2);
}