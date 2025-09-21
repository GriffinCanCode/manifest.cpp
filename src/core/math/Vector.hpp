#pragma once

#include <array>
#include <cmath>
#include <type_traits>

// Only include SIMD intrinsics on x86/x64 architectures
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#include <immintrin.h>
#define HAS_SIMD_SUPPORT 1
#else
#define HAS_SIMD_SUPPORT 0
#endif

namespace Manifest {
namespace Core {
namespace Math {

template<typename T, std::size_t N>
class Vector {
    static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");
    static_assert(N >= 2 && N <= 4, "Vector dimension must be between 2 and 4");
    std::array<T, N> data_;
    
public:
    using value_type = T;
    static constexpr std::size_t size = N;
    
    constexpr Vector() = default;
    
    template<typename... Args>
    constexpr Vector(Args... args) : data_{static_cast<T>(args)...} {
        static_assert(sizeof...(Args) == N, "Number of arguments must match vector dimension");
    }
    
    constexpr Vector(T value) {
        data_.fill(value);
    }
    
    constexpr T& operator[](std::size_t index) noexcept { return data_[index]; }
    constexpr const T& operator[](std::size_t index) const noexcept { return data_[index]; }
    
    constexpr T* data() noexcept { return data_.data(); }
    constexpr const T* data() const noexcept { return data_.data(); }
    
    // Component accessors
    constexpr T& x() noexcept { 
        static_assert(N >= 1, "Vector must have at least 1 dimension for x()");
        return data_[0]; 
    }
    constexpr const T& x() const noexcept { 
        static_assert(N >= 1, "Vector must have at least 1 dimension for x()");
        return data_[0]; 
    }
    
    constexpr T& y() noexcept { 
        static_assert(N >= 2, "Vector must have at least 2 dimensions for y()");
        return data_[1]; 
    }
    constexpr const T& y() const noexcept { 
        static_assert(N >= 2, "Vector must have at least 2 dimensions for y()");
        return data_[1]; 
    }
    
    constexpr T& z() noexcept { 
        static_assert(N >= 3, "Vector must have at least 3 dimensions for z()");
        return data_[2]; 
    }
    constexpr const T& z() const noexcept { 
        static_assert(N >= 3, "Vector must have at least 3 dimensions for z()");
        return data_[2]; 
    }
    
    constexpr T& w() noexcept { 
        static_assert(N >= 4, "Vector must have 4 dimensions for w()");
        return data_[3]; 
    }
    constexpr const T& w() const noexcept { 
        static_assert(N >= 4, "Vector must have 4 dimensions for w()");
        return data_[3]; 
    }
    
    // Arithmetic operations
    constexpr Vector operator+(const Vector& other) const noexcept {
        Vector result;
        for (std::size_t i = 0; i < N; ++i) {
            result[i] = data_[i] + other[i];
        }
        return result;
    }
    
    constexpr Vector operator-(const Vector& other) const noexcept {
        Vector result;
        for (std::size_t i = 0; i < N; ++i) {
            result[i] = data_[i] - other[i];
        }
        return result;
    }
    
    constexpr Vector operator*(T scalar) const noexcept {
        Vector result;
        for (std::size_t i = 0; i < N; ++i) {
            result[i] = data_[i] * scalar;
        }
        return result;
    }
    
    constexpr Vector operator/(T scalar) const noexcept {
        Vector result;
        for (std::size_t i = 0; i < N; ++i) {
            result[i] = data_[i] / scalar;
        }
        return result;
    }
    
    constexpr Vector& operator+=(const Vector& other) noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            data_[i] += other[i];
        }
        return *this;
    }
    
    constexpr Vector& operator-=(const Vector& other) noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            data_[i] -= other[i];
        }
        return *this;
    }
    
    constexpr Vector& operator*=(T scalar) noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            data_[i] *= scalar;
        }
        return *this;
    }
    
    constexpr Vector& operator/=(T scalar) noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            data_[i] /= scalar;
        }
        return *this;
    }
    
    // Vector operations
    constexpr T dot(const Vector& other) const noexcept {
        T result{};
        for (std::size_t i = 0; i < N; ++i) {
            result += data_[i] * other[i];
        }
        return result;
    }
    
    constexpr T length_squared() const noexcept {
        return dot(*this);
    }
    
    template<typename U = T>
    typename std::enable_if<std::is_floating_point<U>::value, T>::type 
    length() const noexcept {
        return std::sqrt(length_squared());
    }
    
    template<typename U = T>
    typename std::enable_if<std::is_floating_point<U>::value, Vector>::type 
    normalized() const noexcept {
        T len = length();
        return len > T{0} ? (*this / len) : Vector{};
    }
    
    template<typename U = T>
    typename std::enable_if<std::is_floating_point<U>::value, void>::type 
    normalize() noexcept {
        T len = length();
        if (len > T{0}) {
            *this /= len;
        }
    }
    
    // Cross product (3D only)
    template<std::size_t M = N>
    typename std::enable_if<M == 3, Vector>::type 
    cross(const Vector& other) const noexcept {
        return Vector{
            data_[1] * other[2] - data_[2] * other[1],
            data_[2] * other[0] - data_[0] * other[2],
            data_[0] * other[1] - data_[1] * other[0]
        };
    }
    
    // Comparison
    constexpr bool operator==(const Vector& other) const noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            if (data_[i] != other.data_[i]) return false;
        }
        return true;
    }
    
    // Swizzling helpers
    template<std::size_t M = N>
    typename std::enable_if<M >= 2, Vector<T, 2>>::type 
    xy() const noexcept {
        return {data_[0], data_[1]};
    }
    
    template<std::size_t M = N>
    typename std::enable_if<M >= 3, Vector<T, 3>>::type 
    xyz() const noexcept {
        return {data_[0], data_[1], data_[2]};
    }
};

