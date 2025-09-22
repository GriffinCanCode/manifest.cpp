#pragma once

// Modern C++23 standard library features
// Include this header for consistent access to modern features

#include <cstddef>     // For std::byte and std::size_t  
#include <algorithm>
#include <string>
#include <array>
#include <vector>
#include <type_traits>
#include <utility>     // For std::move, std::forward
#include <span>        // For std::span (C++20)
#include <expected>    // For std::expected (C++23)

namespace Manifest {
namespace Core {
namespace Modern {

// std::byte compatibility for C++23
#if __cplusplus >= 202302L && __has_include(<cstddef>)
    using byte = std::byte;
#else
    // Fallback for older compilers or when std::byte is not available
    using byte = unsigned char;
#endif

// C++23 compatible Result implementation (similar to std::expected)
template <typename T, typename E>
class Result {
private:
    bool has_value_;
    union {
        T value_;
        E error_;
    };

    void destroy_value() {
        if constexpr (!std::is_trivially_destructible<T>::value) {
            value_.~T();
        }
    }

    void destroy_error() {
        if constexpr (!std::is_trivially_destructible<E>::value) {
            error_.~E();
        }
    }

public:
    // Default constructor - constructs value  
    template<typename = std::enable_if_t<std::is_default_constructible<T>::value>>
    Result() : has_value_(true), value_() {}

    // Value constructors
    Result(const T& value) : has_value_(true), value_(value) {}
    Result(T&& value) : has_value_(true), value_(std::move(value)) {}

    // Error constructors  
    Result(const E& error) : has_value_(false), error_(error) {}
    Result(E&& error) : has_value_(false), error_(std::move(error)) {}

    // Copy constructor
    Result(const Result& other) : has_value_(other.has_value_) {
        if (has_value_) {
            new (&value_) T(other.value_);
        } else {
            new (&error_) E(other.error_);
        }
    }

    // Move constructor
    Result(Result&& other) noexcept : has_value_(other.has_value_) {
        if (has_value_) {
            new (&value_) T(std::move(other.value_));
        } else {
            new (&error_) E(std::move(other.error_));
        }
    }

    // Copy assignment
    Result& operator=(const Result& other) {
        if (this != &other) {
            if (has_value_ && other.has_value_) {
                value_ = other.value_;
            } else if (!has_value_ && !other.has_value_) {
                error_ = other.error_;
            } else if (has_value_ && !other.has_value_) {
                destroy_value();
                new (&error_) E(other.error_);
                has_value_ = false;
            } else {
                destroy_error();
                new (&value_) T(other.value_);
                has_value_ = true;
            }
        }
        return *this;
    }

    // Move assignment
    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            if (has_value_ && other.has_value_) {
                value_ = std::move(other.value_);
            } else if (!has_value_ && !other.has_value_) {
                error_ = std::move(other.error_);
            } else if (has_value_ && !other.has_value_) {
                destroy_value();
                new (&error_) E(std::move(other.error_));
                has_value_ = false;
            } else {
                destroy_error();
                new (&value_) T(std::move(other.value_));
                has_value_ = true;
            }
        }
        return *this;
    }

    ~Result() {
        if (has_value_) {
            destroy_value();
        } else {
            destroy_error();
        }
    }

    [[nodiscard]] bool has_value() const noexcept { return has_value_; }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value_; }

    [[nodiscard]] const T& value() const& { return value_; }
    [[nodiscard]] T& value() & { return value_; }
    [[nodiscard]] T&& value() && { return std::move(value_); }
    
    [[nodiscard]] const T& operator*() const& { return value_; }
    [[nodiscard]] T& operator*() & { return value_; }
    [[nodiscard]] T&& operator*() && { return std::move(value_); }

    [[nodiscard]] const E& error() const& { return error_; }
    [[nodiscard]] E& error() & { return error_; }
    [[nodiscard]] E&& error() && { return std::move(error_); }
};

// Specialization for void types
template <typename E>
class Result<void, E> {
private:
    bool has_value_;
    union {
        char dummy_;  // Dummy for successful void case
        E error_;
    };

    void destroy_error() {
        if constexpr (!std::is_trivially_destructible<E>::value) {
            error_.~E();
        }
    }

public:
    // Default constructor for void success
    Result() : has_value_(true), dummy_(0) {}

    // Error constructors
    Result(const E& error) : has_value_(false), error_(error) {}
    Result(E&& error) : has_value_(false), error_(std::move(error)) {}

    // Copy constructor
    Result(const Result& other) : has_value_(other.has_value_) {
        if (has_value_) {
            dummy_ = other.dummy_;
        } else {
            if constexpr (std::is_copy_constructible<E>::value) {
                new (&error_) E(other.error_);
            }
        }
    }

    // Move constructor  
    Result(Result&& other) noexcept : has_value_(other.has_value_) {
        if (has_value_) {
            dummy_ = other.dummy_;
        } else {
            if constexpr (std::is_move_constructible<E>::value) {
                new (&error_) E(std::move(other.error_));
            }
        }
    }

