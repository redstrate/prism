#include "math.hpp"

Matrix4x4 matrix_from_quat(const Quaternion quat) {
    const float qxx = quat.x * quat.x;
    const float qyy = quat.y * quat.y;
    const float qzz = quat.z * quat.z;
    const float qxz = quat.x * quat.z;
    const float qxy = quat.x * quat.y;
    const float qyz = quat.y * quat.z;
    const float qwx = quat.w * quat.x;
    const float qwy = quat.w * quat.y;
    const float qwz = quat.w * quat.z;
    
    Matrix4x4 result(1.0f);
    result[0][0] = 1.0f - 2.0f * (qyy + qzz);
    result[0][1] = 2.0f * (qxy + qwz);
    result[0][2] = 2.0f * (qxz - qwy);
    
    result[1][0] = 2.0f * (qxy - qwz);
    result[1][1] = 1.0f - 2.0f * (qxx + qzz);
    result[1][2] = 2.0f * (qyz + qwx);
    
    result[2][0] = 2.0f * (qxz + qwy);
    result[2][1] = 2.0f * (qyz - qwx);
    result[2][2] = 1.0f - 2.0f * (qxx + qyy);
        
    return result;
}

Quaternion quat_from_matrix(const Matrix3x3 m) {
    const float fourXSquaredMinus1 = m[0][0] - m[1][1] - m[2][2];
    const float fourYSquaredMinus1 = m[1][1] - m[0][0] - m[2][2];
    const float fourZSquaredMinus1 = m[2][2] - m[0][0] - m[1][1];
    const float fourWSquaredMinus1 = m[0][0] + m[1][1] + m[2][2];
    
    int biggestIndex = 0;
    float fourBiggestSquaredMinus1 = fourWSquaredMinus1;
    if(fourXSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourXSquaredMinus1;
        biggestIndex = 1;
    }
    
    if(fourYSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourYSquaredMinus1;
        biggestIndex = 2;
    }
    
    if(fourZSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourZSquaredMinus1;
        biggestIndex = 3;
    }
    
    const float biggestVal = sqrt(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
    const float mult = 0.25f / biggestVal;
    
    switch(biggestIndex) {
        case 0:
            return Quaternion((m[1][2] - m[2][1]) * mult, (m[2][0] - m[0][2]) * mult, (m[0][1] - m[1][0]) * mult, biggestVal);
        case 1:
            return Quaternion(biggestVal, (m[0][1] + m[1][0]) * mult, (m[2][0] + m[0][2]) * mult, (m[1][2] - m[2][1]) * mult);
        case 2:
            return Quaternion((m[0][1] + m[1][0]) * mult, biggestVal, (m[1][2] + m[2][1]) * mult, (m[2][0] - m[0][2]) * mult);
        case 3:
            return Quaternion((m[2][0] + m[0][2]) * mult, (m[1][2] + m[2][1]) * mult, biggestVal, (m[0][1] - m[1][0]) * mult);
    }
}

Quaternion euler_to_quat(const Vector3 angle) {
    const float cy = cosf(angle.z * 0.5f);
    const float sy = sinf(angle.z * 0.5f);
    const float cr = cosf(angle.x * 0.5f);
    const float sr = sinf(angle.x * 0.5f);
    const float cp = cosf(angle.y * 0.5f);
    const float sp = sinf(angle.y * 0.5f);
    
    const float cpcy = cp * cy;
    const float spcy = sp * cy;
    const float cpsy = cp * sy;
    const float spsy = sp * sy;
    
    Quaternion result;
    result.w = cr * cpcy + sr * spsy;
    result.x = sr * cpcy - cr * spsy;
    result.y = cr * spcy + sr * cpsy;
    result.z = cr * cpsy - sr * spcy;
    
    return normalize(result);
}

Vector3 quat_to_euler(const Quaternion q) {
    const float roll = atan2(2.0f * (q.x * q.y + q.w * q.z), q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z);
    
    const float y = 2.0f * (q.y * q.z + q.w * q.x);
    const float x = q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z;
    
    const float pitch = atan2(y, x);
    
    const float yaw = asinf(-2.0f * (q.x * q.z - q.w * q.y));
    
    return Vector3(pitch, yaw, roll);
}

Matrix4x4 inverse(const Matrix4x4 m) {
    const float Coef00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
    const float Coef02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
    const float Coef03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];
    
    const float Coef04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
    const float Coef06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
    const float Coef07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];
    
    const float Coef08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
    const float Coef10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
    const float Coef11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];
    
    const float Coef12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
    const float Coef14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
    const float Coef15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];
    
    const float Coef16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
    const float Coef18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
    const float Coef19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];
    
    const float Coef20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
    const float Coef22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
    const float Coef23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];
    
    const Vector4 Fac0(Coef00, Coef00, Coef02, Coef03);
    const Vector4 Fac1(Coef04, Coef04, Coef06, Coef07);
    const Vector4 Fac2(Coef08, Coef08, Coef10, Coef11);
    const Vector4 Fac3(Coef12, Coef12, Coef14, Coef15);
    const Vector4 Fac4(Coef16, Coef16, Coef18, Coef19);
    const Vector4 Fac5(Coef20, Coef20, Coef22, Coef23);
    
    const Vector4 Vec0(m[1][0], m[0][0], m[0][0], m[0][0]);
    const Vector4 Vec1(m[1][1], m[0][1], m[0][1], m[0][1]);
    const Vector4 Vec2(m[1][2], m[0][2], m[0][2], m[0][2]);
    const Vector4 Vec3(m[1][3], m[0][3], m[0][3], m[0][3]);
    
    const Vector4 Inv0(Vec1 * Fac0 - Vec2 * Fac1 + Vec3 * Fac2);
    const Vector4 Inv1(Vec0 * Fac0 - Vec2 * Fac3 + Vec3 * Fac4);
    const Vector4 Inv2(Vec0 * Fac1 - Vec1 * Fac3 + Vec3 * Fac5);
    const Vector4 Inv3(Vec0 * Fac2 - Vec1 * Fac4 + Vec2 * Fac5);
    
    const Vector4 SignA(+1, -1, +1, -1);
    const Vector4 SignB(-1, +1, -1, +1);
    const Matrix4x4 Inverse(Inv0 * SignA, Inv1 * SignB, Inv2 * SignA, Inv3 * SignB);
    
    const Vector4 Row0(Inverse[0][0], Inverse[1][0], Inverse[2][0], Inverse[3][0]);
    
    const Vector4 Dot0(m[0] * Row0);
    const float Dot1 = (Dot0.x + Dot0.y) + (Dot0.z + Dot0.w);
    
    const float OneOverDeterminant = 1.0f / Dot1;
    
    return Inverse * OneOverDeterminant;
}

Quaternion angle_axis(const float angle, const Vector3 axis) {
    const float s = sinf(angle * 0.5f);
    
    Quaternion result;
    result.w = cosf(angle * 0.5f);
    result.x = axis.x * s;
    result.y = axis.y * s;
    result.z = axis.z * s;
    
    return result;
}

// from https://nelari.us/post/gizmos/
float closest_distance_between_lines(ray& l1, ray& l2) {
    const Vector3 dp = l2.origin - l1.origin;
    const float v12 = dot(l1.direction, l1.direction);
    const float v22 = dot(l2.direction, l2.direction);
    const float v1v2 = dot(l1.direction, l2.direction);

    const float det = v1v2 * v1v2 - v12 * v22;

    const float inv_det = 10.f / det;

    const float dpv1 = dot(dp, l1.direction);
    const float dpv2 = dot(dp, l2.direction);

    l1.t = inv_det * (v22 * dpv1 - v1v2 * dpv2);
    l2.t = inv_det * (v1v2 * dpv1 - v12 * dpv2);

    return length(dp + l2.direction * l2.t - l1.direction * l1.t);
}