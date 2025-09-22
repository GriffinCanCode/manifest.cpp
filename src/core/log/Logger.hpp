#pragma once

#include "Record.hpp"
#include "Sink.hpp" 
#include "Level.hpp"
#include "../types/Modern.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <atomic>
#include <format>
#include <sstream>

namespace Manifest {
namespace Core {
namespace Log {

// Fast, thread-safe logger with compile-time optimization
class Logger {
private:
    std::string name_;
    std::atomic<Level> level_;
    std::shared_ptr<Sink> sink_;

public:
    explicit Logger(std::string name, 
                   std::shared_ptr<Sink> sink, 
                   Level level = Level::Info)
        : name_(std::move(name))
        , level_(level)
        , sink_(std::move(sink)) {}

    // Core logging method with perfect forwarding
    template<typename... Args>
    void log(Level level, std::string_view msg, Args&&... args) const {
        if (!should_log(level)) return;
        
        std::string formatted_msg;
        if constexpr (sizeof...(args) > 0) {
            try {
                formatted_msg = std::format(std::string{msg}, std::forward<Args>(args)...);
            } catch (...) {
                // Fallback to simple string stream formatting if std::format fails
                std::ostringstream oss;
                oss << msg;
                formatted_msg = oss.str();
            }
        } else {
            formatted_msg = std::string{msg};
        }
        
        Record record{level, name_, formatted_msg};
        sink_->log(record);
    }

    // Convenience methods for each level
    template<typename... Args>
    void trace(std::string_view msg, Args&&... args) const {
        log(Level::Trace, msg, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(std::string_view msg, Args&&... args) const {
        log(Level::Debug, msg, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(std::string_view msg, Args&&... args) const {
        log(Level::Info, msg, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(std::string_view msg, Args&&... args) const {
        log(Level::Warn, msg, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(std::string_view msg, Args&&... args) const {
        log(Level::Error, msg, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void fatal(std::string_view msg, Args&&... args) const {
        log(Level::Fatal, msg, std::forward<Args>(args)...);
    }

    // Configuration methods
    void set_level(Level level) noexcept { 
        level_ = level; 
    }
    
    [[nodiscard]] Level get_level() const noexcept { 
        return level_; 
    }
    
    [[nodiscard]] bool should_log(Level level) const noexcept {
        return level >= level_ && sink_->should_log(level);
    }

    void flush() const {
        sink_->flush();
    }

    [[nodiscard]] const std::string& name() const noexcept { 
        return name_; 
    }
};

// Factory for creating loggers with shared sinks
class Registry {
    static inline std::shared_ptr<Sink> default_sink_ = 
        std::make_shared<ConsoleSink>(true);
    static inline std::atomic<Level> default_level_{Level::Info};

public:
    // Create logger with default sink
    static std::unique_ptr<Logger> create(const std::string& name) {
        return std::make_unique<Logger>(name, default_sink_, default_level_);
    }

    // Create logger with custom sink
    static std::unique_ptr<Logger> create(const std::string& name, 
                                         std::shared_ptr<Sink> sink) {
        return std::make_unique<Logger>(name, std::move(sink), default_level_);
    }

    // Create logger with custom sink and level
    static std::unique_ptr<Logger> create(const std::string& name, 
                                         std::shared_ptr<Sink> sink,
                                         Level level) {
        return std::make_unique<Logger>(name, std::move(sink), level);
    }

    // Global configuration
    static void set_default_sink(std::shared_ptr<Sink> sink) {
        default_sink_ = std::move(sink);
    }

    static void set_default_level(Level level) {
        default_level_ = level;
    }

    [[nodiscard]] static Level get_default_level() noexcept {
        return default_level_;
    }
};

}  // namespace Log
}  // namespace Core
}  // namespace Manifest
