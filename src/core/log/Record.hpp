#pragma once

#include "Level.hpp"
#include "../types/Modern.hpp"
#include <chrono>
#include <string>
#include <string_view>
#include <thread>
#include <source_location>

namespace Manifest {
namespace Core {
namespace Log {

// High-performance log record with minimal allocations
struct Record {
    using TimePoint = std::chrono::system_clock::time_point;
    using ThreadId = std::thread::id;
    
    Level level;
    TimePoint timestamp;
    ThreadId thread_id;
    std::string_view logger_name;
    std::string_view message;
    std::source_location location;
    
    // Structured data support for modern logging
    template<typename... Args>
    Record(Level lvl, std::string_view name, std::string_view msg, 
           const std::source_location& loc = std::source_location::current()) noexcept
        : level(lvl)
        , timestamp(std::chrono::system_clock::now())
        , thread_id(std::this_thread::get_id()) 
        , logger_name(name)
        , message(msg)
        , location(loc) {}
    
    // Move-only semantics for efficiency
    Record(const Record&) = delete;
    Record& operator=(const Record&) = delete;
    Record(Record&&) = default;
    Record& operator=(Record&&) = default;
    
    // Utility methods for formatters
    [[nodiscard]] std::string timestamp_string() const;
    [[nodiscard]] std::string thread_string() const;
    [[nodiscard]] std::string location_string() const;
    [[nodiscard]] std::string full_message() const;
};

// Efficient time formatting (avoids std::put_time allocations)
inline std::string Record::timestamp_string() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()) % 1000;
    
    std::string result;
    result.reserve(23); // "YYYY-MM-DD HH:MM:SS.sss"
    
    std::tm* tm_val = std::localtime(&time_t_val);
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_val);
    
    result = buffer;
    result += '.';
    if (ms.count() < 100) result += '0';
    if (ms.count() < 10) result += '0';
    result += std::to_string(ms.count());
    
    return result;
}

inline std::string Record::thread_string() const {
    std::ostringstream oss;
    oss << thread_id;
    return oss.str().substr(0, 8); // Truncate for readability
}

inline std::string Record::location_string() const {
    std::string result;
    result.reserve(100);
    result += location.file_name();
    result += ':';
    result += std::to_string(location.line());
    return result;
}

inline std::string Record::full_message() const {
    return std::string{message};
}

}  // namespace Log
}  // namespace Core
}  // namespace Manifest
