#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

namespace Manifest {
namespace Core {
namespace Log {

// Strongly typed log levels with compile-time optimization
enum class Level : std::uint8_t {
    Trace = 0,
    Debug = 1, 
    Info = 2,
    Warn = 3,
    Error = 4,
    Fatal = 5,
    Off = 6
};

// Compile-time level utilities
constexpr bool operator<(Level lhs, Level rhs) noexcept {
    return static_cast<std::uint8_t>(lhs) < static_cast<std::uint8_t>(rhs);
}

constexpr bool operator<=(Level lhs, Level rhs) noexcept {
    return static_cast<std::uint8_t>(lhs) <= static_cast<std::uint8_t>(rhs);
}

constexpr bool operator>(Level lhs, Level rhs) noexcept {
    return static_cast<std::uint8_t>(lhs) > static_cast<std::uint8_t>(rhs);
}

constexpr bool operator>=(Level lhs, Level rhs) noexcept {
    return static_cast<std::uint8_t>(lhs) >= static_cast<std::uint8_t>(rhs);
}

// Compile-time string conversion for zero-cost abstractions
constexpr std::string_view to_string(Level level) noexcept {
    switch (level) {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERROR";
        case Level::Fatal: return "FATAL";
        case Level::Off:   return "OFF";
    }
    return "UNKNOWN";
}

// Compile-time color codes for console output
constexpr std::string_view to_color(Level level) noexcept {
    switch (level) {
        case Level::Trace: return "\033[0;37m"; // White
        case Level::Debug: return "\033[0;36m"; // Cyan
        case Level::Info:  return "\033[0;32m"; // Green
        case Level::Warn:  return "\033[1;33m"; // Yellow
        case Level::Error: return "\033[0;31m"; // Red
        case Level::Fatal: return "\033[1;31m"; // Bold Red
        case Level::Off:   return "\033[0m";    // Reset
    }
    return "\033[0m";
}

constexpr std::string_view color_reset() noexcept {
    return "\033[0m";
}

// Template for compile-time level filtering
template<Level CompileLevel>
constexpr bool should_log(Level runtime_level) noexcept {
    return runtime_level >= CompileLevel;
}

}  // namespace Log
}  // namespace Core  
}  // namespace Manifest
