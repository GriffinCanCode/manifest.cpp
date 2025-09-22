#pragma once

// Main logging system header - include this for full logging functionality
#include "Logger.hpp"
#include "Level.hpp"
#include "Sink.hpp"
#include "Record.hpp"
#include "../types/Modern.hpp"
#include <memory>
#include <string_view>

namespace Manifest {
namespace Core {
namespace Log {

// Global logger instance for convenience (singleton pattern)
class Global {
private:
    static inline std::unique_ptr<Logger> logger_ = Registry::create("Global");

public:
    // Get global logger instance
    [[nodiscard]] static Logger& get() noexcept {
        return *logger_;
    }

    // Replace global logger (for testing/configuration)
    static void set(std::unique_ptr<Logger> logger) {
        logger_ = std::move(logger);
    }

    // Convenience methods that delegate to global logger
    template<typename... Args>
    static void trace(std::string_view msg, Args&&... args) {
        logger_->trace(msg, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void debug(std::string_view msg, Args&&... args) {
        logger_->debug(msg, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void info(std::string_view msg, Args&&... args) {
        logger_->info(msg, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void warn(std::string_view msg, Args&&... args) {
        logger_->warn(msg, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(std::string_view msg, Args&&... args) {
        logger_->error(msg, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void fatal(std::string_view msg, Args&&... args) {
        logger_->fatal(msg, std::forward<Args>(args)...);
    }

    static void flush() {
        logger_->flush();
    }

    static void set_level(Level level) {
        logger_->set_level(level);
    }
};

}  // namespace Log
}  // namespace Core
}  // namespace Manifest

// Convenience macros for compile-time optimization and source location
// These are the ONLY macros in the system - kept minimal for performance

#define LOG_TRACE(...) \
    do { if (::Manifest::Core::Log::Global::get().should_log(::Manifest::Core::Log::Level::Trace)) \
        ::Manifest::Core::Log::Global::trace(__VA_ARGS__); } while(0)

#define LOG_DEBUG(...) \
    do { if (::Manifest::Core::Log::Global::get().should_log(::Manifest::Core::Log::Level::Debug)) \
        ::Manifest::Core::Log::Global::debug(__VA_ARGS__); } while(0)

#define LOG_INFO(...) \
    ::Manifest::Core::Log::Global::info(__VA_ARGS__)

#define LOG_WARN(...) \
    ::Manifest::Core::Log::Global::warn(__VA_ARGS__)

#define LOG_ERROR(...) \
    ::Manifest::Core::Log::Global::error(__VA_ARGS__)

#define LOG_FATAL(...) \
    ::Manifest::Core::Log::Global::fatal(__VA_ARGS__)

// Named logger creation helper
#define CREATE_LOGGER(name) \
    ::Manifest::Core::Log::Registry::create(name)

// Scoped logger for specific modules/classes
#define MODULE_LOGGER(name) \
    static inline auto logger_ = ::Manifest::Core::Log::Registry::create(name)

#define SCOPED_LOGGER_TRACE(logger, ...) \
    do { if ((logger).should_log(::Manifest::Core::Log::Level::Trace)) \
        (logger).trace(__VA_ARGS__); } while(0)

#define SCOPED_LOGGER_DEBUG(logger, ...) \
    do { if ((logger).should_log(::Manifest::Core::Log::Level::Debug)) \
        (logger).debug(__VA_ARGS__); } while(0)
