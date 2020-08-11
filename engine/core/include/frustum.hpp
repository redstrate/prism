#pragma once

struct CameraFrustum {
    std::array<Plane, 6> planes;
};

inline CameraFrustum extract_frustum(const Matrix4x4 combined) {
    CameraFrustum frustum;
    
    // left plane
    frustum.planes[0].a = combined.data[0][3] + combined.data[0][0];
    frustum.planes[0].b = combined.data[1][3] + combined.data[1][0];
    frustum.planes[0].c = combined.data[2][3] + combined.data[2][0];
    frustum.planes[0].d = combined.data[3][3] + combined.data[3][0];
    
    // right plane
    frustum.planes[1].a = combined.data[0][3] - combined.data[0][0];
    frustum.planes[1].b = combined.data[1][3] - combined.data[1][0];
    frustum.planes[1].c = combined.data[2][3] - combined.data[2][0];
    frustum.planes[1].d = combined.data[3][3] - combined.data[3][0];
    
    // top plane
    frustum.planes[2].a = combined.data[0][3] - combined.data[0][1];
    frustum.planes[2].b = combined.data[1][3] - combined.data[1][1];
    frustum.planes[2].c = combined.data[2][3] - combined.data[2][1];
    frustum.planes[2].d = combined.data[3][3] - combined.data[3][1];
    
    // bottom plane
    frustum.planes[3].a = combined.data[0][3] + combined.data[0][1];
    frustum.planes[3].b = combined.data[1][3] + combined.data[1][1];
    frustum.planes[3].c = combined.data[2][3] + combined.data[2][1];
    frustum.planes[3].d = combined.data[3][3] + combined.data[3][1];
    
    // near plane
    frustum.planes[4].a = combined.data[0][2];
    frustum.planes[4].b = combined.data[1][2];
    frustum.planes[4].c = combined.data[2][2];
    frustum.planes[4].d = combined.data[3][2];
    
    // far plane
    frustum.planes[5].a = combined.data[0][3] - combined.data[0][2];
    frustum.planes[5].b = combined.data[1][3] - combined.data[1][2];
    frustum.planes[5].c = combined.data[2][3] - combined.data[2][2];
    frustum.planes[5].d = combined.data[3][3] - combined.data[3][2];
    
    return frustum;
}

inline CameraFrustum camera_extract_frustum(Scene& scene, Object cam) {
    const auto camera_component = scene.get<Camera>(cam);
    const Matrix4x4 combined = camera_component.perspective * camera_component.view;
    
    return extract_frustum(combined);
}

inline CameraFrustum normalize_frustum(const CameraFrustum& frustum) {
    CameraFrustum normalized_frustum;
    for(int i = 0; i < 6; i++)
        normalized_frustum.planes[i] = normalize(frustum.planes[i]);
    
    return normalized_frustum;
}

inline bool test_point_plane(const Plane& plane, const Vector3& point) {
    return distance_to_point(plane, point) > 0.0;
}

inline bool test_point_frustum(const CameraFrustum& frustum, const Vector3& point) {
    bool inside_frustum = false;
    
    for(int i = 0; i < 6; i++)
        inside_frustum |= !test_point_plane(frustum.planes[i], point);
    
    return !inside_frustum;
}

inline bool test_aabb_frustum(const CameraFrustum& frustum, const AABB& aabb) {
    for(int i = 0; i < 6; i++) {
        int out = 0;
        
        for(const auto point : get_points(aabb))
            out += (distance_to_point(frustum.planes[i], point) < 0.0) ? 1 : 0;
        
        if(out == 8)
            return false;
    }
    
    return true;
}

inline AABB get_aabb_for_part(const Transform& transform, const Mesh::Part& part) {
    AABB aabb = {};
    aabb.min = part.aabb.min - transform.get_world_position();
    aabb.max = transform.get_world_position() + part.aabb.max;
    aabb.min *= transform.scale;
    aabb.max *= transform.scale;
    
    return aabb;
}
