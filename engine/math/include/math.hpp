#pragma once

#include "matrix.hpp"
#include "transform.hpp"
#include "vector.hpp"
#include "ray.hpp"

constexpr double PI = 3.141592653589793;

template<typename T>
constexpr inline T radians(const T degrees) {
    return degrees / static_cast<T>(180) * static_cast<T>(PI);
}

template<typename T>
constexpr inline T degrees(const T radians) {
    return radians / static_cast<T>(PI) * static_cast<T>(180);
}

template<typename T>
constexpr inline bool nearly_equal(const T a, const T b) {
    return std::nextafter(a, std::numeric_limits<T>::lowest()) <= b
    && std::nextafter(a, std::numeric_limits<T>::max()) >= b;
}

Matrix4x4 matrix_from_quat(const Quaternion quat);
Quaternion quat_from_matrix(const Matrix3x3 matrix);
Quaternion euler_to_quat(const Vector3 angle);
Vector3 quat_to_euler(const Quaternion quat);
Matrix4x4 inverse(const Matrix4x4 m);
Quaternion angle_axis(const float angle, const Vector3 axis);
