#pragma once

#include <array>
#include <cmath>
#include <limits>
#include <type_traits>

#include "Vector.hpp"

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

template <typename T>
using enable_if_numeric_t = typename std::enable_if<std::is_arithmetic<T>::value, T>::type;

template <typename T, std::size_t Rows, std::size_t Cols>
class Matrix {
    static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");
    std::array<std::array<T, Cols>, Rows> data_;

   public:
    using value_type = T;
    static constexpr std::size_t rows = Rows;
    static constexpr std::size_t cols = Cols;

    constexpr Matrix() = default;

    template <typename... Args>
    explicit constexpr Matrix(Args... args) {
        static_assert(sizeof...(Args) == Rows * Cols, "Number of arguments must match matrix size");
        std::array<T, Rows * Cols> temp{static_cast<T>(args)...};
        for (std::size_t row = 0; row < Rows; ++row) {
            for (std::size_t col = 0; col < Cols; ++col) {
                data_[row][col] = temp[(row * Cols) + col];
            }
        }
    }

    constexpr std::array<T, Cols>& operator[](std::size_t row) noexcept { return data_[row]; }

    constexpr const std::array<T, Cols>& operator[](std::size_t row) const noexcept {
        return data_[row];
    }

    constexpr T& at(std::size_t row, std::size_t col) noexcept { return data_[row][col]; }

    constexpr const T& at(std::size_t row, std::size_t col) const noexcept {
        return data_[row][col];
    }

    // Matrix operations
    constexpr Matrix operator+(const Matrix& other) const noexcept {
        Matrix result;
        for (std::size_t row = 0; row < Rows; ++row) {
            for (std::size_t col = 0; col < Cols; ++col) {
                result[row][col] = data_[row][col] + other[row][col];
            }
        }
        return result;
    }

    constexpr Matrix operator-(const Matrix& other) const noexcept {
        Matrix result;
        for (std::size_t row = 0; row < Rows; ++row) {
            for (std::size_t col = 0; col < Cols; ++col) {
                result[row][col] = data_[row][col] - other[row][col];
            }
        }
        return result;
    }

    constexpr Matrix operator*(T scalar) const noexcept {
        Matrix result;
        for (std::size_t row = 0; row < Rows; ++row) {
            for (std::size_t col = 0; col < Cols; ++col) {
                result[row][col] = data_[row][col] * scalar;
            }
        }
        return result;
    }

    template <std::size_t OtherCols>
    constexpr Matrix<T, Rows, OtherCols> operator*(
        const Matrix<T, Cols, OtherCols>& other) const noexcept {
        Matrix<T, Rows, OtherCols> result{};
        for (std::size_t row = 0; row < Rows; ++row) {
            for (std::size_t col = 0; col < OtherCols; ++col) {
                for (std::size_t idx = 0; idx < Cols; ++idx) {
                    result[row][col] += data_[row][idx] * other[idx][col];
                }
            }
        }
        return result;
    }

    // Matrix-vector multiplication
    constexpr Vector<T, Rows> operator*(const Vector<T, Cols>& vec) const noexcept {
        Vector<T, Rows> result{};
        for (std::size_t row = 0; row < Rows; ++row) {
            for (std::size_t col = 0; col < Cols; ++col) {
                result[row] += data_[row][col] * vec[col];
            }
        }
        return result;
    }

    // Transpose
    constexpr Matrix<T, Cols, Rows> transpose() const noexcept {
        Matrix<T, Cols, Rows> result;
        for (std::size_t row = 0; row < Rows; ++row) {
            for (std::size_t col = 0; col < Cols; ++col) {
                result[col][row] = data_[row][col];
            }
        }
        return result;
    }

    // Identity matrix (square matrices only)
    template <std::size_t R = Rows, std::size_t C = Cols>
    static constexpr typename std::enable_if<R == C, Matrix>::type identity() noexcept {
        Matrix result{};
        for (std::size_t idx = 0; idx < Rows; ++idx) {
            result[idx][idx] = T{1};
        }
        return result;
    }

