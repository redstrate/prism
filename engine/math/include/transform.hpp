#pragma once

#include "matrix.hpp"
#include "vector.hpp"
#include "quaternion.hpp"

namespace transform {
    Matrix4x4 perspective(const float field_of_view, const float aspect, const float z_near, const float z_far);
    Matrix4x4 infinite_perspective(const float field_of_view, const float aspect, const float z_near);

    Matrix4x4 orthographic(float left, float right, float bottom, float top, float zNear, float zFar);

    Matrix4x4 look_at(const Vector3 eye, const Vector3 center, const Vector3 up);
    Quaternion quat_look_at(const Vector3 eye, const Vector3 center, const Vector3 up);

    Matrix4x4 translate(const Matrix4x4 matrix, const Vector3 translation);
    Matrix4x4 rotate(const Matrix4x4 matrix, const float angle, const Vector3 axis);
	Matrix4x4 scale(const Matrix4x4 matrix, const Vector3 scale);
}