    // Copy assignment
    Result& operator=(const Result& other) {
        if (this != &other) {
            if (has_value_ && other.has_value_) {
                dummy_ = other.dummy_;
            } else if (!has_value_ && !other.has_value_) {
                error_ = other.error_;
            } else if (has_value_ && !other.has_value_) {
                new (&error_) E(other.error_);
                has_value_ = false;
            } else {
                destroy_error();
                dummy_ = other.dummy_;
                has_value_ = true;
            }
        }
        return *this;
    }

    // Move assignment
    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            if (has_value_ && other.has_value_) {
                dummy_ = other.dummy_;
            } else if (!has_value_ && !other.has_value_) {
                error_ = std::move(other.error_);
            } else if (has_value_ && !other.has_value_) {
                new (&error_) E(std::move(other.error_));
                has_value_ = false;
            } else {
                destroy_error();
                dummy_ = other.dummy_;
                has_value_ = true;
            }
        }
        return *this;
    }

    ~Result() {
        if (!has_value_) {
            destroy_error();
        }
    }

    [[nodiscard]] bool has_value() const noexcept { return has_value_; }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value_; }

    [[nodiscard]] const E& error() const& { return error_; }
    [[nodiscard]] E& error() & { return error_; }  
    [[nodiscard]] E&& error() && { return std::move(error_); }
};

// Filesystem compatibility - provide basic stubs
namespace fs {
class path {
    std::string path_;
   public:
    path() = default;
    path(const std::string& p) : path_(p) {}
    path(const char* p) : path_(p) {}
    const std::string& string() const { return path_; }
    operator std::string() const { return path_; }
};
}

// Enhanced span implementation for C++20+ compatibility  
#if __cplusplus >= 202002L && __has_include(<span>)
// Use std::span when available in C++20+
template <typename T>
using span = std::span<T>;
#else
// Custom span implementation for compatibility
template <typename T>
class span {
private:
    T* data_;
    std::size_t size_;

public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;

    // Constructors
    constexpr span() noexcept : data_(nullptr), size_(0) {}
    constexpr span(T* data, std::size_t size) noexcept : data_(data), size_(size) {}
    
    // Array constructors
    template<std::size_t N>
    constexpr span(T (&array)[N]) noexcept : data_(array), size_(N) {}
    template<std::size_t N>
    constexpr span(const T (&array)[N]) noexcept : data_(const_cast<T*>(array)), size_(N) {}
    
    // std::array constructors
    template<std::size_t N>
    constexpr span(std::array<T, N>& array) noexcept : data_(array.data()), size_(N) {}
    template<std::size_t N>
    constexpr span(const std::array<T, N>& array) noexcept : data_(const_cast<T*>(array.data())), size_(N) {}
    
    // std::vector constructors
    template<typename Alloc>
    constexpr span(std::vector<T, Alloc>& vec) noexcept : data_(vec.data()), size_(vec.size()) {}
    template<typename Alloc>
    constexpr span(const std::vector<T, Alloc>& vec) noexcept : data_(const_cast<T*>(vec.data())), size_(vec.size()) {}
    
    // Compatible pointer types constructor
    template<typename U>
    constexpr span(U* data, std::size_t size, 
         typename std::enable_if<std::is_convertible<U*, T*>::value>::type* = nullptr) noexcept
        : data_(const_cast<T*>(data)), size_(size) {}
    
    // Compatible span conversion constructor - for other span types  
    template<typename U>
    constexpr span(const span<U>& other_span,
         typename std::enable_if<(std::is_same<T, byte>::value && 
                          (std::is_same<U, const byte>::value || std::is_same<U, byte>::value)) ||
                          std::is_convertible<U*, T*>::value>::type* = nullptr) noexcept
        : data_(reinterpret_cast<T*>(const_cast<typename std::remove_const<U>::type*>(other_span.data()))), 
          size_(other_span.size()) {}
    
    // Observers
    constexpr T* data() const noexcept { return data_; }
    constexpr std::size_t size() const noexcept { return size_; }
    constexpr bool empty() const noexcept { return size_ == 0; }
    constexpr std::size_t size_bytes() const noexcept { return size_ * sizeof(T); }
    
    // Element access
    constexpr T& operator[](std::size_t idx) const noexcept { return data_[idx]; }
    constexpr T& front() const noexcept { return data_[0]; }
    constexpr T& back() const noexcept { return data_[size_ - 1]; }
    
    // Iterators
    constexpr T* begin() const noexcept { return data_; }
    constexpr T* end() const noexcept { return data_ + size_; }
    
    // Subviews
    constexpr span<T> first(std::size_t count) const noexcept {
        return span<T>(data_, count);
    }
    constexpr span<T> last(std::size_t count) const noexcept {
        return span<T>(data_ + size_ - count, count);
    }
    constexpr span<T> subspan(std::size_t offset, std::size_t count = std::size_t(-1)) const noexcept {
        return span<T>(data_ + offset, count == std::size_t(-1) ? size_ - offset : count);
    }
};
#endif


}  // namespace Modern
}  // namespace Core
}  // namespace Manifest