    // Determinant (1x1, 2x2, 3x3, 4x4)
    template <std::size_t R = Rows, std::size_t C = Cols>
    constexpr typename std::enable_if<R == C, T>::type determinant() const noexcept {
        if (Rows == 1) {
            return data_[0][0];
        }
        if (Rows == 2) {
            return (data_[0][0] * data_[1][1]) - (data_[0][1] * data_[1][0]);
        }
        if (Rows == 3) {
            return data_[0][0] * ((data_[1][1] * data_[2][2]) - (data_[1][2] * data_[2][1])) -
                   data_[0][1] * ((data_[1][0] * data_[2][2]) - (data_[1][2] * data_[2][0])) +
                   data_[0][2] * ((data_[1][0] * data_[2][1]) - (data_[1][1] * data_[2][0]));
        }
        if (Rows == 4) {
            // Manually compute 4x4 determinant using cofactor expansion
            T a00 = data_[0][0];
            T a01 = data_[0][1];
            T a02 = data_[0][2];
            T a03 = data_[0][3];
            T a10 = data_[1][0];
            T a11 = data_[1][1];
            T a12 = data_[1][2];
            T a13 = data_[1][3];
            T a20 = data_[2][0];
            T a21 = data_[2][1];
            T a22 = data_[2][2];
            T a23 = data_[2][3];
            T a30 = data_[3][0];
            T a31 = data_[3][1];
            T a32 = data_[3][2];
            T a33 = data_[3][3];
            
            T det3x3_0 = a11 * (a22 * a33 - a23 * a32) - a12 * (a21 * a33 - a23 * a31) + a13 * (a21 * a32 - a22 * a31);
            T det3x3_1 = a10 * (a22 * a33 - a23 * a32) - a12 * (a20 * a33 - a23 * a30) + a13 * (a20 * a32 - a22 * a30);
            T det3x3_2 = a10 * (a21 * a33 - a23 * a31) - a11 * (a20 * a33 - a23 * a30) + a13 * (a20 * a31 - a21 * a30);
            T det3x3_3 = a10 * (a21 * a32 - a22 * a31) - a11 * (a20 * a32 - a22 * a30) + a12 * (a20 * a31 - a21 * a30);
            
            return a00 * det3x3_0 - a01 * det3x3_1 + a02 * det3x3_2 - a03 * det3x3_3;
        }
        return T{0};
    }

   private:
    template<std::size_t R = Rows, std::size_t C = Cols>
        requires (R > 1 && C > 1)
    constexpr Matrix<T, R - 1, C - 1> get_minor(std::size_t skip_row,
                                                 std::size_t skip_col) const noexcept {
        Matrix<T, R - 1, C - 1> minor;
        std::size_t minor_row = 0;
        for (std::size_t row = 0; row < R; ++row) {
            if (row == skip_row) {
                continue;
            }
            std::size_t minor_col = 0;
            for (std::size_t col = 0; col < C; ++col) {
                if (col == skip_col) {
                    continue;
                }
                minor[minor_row][minor_col] = data_[row][col];
                ++minor_col;
            }
            ++minor_row;
        }
        return minor;
    }