// Type aliases
using Vec2f = Vector<float, 2>;
using Vec3f = Vector<float, 3>;
using Vec4f = Vector<float, 4>;
using Vec2d = Vector<double, 2>;
using Vec3d = Vector<double, 3>;
using Vec4d = Vector<double, 4>;
using Vec2i = Vector<int, 2>;
using Vec3i = Vector<int, 3>;
using Vec4i = Vector<int, 4>;

// SIMD specialization for Vec4f (only on supported architectures)
#if HAS_SIMD_SUPPORT
template<>
class Vector<float, 4> {
    __m128 data_;
    
public:
    using value_type = float;
    static constexpr std::size_t size = 4;
    
    Vector() : data_{_mm_setzero_ps()} {}
    Vector(float x, float y, float z, float w) : data_{_mm_set_ps(w, z, y, x)} {}
    explicit Vector(float value) : data_{_mm_set1_ps(value)} {}
    explicit Vector(__m128 data) : data_{data} {}
    
    float operator[](std::size_t index) const noexcept {
        alignas(16) float temp[4];
        _mm_store_ps(temp, data_);
        return temp[index];
    }
    
    Vector operator+(const Vector& other) const noexcept {
        return Vector{_mm_add_ps(data_, other.data_)};
    }
    
    Vector operator-(const Vector& other) const noexcept {
        return Vector{_mm_sub_ps(data_, other.data_)};
    }
    
    Vector operator*(float scalar) const noexcept {
        return Vector{_mm_mul_ps(data_, _mm_set1_ps(scalar))};
    }
    
    Vector operator/(float scalar) const noexcept {
        return Vector{_mm_div_ps(data_, _mm_set1_ps(scalar))};
    }
    
    Vector& operator+=(const Vector& other) noexcept {
        data_ = _mm_add_ps(data_, other.data_);
        return *this;
    }
    
    Vector& operator-=(const Vector& other) noexcept {
        data_ = _mm_sub_ps(data_, other.data_);
        return *this;
    }
    
    Vector& operator*=(float scalar) noexcept {
        data_ = _mm_mul_ps(data_, _mm_set1_ps(scalar));
        return *this;
    }
    
    Vector& operator/=(float scalar) noexcept {
        data_ = _mm_div_ps(data_, _mm_set1_ps(scalar));
        return *this;
    }
    
    float dot(const Vector& other) const noexcept {
        __m128 mul = _mm_mul_ps(data_, other.data_);
        __m128 hadd1 = _mm_hadd_ps(mul, mul);
        __m128 hadd2 = _mm_hadd_ps(hadd1, hadd1);
        return _mm_cvtss_f32(hadd2);
    }
    
    float length_squared() const noexcept {
        return dot(*this);
    }
    
    float length() const noexcept {
        return std::sqrt(length_squared());
    }
    
    Vector normalized() const noexcept {
        float len = length();
        return len > 0.0f ? (*this / len) : Vector{};
    }
    
    void normalize() noexcept {
        float len = length();
        if (len > 0.0f) {
            *this /= len;
        }
    }
    
    __m128 data() const noexcept { return data_; }
};
#endif // HAS_SIMD_SUPPORT

// Scalar multiplication from left
template<typename T, std::size_t N>
constexpr Vector<T, N> operator*(T scalar, const Vector<T, N>& vec) noexcept {
    return vec * scalar;
}

// Distance functions
template<typename T, std::size_t N>
constexpr T distance_squared(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    return (b - a).length_squared();
}

template<typename T, std::size_t N>
typename std::enable_if<std::is_floating_point<T>::value, T>::type
distance(const Vector<T, N>& a, const Vector<T, N>& b) noexcept {
    return (b - a).length();
}

// Lerp
template<typename T, std::size_t N>
constexpr Vector<T, N> lerp(const Vector<T, N>& a, const Vector<T, N>& b, T t) noexcept {
    static_assert(std::is_floating_point<T>::value, "T must be a floating point type for lerp");
    return a + (b - a) * t;
}

} // namespace Math
} // namespace Core  
} // namespace Manifest
