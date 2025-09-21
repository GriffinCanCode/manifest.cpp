#pragma once

#include <array>
#include <cmath>
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
    constexpr Matrix(Args... args,
                     typename std::enable_if<sizeof...(Args) == Rows * Cols>::type* = nullptr) {
        std::array<T, Rows * Cols> temp{static_cast<T>(args)...};
        for (std::size_t r = 0; r < Rows; ++r) {
            for (std::size_t c = 0; c < Cols; ++c) {
                data_[r][c] = temp[r * Cols + c];
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
        for (std::size_t r = 0; r < Rows; ++r) {
            for (std::size_t c = 0; c < Cols; ++c) {
                result[r][c] = data_[r][c] + other[r][c];
            }
        }
        return result;
    }

    constexpr Matrix operator-(const Matrix& other) const noexcept {
        Matrix result;
        for (std::size_t r = 0; r < Rows; ++r) {
            for (std::size_t c = 0; c < Cols; ++c) {
                result[r][c] = data_[r][c] - other[r][c];
            }
        }
        return result;
    }

    constexpr Matrix operator*(T scalar) const noexcept {
        Matrix result;
        for (std::size_t r = 0; r < Rows; ++r) {
            for (std::size_t c = 0; c < Cols; ++c) {
                result[r][c] = data_[r][c] * scalar;
            }
        }
        return result;
    }

    template <std::size_t OtherCols>
    constexpr Matrix<T, Rows, OtherCols> operator*(
        const Matrix<T, Cols, OtherCols>& other) const noexcept {
        Matrix<T, Rows, OtherCols> result{};
        for (std::size_t r = 0; r < Rows; ++r) {
            for (std::size_t c = 0; c < OtherCols; ++c) {
                for (std::size_t k = 0; k < Cols; ++k) {
                    result[r][c] += data_[r][k] * other[k][c];
                }
            }
        }
        return result;
    }

    // Matrix-vector multiplication
    constexpr Vector<T, Rows> operator*(const Vector<T, Cols>& vec) const noexcept {
        Vector<T, Rows> result{};
        for (std::size_t r = 0; r < Rows; ++r) {
            for (std::size_t c = 0; c < Cols; ++c) {
                result[r] += data_[r][c] * vec[c];
            }
        }
        return result;
    }

    // Transpose
    constexpr Matrix<T, Cols, Rows> transpose() const noexcept {
        Matrix<T, Cols, Rows> result;
        for (std::size_t r = 0; r < Rows; ++r) {
            for (std::size_t c = 0; c < Cols; ++c) {
                result[c][r] = data_[r][c];
            }
        }
        return result;
    }

    // Identity matrix (square matrices only)
    template <std::size_t R = Rows, std::size_t C = Cols>
    static constexpr typename std::enable_if<R == C, Matrix>::type identity() noexcept {
        Matrix result{};
        for (std::size_t i = 0; i < Rows; ++i) {
            result[i][i] = T{1};
        }
        return result;
    }

    // Determinant (2x2, 3x3, 4x4)
    template <std::size_t R = Rows, std::size_t C = Cols>
    constexpr typename std::enable_if<R == C, T>::type determinant() const noexcept {
        if (Rows == 2) {
            return data_[0][0] * data_[1][1] - data_[0][1] * data_[1][0];
        } else if (Rows == 3) {
            return data_[0][0] * (data_[1][1] * data_[2][2] - data_[1][2] * data_[2][1]) -
                   data_[0][1] * (data_[1][0] * data_[2][2] - data_[1][2] * data_[2][0]) +
                   data_[0][2] * (data_[1][0] * data_[2][1] - data_[1][1] * data_[2][0]);
        } else if (Rows == 4) {
            T det = T{0};
            for (std::size_t i = 0; i < 4; ++i) {
                Matrix<T, 3, 3> minor = get_minor(0, i);
                T cofactor = (i % 2 == 0 ? T{1} : T{-1}) * data_[0][i] * minor.determinant();
                det += cofactor;
            }
            return det;
        }
    }

   private:
    constexpr Matrix<T, Rows - 1, Cols - 1> get_minor(std::size_t skip_row,
                                                      std::size_t skip_col) const noexcept {
        Matrix<T, Rows - 1, Cols - 1> minor;
        std::size_t minor_row = 0;
        for (std::size_t r = 0; r < Rows; ++r) {
            if (r == skip_row) continue;
            std::size_t minor_col = 0;
            for (std::size_t c = 0; c < Cols; ++c) {
                if (c == skip_col) continue;
                minor[minor_row][minor_col] = data_[r][c];
                ++minor_col;
            }
            ++minor_row;
        }
        return minor;
    }
};

// Type aliases
using Mat2f = Matrix<float, 2, 2>;
using Mat3f = Matrix<float, 3, 3>;
using Mat4f = Matrix<float, 4, 4>;
using Mat2d = Matrix<double, 2, 2>;
using Mat3d = Matrix<double, 3, 3>;
using Mat4d = Matrix<double, 4, 4>;

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
    T c = std::cos(angle);
    T s = std::sin(angle);
    mat[1][1] = c;
    mat[1][2] = -s;
    mat[2][1] = s;
    mat[2][2] = c;
    return mat;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, Matrix<T, 4, 4>>::type rotation_y(
    T angle) noexcept {
    auto mat = Matrix<T, 4, 4>::identity();
    T c = std::cos(angle);
    T s = std::sin(angle);
    mat[0][0] = c;
    mat[0][2] = s;
    mat[2][0] = -s;
    mat[2][2] = c;
    return mat;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, Matrix<T, 4, 4>>::type rotation_z(
    T angle) noexcept {
    auto mat = Matrix<T, 4, 4>::identity();
    T c = std::cos(angle);
    T s = std::sin(angle);
    mat[0][0] = c;
    mat[0][1] = -s;
    mat[1][0] = s;
    mat[1][1] = c;
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
    const Vector<T, 3>& eye, const Vector<T, 3>& center, const Vector<T, 3>& up) noexcept {
    Vector<T, 3> f = (center - eye).normalized();
    Vector<T, 3> u = up.normalized();
    Vector<T, 3> s = f.cross(u).normalized();
    u = s.cross(f);

    Matrix<T, 4, 4> mat = Matrix<T, 4, 4>::identity();
    mat[0][0] = s.x();
    mat[0][1] = s.y();
    mat[0][2] = s.z();
    mat[0][3] = -s.dot(eye);
    mat[1][0] = u.x();
    mat[1][1] = u.y();
    mat[1][2] = u.z();
    mat[1][3] = -u.dot(eye);
    mat[2][0] = -f.x();
    mat[2][1] = -f.y();
    mat[2][2] = -f.z();
    mat[2][3] = f.dot(eye);
    return mat;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, Matrix<T, 4, 4>>::type perspective(
    T fovy, T aspect, T near, T far) noexcept {
    T tan_half_fovy = std::tan(fovy / T{2});
    Matrix<T, 4, 4> mat{};
    mat[0][0] = T{1} / (aspect * tan_half_fovy);
    mat[1][1] = T{1} / tan_half_fovy;
    mat[2][2] = -(far + near) / (far - near);
    mat[2][3] = -(T{2} * far * near) / (far - near);
    mat[3][2] = -T{1};
    return mat;
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, Matrix<T, 4, 4>>::type orthographic(
    T left, T right, T bottom, T top, T near, T far) noexcept {
    Matrix<T, 4, 4> mat = Matrix<T, 4, 4>::identity();
    mat[0][0] = T{2} / (right - left);
    mat[1][1] = T{2} / (top - bottom);
    mat[2][2] = -T{2} / (far - near);
    mat[0][3] = -(right + left) / (right - left);
    mat[1][3] = -(top + bottom) / (top - bottom);
    mat[2][3] = -(far + near) / (far - near);
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