   public:
    // Matrix inverse (4x4 only for now)
    template <std::size_t R = Rows, std::size_t C = Cols>
    typename std::enable_if<R == 4 && C == 4, Matrix>::type inverse() const noexcept {
        // Simple inverse implementation for 4x4 matrices
        // This is a basic implementation - could be optimized
        T det = determinant();
        if (std::abs(det) < std::numeric_limits<T>::epsilon()) {
            return Matrix::identity(); // Return identity for singular matrices
        }
        
        Matrix result;
        // Compute adjugate matrix and divide by determinant
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t col = 0; col < 4; ++col) {
                auto minor = get_minor(row, col);
                T sign = ((row + col) % 2 == 0) ? T{1} : T{-1};
                result[col][row] = (sign * minor.determinant()) / det; // Note: transposed
            }
        }
        return result;
    }

    // Transform point (4x4 matrix with 3D point)
    template <std::size_t R = Rows, std::size_t C = Cols>
    typename std::enable_if<R == 4 && C == 4, Vector<T, 3>>::type 
    transform_point(const Vector<T, 3>& point) const noexcept {
        Vector<T, 4> homogeneous{point.x(), point.y(), point.z(), T{1}};
        Vector<T, 4> result = (*this) * homogeneous;
        // Perspective divide
        if (std::abs(result[3]) > std::numeric_limits<T>::epsilon()) {
            return Vector<T, 3>{result[0] / result[3], result[1] / result[3], result[2] / result[3]};
        }
        return Vector<T, 3>{result[0], result[1], result[2]};
    }

    // Static factory methods for common transformations
    template <std::size_t R = Rows, std::size_t C = Cols>
    static typename std::enable_if<R == 4 && C == 4, Matrix>::type 
    look_at(const Vector<T, 3>& eye_position, const Vector<T, 3>& target_center, const Vector<T, 3>& up_vector) noexcept;

    template <std::size_t R = Rows, std::size_t C = Cols>
    static typename std::enable_if<R == 4 && C == 4, Matrix>::type 
    orthographic(T left_bound, T right_bound, T bottom_bound, T top_bound, T near_plane, T far_plane) noexcept;

};

// Type aliases
using Mat2f = Matrix<float, 2, 2>;
using Mat3f = Matrix<float, 3, 3>;
using Mat4f = Matrix<float, 4, 4>;
using Mat2d = Matrix<double, 2, 2>;
using Mat3d = Matrix<double, 3, 3>;
using Mat4d = Matrix<double, 4, 4>;

// Implementation of static methods
template <typename T, std::size_t Rows, std::size_t Cols>
template <std::size_t R, std::size_t C>
typename std::enable_if<R == 4 && C == 4, Matrix<T, Rows, Cols>>::type 
Matrix<T, Rows, Cols>::look_at(const Vector<T, 3>& eye_position, const Vector<T, 3>& target_center, const Vector<T, 3>& up_vector) noexcept {
    Vector<T, 3> forward = (target_center - eye_position).normalized();
    Vector<T, 3> up_norm = up_vector.normalized();
    Vector<T, 3> side = forward.cross(up_norm).normalized();
    up_norm = side.cross(forward);

    Matrix<T, 4, 4> mat = Matrix<T, 4, 4>::identity();
    mat[0][0] = side.x();
    mat[0][1] = side.y();
    mat[0][2] = side.z();
    mat[0][3] = -side.dot(eye_position);
    mat[1][0] = up_norm.x();
    mat[1][1] = up_norm.y();
    mat[1][2] = up_norm.z();
    mat[1][3] = -up_norm.dot(eye_position);
    mat[2][0] = -forward.x();
    mat[2][1] = -forward.y();
    mat[2][2] = -forward.z();
    mat[2][3] = forward.dot(eye_position);
    return mat;
}

template <typename T, std::size_t Rows, std::size_t Cols>
template <std::size_t R, std::size_t C>
typename std::enable_if<R == 4 && C == 4, Matrix<T, Rows, Cols>>::type 
Matrix<T, Rows, Cols>::orthographic(T left_bound, T right_bound, T bottom_bound, T top_bound, T near_plane, T far_plane) noexcept {
    Matrix<T, 4, 4> mat = Matrix<T, 4, 4>::identity();
    mat[0][0] = T{2} / (right_bound - left_bound);
    mat[1][1] = T{2} / (top_bound - bottom_bound);
    mat[2][2] = -T{2} / (far_plane - near_plane);
    mat[0][3] = -(right_bound + left_bound) / (right_bound - left_bound);
    mat[1][3] = -(top_bound + bottom_bound) / (top_bound - bottom_bound);
    mat[2][3] = -(far_plane + near_plane) / (far_plane - near_plane);
    return mat;
}

