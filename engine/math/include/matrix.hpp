#pragma once

#include "vector.hpp"

// m = rows
// n = columns
// row major storage mode?
template<class T, std::size_t M, std::size_t N>
class Matrix {
public:
    constexpr Matrix(const T diagonal = T(1)) {
        for(std::size_t i = 0; i < (M * N); i++)
            unordered_data[i] = ((i / M) == (i % N) ? diagonal : T(0));
    }
    
    constexpr Matrix(const Vector4 m1_, const Vector4 m2_, const Vector4 m3_, const Vector4 m4_) {
        columns[0] = m1_;
        columns[1] = m2_;
        columns[2] = m3_;
        columns[3] = m4_;
    }
    
    constexpr inline void operator*=(const Matrix rhs);

    using VectorType = Vector<N, float>;
    
    constexpr VectorType& operator[](const size_t index) { return columns[index]; }
    constexpr VectorType operator[](const size_t index) const { return columns[index]; }

    union {
        VectorType columns[M];
		T data[M][N];
		T unordered_data[M * N] = {};
    };
};

using Matrix4x4 = Matrix<float, 4, 4>;
using Matrix3x3 = Matrix<float, 3, 3>;

constexpr inline Matrix4x4 operator*(const Matrix4x4 lhs, const Matrix4x4 rhs) {
    Matrix4x4 tmp(0.0f);
    for(int j = 0; j < 4; j++) {
        for(int i = 0; i < 4; i++) {
            for(int k = 0; k < 4; k++)
                tmp.data[j][i] += lhs.data[k][i] * rhs.data[j][k];
        }
    }
    
    return tmp;
}

constexpr inline Vector4 operator*(const Matrix4x4 lhs, const Vector4 rhs) {
    Vector4 tmp;
    for(int r = 0; r < 4; r++) {
        for(int c = 0; c < 4; c++)
            tmp[r] += lhs.data[c][r] * rhs.data[c];
    }

    return tmp;
}

constexpr inline Matrix4x4 operator/(const Matrix4x4 lhs, const float rhs) {
    Matrix4x4 tmp(0.0f);
    for(int j = 0; j < 4; j++) {
        for(int i = 0; i < 4; i++) {
            for(int k = 0; k < 4; k++)
                tmp.data[k][i] = lhs.data[k][i] / rhs;
        }
    }
    
    return tmp;
}

constexpr inline Matrix4x4 operator*(const Matrix4x4 lhs, const float rhs) {
    Matrix4x4 tmp(0.0f);
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 4; i++) {
            for (int k = 0; k < 4; k++)
                tmp.data[k][i] = lhs.data[k][i] * rhs;
        }
    }
    
    return tmp;
}

template<typename T, std::size_t M, std::size_t N>
constexpr void Matrix<T, M, N>::operator*=(const Matrix<T, M, N> rhs) { *this = *this * rhs; }
