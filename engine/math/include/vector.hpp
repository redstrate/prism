#pragma once

#include <cmath>
#include <type_traits>
#include <typeinfo>
#include <array>

#define DEFINE_OPERATORS(Size) \
constexpr Vector(const T scalar = T(0)) { \
    for(std::size_t i = 0; i < Size; i++) \
        data[i] = scalar; \
} \
constexpr T& operator[](const size_t index) { \
    return data[index]; \
} \
constexpr T operator[](const size_t index) const { \
    return data[index]; \
} \
constexpr Vector& operator+=(const Vector rhs) { \
    for(std::size_t i = 0; i < Size; i++)\
        data[i] += rhs[i]; \
    return *this; \
} \
constexpr Vector& operator-=(const Vector rhs) { \
    for(std::size_t i = 0; i < Size; i++)\
        data[i] -= rhs[i]; \
    return *this; \
} \
constexpr Vector& operator*=(const Vector rhs) { \
    for(std::size_t i = 0; i < Size; i++) \
        data[i] *= rhs[i]; \
    return *this; \
} \
constexpr Vector& operator/=(const T scalar) { \
    for(std::size_t i = 0; i < Size; i++) \
        data[i] /= scalar; \
    return *this; \
} \
constexpr T* ptr() const { \
    return data.data(); \
} \
constexpr T* ptr() { \
    return data.data(); \
} \
constexpr Vector operator- () const { \
    Vector vec; \
    for(std::size_t i = 0; i < Size; i++) \
        vec[i] = -data[i]; \
    return vec; \
}

template<std::size_t N, class T>
struct Vector {
    std::array<T, N> data;
};

template<class T>
struct Vector<2, T> {
    constexpr Vector(const T x_, const T y_) {
        x = x_;
        y = y_;
    }
    
    DEFINE_OPERATORS(2)
    
    union {
        std::array<T, 2> data;
        
        struct {
            T x, y;
        };
    };
};

template<class T>
struct Vector<3, T> {
    constexpr Vector(const T x_, const T y_, const T z_) {
        x = x_;
        y = y_;
        z = z_;
    }

    DEFINE_OPERATORS(3)
    
    union {
        std::array<T, 3> data;
        
        struct {
            T x, y, z;
        };
    };
};

template<class T>
struct alignas(16) Vector<4, T> {
    constexpr Vector(const T x_, const T y_, const T z_, const T w_) {
        x = x_;
        y = y_;
        z = z_;
        w = w_;
    }
    
    constexpr Vector(const Vector<3, T>& v, const float w = 0.0f) : x(v.x), y(v.y), z(v.z), w(w) {}
    
    DEFINE_OPERATORS(4)
    
    union {
        std::array<T, 4> data;

        struct {
            T x, y, z, w;
        };
        
        struct {
            Vector<3, T> xyz;
            T padding_xyz;
        };
    };
};

using Vector2 = Vector<2, float>;
using Vector3 = Vector<3, float>;
using Vector4 = Vector<4, float>;

template<std::size_t N, class T>
constexpr inline bool operator==(const Vector<N, T> lhs, const Vector<N, T> rhs) {
    bool is_equal = true;
    for(std::size_t i = 0; i < N; i++) {
        if(lhs[i] != rhs[i])
            is_equal = false;
    }
    
    return is_equal;
}

template<std::size_t N, class T>
constexpr inline bool operator!=(const Vector<N, T> lhs, const Vector<N, T> rhs) {
    return !(lhs == rhs);
}

template<std::size_t N, class T>
constexpr inline Vector<N, T> operator-(const Vector<N, T> lhs, const Vector<N, T> rhs) {
    Vector<N, T> vec;
    for(std::size_t i = 0; i < N; i++)
        vec[i] = lhs[i] - rhs[i];
    
    return vec;
}

template<std::size_t N, class T>
constexpr inline Vector<N, T> operator+(const Vector<N, T> lhs, const Vector<N, T> rhs) {
    Vector<N, T> vec;
    for(std::size_t i = 0; i < N; i++)
        vec[i] = lhs[i] + rhs[i];
    
    return vec;
}

template<std::size_t N, class T>
constexpr inline Vector<N, T> operator*(const Vector<N, T> lhs, const Vector<N, T> rhs) {
    Vector<N, T> vec;
    for(std::size_t i = 0; i < N; i++)
        vec[i] = lhs[i] * rhs[i];
    
    return vec;
}

template<std::size_t N, class T>
constexpr inline Vector<N, T> operator+(const Vector<N, T> lhs, const T scalar) {
    Vector<N, T> vec;
    for(std::size_t i = 0; i < N; i++)
        vec[i] = lhs[i] + scalar;
    
    return vec;
}

template<std::size_t N, class T>
constexpr inline Vector<N, T> operator*(const Vector<N, T> lhs, const T scalar) {
    Vector<N, T> vec;
    for(std::size_t i = 0; i < N; i++)
        vec[i] = lhs[i] * scalar;
    
    return vec;
}

template<std::size_t N, class T>
constexpr inline Vector<N, T> operator/(const Vector<N, T> lhs, const Vector<N, T> rhs) {
    Vector<N, T> vec;
    for(std::size_t i = 0; i < N; i++)
        vec[i] = lhs[i] / rhs[i];
    
    return vec;
}

template<std::size_t N, class T>
constexpr inline Vector<N, T> operator/(const Vector<N, T> lhs, const T scalar) {
    Vector<N, T> vec;
    for(std::size_t i = 0; i < N; i++)
        vec[i] = lhs[i] / scalar;
    
    return vec;
}

template<std::size_t N, class T>
constexpr inline T dot(const Vector<N, T> lhs, const Vector<N, T> rhs) {
    T product = T(0.0);
    
    for(std::size_t i = 0; i < N; i++)
        product += lhs[i] * rhs[i];
    
    return product;
}

template<std::size_t N, class T>
constexpr inline T length(const Vector<N, T> vec) {
    return sqrtf(dot(vec, vec));
}

template<std::size_t N, class T>
constexpr inline Vector<N, T> lerp(const Vector<N, T> a, const Vector<N, T> b, const T t) {
    Vector<N, T> vec;
    for(std::size_t i = 0; i < N; i++)
        vec[i] = (T(1) - t) * a[i] + t * b[i];
    
    return vec;
}

template<std::size_t N, class T>
constexpr inline Vector<N, T> normalize(const Vector<N, T> vec) {
    Vector<N, T> result;
    
    const float len = length(vec);
    for(std::size_t i = 0; i < N; i++)
        result[i] = vec[i] / len;
    
    return result;
}

constexpr inline Vector3 cross(const Vector3 u, const Vector3 v) {
    return Vector3(u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x);
}

inline float distance(const Vector3 lhs, const Vector3 rhs) {
	const float diffX = lhs.x - rhs.x;
	const float diffY = lhs.y - rhs.y;
	const float diffZ = lhs.z - rhs.z;

	return std::sqrt((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
}

template<std::size_t N, class T>
constexpr inline T norm(const Vector<N, T> vec) {
    T val = T(0);
    for(std::size_t i = 0; i < N; i++)
        val += abs(vec[i]);
    
    return sqrtf(dot(vec, vec));
}