// Transform matrices
template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, Matrix<T, 4, 4>>::
    type constexpr translation(const Vector<T, 3>& offset) noexcept {
    auto mat = Matrix<T, 4, 4>::identity();
    mat[0][3] = offset.x();
    mat[1][3] = offset.y();
    mat[2][3] = offset.z();
    return mat;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, Matrix<T, 4, 4>>::type rotation_x(
    T angle) noexcept {
    auto mat = Matrix<T, 4, 4>::identity();
    T cosine = std::cos(angle);
    T sine = std::sin(angle);
    mat[1][1] = cosine;
    mat[1][2] = -sine;
    mat[2][1] = sine;
    mat[2][2] = cosine;
    return mat;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, Matrix<T, 4, 4>>::type rotation_y(
    T angle) noexcept {
    auto mat = Matrix<T, 4, 4>::identity();
    T cosine = std::cos(angle);
    T sine = std::sin(angle);
    mat[0][0] = cosine;
    mat[0][2] = sine;
    mat[2][0] = -sine;
    mat[2][2] = cosine;
    return mat;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, Matrix<T, 4, 4>>::type rotation_z(
    T angle) noexcept {
    auto mat = Matrix<T, 4, 4>::identity();
    T cosine = std::cos(angle);
    T sine = std::sin(angle);
    mat[0][0] = cosine;
    mat[0][1] = -sine;
    mat[1][0] = sine;
    mat[1][1] = cosine;
    return mat;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, Matrix<T, 4, 4>>::type constexpr scale(
    const Vector<T, 3>& factors) noexcept {
    auto mat = Matrix<T, 4, 4>::identity();
    mat[0][0] = factors.x();
    mat[1][1] = factors.y();
    mat[2][2] = factors.z();
    return mat;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, Matrix<T, 4, 4>>::type look_at(
    const Vector<T, 3>& eye_position, const Vector<T, 3>& target_center, const Vector<T, 3>& up_vector) noexcept {
    Vector<T, 3> forward = (target_center - eye_position).normalized();
    Vector<T, 3> up_norm = up_vector.normalized();
    Vector<T, 3> side = forward.cross(up_norm).normalized();
    up_norm = side.cross(forward);

    Matrix<T, 4, 4> mat = Matrix<T, 4, 4>::identity();
    mat[0][0] = side.x();
    mat[0][1] = side.y();
    mat[0][2] = side.z();
    mat[0][3] = -side.dot(eye_position);
    mat[1][0] = up_norm.x();
    mat[1][1] = up_norm.y();
    mat[1][2] = up_norm.z();
    mat[1][3] = -up_norm.dot(eye_position);
    mat[2][0] = -forward.x();
    mat[2][1] = -forward.y();
    mat[2][2] = -forward.z();
    mat[2][3] = forward.dot(eye_position);
    return mat;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, Matrix<T, 4, 4>>::type perspective(
    T field_of_view_y, T aspect_ratio, T near_plane, T far_plane) noexcept {
    T tan_half_fovy = std::tan(field_of_view_y / T{2});
    Matrix<T, 4, 4> mat{};
    mat[0][0] = T{1} / (aspect_ratio * tan_half_fovy);
    mat[1][1] = T{1} / tan_half_fovy;
    mat[2][2] = -(far_plane + near_plane) / (far_plane - near_plane);
    mat[2][3] = -(T{2} * far_plane * near_plane) / (far_plane - near_plane);
    mat[3][2] = -T{1};
    return mat;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, Matrix<T, 4, 4>>::type orthographic(
    T left_bound, T right_bound, T bottom_bound, T top_bound, T near_plane, T far_plane) noexcept {
    Matrix<T, 4, 4> mat = Matrix<T, 4, 4>::identity();
    mat[0][0] = T{2} / (right_bound - left_bound);
    mat[1][1] = T{2} / (top_bound - bottom_bound);
    mat[2][2] = -T{2} / (far_plane - near_plane);
    mat[0][3] = -(right_bound + left_bound) / (right_bound - left_bound);
    mat[1][3] = -(top_bound + bottom_bound) / (top_bound - bottom_bound);
    mat[2][3] = -(far_plane + near_plane) / (far_plane - near_plane);
    return mat;
}

// Scalar multiplication
template <typename T, std::size_t Rows, std::size_t Cols>
constexpr Matrix<T, Rows, Cols> operator*(T scalar, const Matrix<T, Rows, Cols>& mat) noexcept {
    static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");
    return mat * scalar;
}

}  // namespace Math
}  // namespace Core
}  // namespace Manifest
