#pragma once

#include <array>

#include "frustum.hpp"
#include "matrix.hpp"
#include "vector.hpp"
#include "components.hpp"
#include "aabb.hpp"
#include "plane.hpp"
#include "asset_types.hpp"

class Scene;

struct CameraFrustum {
    std::array<Plane, 6> planes;
};

CameraFrustum extract_frustum(Matrix4x4 combined);
CameraFrustum camera_extract_frustum(Scene& scene, Object cam);
CameraFrustum normalize_frustum(const CameraFrustum& frustum);

bool test_point_plane(const Plane& plane, const prism::float3& point);
bool test_point_frustum(const CameraFrustum& frustum, const prism::float3& point);
bool test_aabb_frustum(const CameraFrustum& frustum, const prism::aabb& aabb);

prism::aabb get_aabb_for_part(const Transform& transform, const Mesh::Part& part);
