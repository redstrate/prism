#pragma once

#include "vector.hpp"

class Quaternion {
public:
    constexpr Quaternion(const float v = 0.0f) : w(1.0f), x(v), y(v), z(v) {}
    
    constexpr Quaternion(const float x, const float y, const float z, const float w) : w(w), x(x), y(y), z(z) {}
    
    float w, x, y, z;
}; 

constexpr inline Quaternion operator*(const Quaternion lhs, const float rhs) {
    Quaternion tmp(lhs);
    tmp.x *= rhs;
    tmp.y *= rhs;
    tmp.z *= rhs;
    tmp.w *= rhs;
    
    return tmp;
}

constexpr inline prism::float3 operator*(const Quaternion& lhs, const prism::float3& rhs) {
    const prism::float3 quatVector = prism::float3(lhs.x, lhs.y, lhs.z);
    const prism::float3 uv = cross(quatVector, rhs);
    const prism::float3 uuv = cross(quatVector, uv);
    
    return rhs + ((uv * lhs.w) + uuv) * 2.0f;
}

constexpr inline prism::float3 operator*(const prism::float3& rhs, const Quaternion& lhs) {
    return lhs * rhs;
}

constexpr inline float dot(const Quaternion& a, const Quaternion& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline float length(const Quaternion& quat) {
    return sqrtf(dot(quat, quat));
}

inline Quaternion normalize(const Quaternion& quat) {
    const float l = length(quat);
    
    return quat * (1.0f / l);
}

constexpr inline Quaternion lerp(const Quaternion& a, const Quaternion& b, const float time) {
    float cosom = dot(a, b);
    
    Quaternion end = b;
    if(cosom < 0.0f) {
        cosom = -cosom;
        end.x = -end.x;
        end.y = -end.y;
        end.z = -end.z;
        end.w = -end.w;
    }
    
    float sclp = 0.0f, sclq = 0.0f;
    if((1.0f - cosom) > 0.0001f) {
        const float omega = std::acos(cosom);
        const float sinom = std::sin(omega);
        sclp  = std::sin((1.0f - time) * omega) / sinom;
        sclq  = std::sin(time* omega) / sinom;
    } else {
        sclp = 1.0f - time;
        sclq = time;
    }
    
    return {sclp * a.x + sclq * end.x,
        sclp * a.y + sclq * end.y,
        sclp * a.z + sclq * end.z,
        sclp * a.w + sclq * end.w};
}

constexpr inline Quaternion operator*(const Quaternion lhs, const Quaternion rhs) {
    Quaternion tmp;
    tmp.w = lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z;
    tmp.x = lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y;
    tmp.y = lhs.w * rhs.y + lhs.y * rhs.w + lhs.z * rhs.x - lhs.x * rhs.z;
    tmp.z = lhs.w * rhs.z + lhs.z * rhs.w + lhs.x * rhs.y - lhs.y * rhs.x;
    
    return tmp;
}
