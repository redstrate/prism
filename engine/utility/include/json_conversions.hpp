#pragma once

#include <nlohmann/json.hpp>

#include "vector.hpp"
#include "quaternion.hpp"

inline void to_json(nlohmann::json& j, const Vector3& p) {
    j["x"] = p.x;
    j["y"] = p.y;
    j["z"] = p.z;
}

inline void from_json(const nlohmann::json& j, Vector3& p) {
    p.x = j["x"];
    p.y = j["y"];
    p.z = j["z"];
}

inline void to_json(nlohmann::json& j, const Quaternion& p) {
    j["x"] = p.x;
    j["y"] = p.y;
    j["z"] = p.z;
    j["w"] = p.w;
}

inline void from_json(const nlohmann::json& j, Quaternion& p) {
    p.x = j["x"];
    p.y = j["y"];
    p.z = j["z"];
    p.w = j["w"];
}
