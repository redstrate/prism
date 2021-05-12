#pragma once

#include "matrix.hpp"
#include "vector.hpp"
#include "quaternion.hpp"

namespace prism {
    Matrix4x4 perspective(float field_of_view, float aspect, float z_near, float z_far);
    Matrix4x4 infinite_perspective(float field_of_view, float aspect, float z_near);

    Matrix4x4 orthographic(float left, float right, float bottom, float top, float zNear, float zFar);

    Matrix4x4 look_at(float3 eye, float3 center, float3 up);
    Quaternion quat_look_at(float3 eye, float3 center, float3 up);

    Matrix4x4 translate(Matrix4x4 matrix, float3 translation);
    Matrix4x4 rotate(Matrix4x4 matrix, float angle, float3 axis);
	Matrix4x4 scale(Matrix4x4 matrix, float3 scale);
}
