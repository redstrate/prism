#include "transform.hpp"

#include <cmath>

#include "math.hpp"

/*
 These produce left-handed matrices.
 Metal/DX12 are both NDC +y down.
 */
Matrix4x4 transform::perspective(const float field_of_view, const float aspect, const float z_near, const float z_far) {
    const float tan_half_fov = tanf(field_of_view / 2.0f);

    Matrix4x4 result(0.0f);
    result[0][0] = 1.0f / (aspect * tan_half_fov);
    result[1][1] = 1.0f / tan_half_fov;
    result[2][2] = z_far / (z_far - z_near);
    result[2][3] = 1.0f;
    result[3][2] = -(z_far * z_near) / (z_far - z_near);
    
    return result;
}

Matrix4x4 transform::infinite_perspective(const float field_of_view, const float aspect, const float z_near) {
    const float range = tanf(field_of_view / 2.0f) * z_near;
    const float left = -range * aspect;
    const float right = range * aspect;
    const float bottom = -range;
    const float top = range;

    Matrix4x4 result(0.0f);
    result[0][0] = (2.0f * z_near) / (right - left);
    result[1][1] = (2.0f * z_near) / (top - bottom);
    result[2][2] = 1.0f;
    result[2][3] = 1.0f;
    result[3][2] = -2.0 * z_near;
            
    return result;
}

Matrix4x4 transform::orthographic(float left, float right, float bottom, float top, float zNear, float zFar) {
    Matrix4x4 result(1.0f);
    result[0][0] = 2.0f / (right - left);
    result[1][1] = 2.0f / (top - bottom);
    result[2][2] = 1.0f / (zFar - zNear);
    result[3][0] = -(right + left) / (right - left);
    result[3][1] = -(top + bottom) / (top - bottom);
    result[3][2] = - zNear / (zFar - zNear);

    return result;
}

Matrix4x4 transform::look_at(const Vector3 eye, const Vector3 center, const Vector3 up) {
    const Vector3 f = normalize(center - eye);
    const Vector3 s = normalize(cross(f, up));
    const Vector3 u = cross(f, s);
    
    Matrix4x4 result(1.0f);
    result[0][0] = s.x;
    result[1][0] = s.y;
    result[2][0] = s.z;
    result[0][1] = u.x;
    result[1][1] = u.y;
    result[2][1] = u.z;
    result[0][2] = f.x;
    result[1][2] = f.y;
    result[2][2] = f.z;
    result[3][0] = -dot(s, eye);
    result[3][1] = -dot(u, eye);
    result[3][2] = -dot(f, eye);
    
    return result;
}

Matrix4x4 transform::translate(const Matrix4x4 matrix, const Vector3 translation) {
    Matrix4x4 result(1.0f);
    result[3] = Vector4(translation, 1.0);
    
    return matrix * result;
}

Matrix4x4 transform::rotate(const Matrix4x4 matrix, const float angle, const Vector3 v) {
    const float a = angle;
    const float c = cosf(a);
    const float s = sinf(a);

    const Vector3 axis = normalize(v);
    const Vector3 temp = Vector3((1.0f - c) * axis);

    Matrix4x4 Rotate(1.0f);
    Rotate[0][0] = c + temp[0] * axis[0];
    Rotate[0][1] = temp[0] * axis[1] + s * axis[2];
    Rotate[0][2] = temp[0] * axis[2] - s * axis[1];

    Rotate[1][0] = temp[1] * axis[0] - s * axis[2];
    Rotate[1][1] = c + temp[1] * axis[1];
    Rotate[1][2] = temp[1] * axis[2] + s * axis[0];

    Rotate[2][0] = temp[2] * axis[0] + s * axis[1];
    Rotate[2][1] = temp[2] * axis[1] - s * axis[0];
    Rotate[2][2] = c + temp[2] * axis[2];

    Matrix4x4 result(1.0f);
    result[0] = Rotate[0][0] + Rotate[0][1] + Rotate[0][2];
    result[1] = Rotate[1][0] + Rotate[1][1] + Rotate[1][2];
    result[2] = Rotate[2][0] + Rotate[2][1] + Rotate[2][2];
    
    return matrix * result;
}

Matrix4x4 transform::scale(const Matrix4x4 matrix, const Vector3 scale) {
    Matrix4x4 result(1.0f);
    result[0][0] = scale.x;
    result[1][1] = scale.y;
    result[2][2] = scale.z;
    
    return matrix * result;
}

Quaternion transform::quat_look_at(const Vector3 eye, const Vector3 center, const Vector3 up) {
    const Vector3 direction = normalize(center - eye);
    
    Matrix3x3 result(1.0f);
    result[2] = direction;
    result[0] = cross(up, result[2]);
    result[1] = cross(result[2], result[0]);
    
    return quat_from_matrix(result);
}
